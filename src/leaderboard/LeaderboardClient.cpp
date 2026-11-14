#include "leaderboard/LeaderboardClient.hpp"

#include <filesystem>
#include <fstream>
#include <json.hpp>

#ifdef __EMSCRIPTEN__
#include <cstdlib>
#include <emscripten.h>
EM_JS(void, SubmitLeaderboardWeb, (const char* json), {
    Module['api'].submitRun(JSON.parse(UTF8ToString(json)));
});
EM_JS(char*, ReadLeaderboardWeb, (), {
    return stringToNewUTF8(JSON.stringify(Module['api'].leaderboardSnapshot()));
});
EM_JS_DEPS(leaderboard_deps, "$stringToNewUTF8");
#endif

namespace {

nlohmann::json StatisticsJson(const Scoring::RunStatistics& statistics) {
    return {
        {"initialParts", statistics.initialParts},
        {"passengersCollected", statistics.passengersCollected},
        {"passengersDelivered", statistics.passengersDelivered},
        {"partsDelivered", statistics.partsDelivered},
        {"driverTimeMs", statistics.driverTimeMs}
    };
}

nlohmann::json ScoreJson(const Scoring::ScoreBreakdown& score) {
    return {
        {"valid", score.valid},
        {"passengerPickup", score.passengerPickup},
        {"passengerDelivery", score.passengerDelivery},
        {"partDelivery", score.partDelivery},
        {"constructionPenalty", score.constructionPenalty},
        {"speed", score.speed},
        {"total", score.total}
    };
}

PersonalBest ReadPersonalBest(const nlohmann::json& value) {
    if (!value.is_object()) return {};
    const auto& statistics = value.value("statistics", nlohmann::json::object());
    const auto& score = value.value("score", nlohmann::json::object());
    if (!statistics.is_object() || !score.is_object()) return {};
    return {
        .available = true,
        .currentAttempt = value.value("currentAttempt", false),
        .total = score.value("total", std::int64_t{0}),
        .driverTimeMs = statistics.value("driverTimeMs", std::int64_t{0}),
        .initialParts = statistics.value("initialParts", std::int64_t{0})
    };
}

bool BetterRecord(const nlohmann::json& first, const nlohmann::json& second) {
    const auto& firstScore = first.at("score");
    const auto& secondScore = second.at("score");
    const auto& firstStats = first.at("statistics");
    const auto& secondStats = second.at("statistics");
    if (firstScore.at("total") != secondScore.at("total")) {
        return firstScore.at("total").get<std::int64_t>() >
            secondScore.at("total").get<std::int64_t>();
    }
    if (firstStats.at("driverTimeMs") != secondStats.at("driverTimeMs")) {
        return firstStats.at("driverTimeMs").get<std::int64_t>() <
            secondStats.at("driverTimeMs").get<std::int64_t>();
    }
    return firstStats.at("initialParts").get<std::int64_t>() <
        secondStats.at("initialParts").get<std::int64_t>();
}

}

void LeaderboardClient::Submit(
    const std::string& levelId,
    const std::string& attemptId,
    const IdentityScope& scope,
    const Scoring::RunStatistics& statistics,
    const Scoring::ScoreBreakdown& score,
    const nlohmann::json& capture
) {
    m_attemptId = attemptId;
    m_combiId = scope.combiId;
    m_pollDelayMs = 0.0;
    m_snapshot = {.status = LeaderboardStatus::Loading};
    const nlohmann::json payload = {
        {"attemptId", attemptId},
        {"formatVersion", "run-v2"},
        {"levelId", levelId},
        {"physicsVersion", "custom-physics-v1"},
        {"rulesVersion", std::string(Scoring::RulesVersion)},
        {"scope", {{"id", scope.combiId}, {"epoch", scope.epoch}}},
        {"statistics", StatisticsJson(statistics)},
        {"localScore", ScoreJson(score)},
        {"capture", capture}
    };

#ifdef __EMSCRIPTEN__
    const std::string serialized = payload.dump();
    SubmitLeaderboardWeb(serialized.c_str());
    Poll(250.0);
#else
    std::filesystem::create_directories("user_data");
    const std::filesystem::path path = "user_data/personal_records.json";
    nlohmann::json records = nlohmann::json::object();
    std::ifstream input(path);
    if (input) {
        records = nlohmann::json::parse(input, nullptr, false);
        if (!records.is_object()) records = nlohmann::json::object();
    }
    const std::string key = (scope.combiId.empty() ? "native-local" : scope.combiId) +
        "|" + (levelId.empty() ? "local" : levelId);
    nlohmann::json candidate = {
        {"attemptId", attemptId},
        {"statistics", StatisticsJson(statistics)},
        {"score", ScoreJson(score)}
    };
    bool replace = !records.contains(key);
    if (!replace) {
        try {
            replace = BetterRecord(candidate, records.at(key));
        } catch (const nlohmann::json::exception&) {
            replace = true;
        }
    }
    if (replace) {
        records[key] = candidate;
        std::ofstream(path) << records.dump();
    }
    nlohmann::json best = records.at(key);
    best["currentAttempt"] = best.value("attemptId", "") == attemptId;
    m_snapshot = {
        .status = LeaderboardStatus::Unavailable,
        .personalBest = ReadPersonalBest(best),
        .message = "Ranking global no disponible."
    };
#endif
}

void LeaderboardClient::Poll(double frameMs) {
#ifdef __EMSCRIPTEN__
    m_pollDelayMs -= frameMs;
    if (m_pollDelayMs > 0.0) return;
    m_pollDelayMs = 250.0;
    char* raw = ReadLeaderboardWeb();
    const nlohmann::json state = nlohmann::json::parse(raw, nullptr, false);
    std::free(raw);
    if (state.is_discarded() || !state.is_object()) return;

    const std::string status = state.value("status", "idle");
    m_snapshot.status = status == "loading"
        ? LeaderboardStatus::Loading
        : status == "ready"
        ? LeaderboardStatus::Ready
        : status == "unavailable"
        ? LeaderboardStatus::Unavailable
        : LeaderboardStatus::Idle;
    m_snapshot.message = state.value("message", "");
    m_snapshot.championRunRef = state.value("championRunRef", "");
    m_snapshot.personalBest = ReadPersonalBest(
        state.value("personalBest", nlohmann::json::object())
    );
    const auto submission = state.value("submission", nlohmann::json::object());
    m_snapshot.submissionConfirmed = submission.is_object();
    m_snapshot.previousBestRetained = submission.is_object() &&
        submission.value("previousBestRetained", false);
    m_snapshot.entries.clear();
    for (const auto& entry : state.value("entries", nlohmann::json::array())) {
        const auto& statistics = entry.at("statistics");
        const auto& score = entry.at("score");
        const std::string name = entry.contains("name") && entry["name"].is_string()
            ? entry["name"].get<std::string>()
            : "";
        m_snapshot.entries.push_back({
            .rank = entry.value("rank", 0),
            .combiId = entry.value("combiId", ""),
            .name = name,
            .attemptId = entry.value("attemptId", ""),
            .runRef = entry.value("runRef", ""),
            .total = score.value("total", std::int64_t{0}),
            .driverTimeMs = statistics.value("driverTimeMs", std::int64_t{0}),
            .initialParts = statistics.value("initialParts", std::int64_t{0})
        });
    }
#else
    static_cast<void>(frameMs);
#endif
}

void LeaderboardClient::Reset() {
    m_snapshot = {};
    m_attemptId.clear();
    m_combiId.clear();
    m_pollDelayMs = 0.0;
}
