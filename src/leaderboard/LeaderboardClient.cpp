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

std::string TextField(const nlohmann::json& object, const char* key) {
    const auto found = object.find(key);
    return found != object.end() && found->is_string() ? found->get<std::string>() : std::string{};
}

std::int64_t NumberField(const nlohmann::json& object, const char* key) {
    const auto found = object.find(key);
    return found != object.end() && found->is_number() ? found->get<std::int64_t>() : 0;
}

bool BoolField(const nlohmann::json& object, const char* key) {
    const auto found = object.find(key);
    return found != object.end() && found->is_boolean() && found->get<bool>();
}

const nlohmann::json& ObjectField(const nlohmann::json& object, const char* key) {
    static const nlohmann::json none = nlohmann::json::object();
    const auto found = object.find(key);
    return found != object.end() && found->is_object() ? *found : none;
}

PersonalBest ReadPersonalBest(const nlohmann::json& value) {
    if (!value.is_object()) return {};
    const auto& statistics = value.value("statistics", nlohmann::json::object());
    const auto& score = value.value("score", nlohmann::json::object());
    if (!statistics.is_object() || !score.is_object()) return {};
    return {
        .available = true,
        .currentAttempt = BoolField(value, "currentAttempt"),
        .total = NumberField(score, "total"),
        .driverTimeMs = NumberField(statistics, "driverTimeMs"),
        .initialParts = NumberField(statistics, "initialParts")
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

    const std::string status = TextField(state, "status");
    m_snapshot.status = status == "loading"
        ? LeaderboardStatus::Loading
        : status == "ready"
        ? LeaderboardStatus::Ready
        : status == "unavailable"
        ? LeaderboardStatus::Unavailable
        : LeaderboardStatus::Idle;
    m_snapshot.message = TextField(state, "message");
    m_snapshot.championRunRef = TextField(state, "championRunRef");
    const auto personalBest = state.find("personalBest");
    m_snapshot.personalBest = personalBest != state.end()
        ? ReadPersonalBest(*personalBest)
        : PersonalBest{};
    const auto submission = state.value("submission", nlohmann::json::object());
    m_snapshot.submissionConfirmed = submission.is_object();
    m_snapshot.previousBestRetained = submission.is_object() &&
        BoolField(submission, "previousBestRetained");
    m_snapshot.entries.clear();
    const auto entries = state.find("entries");
    if (entries == state.end() || !entries->is_array()) return;
    for (const auto& entry : *entries) {
        if (!entry.is_object()) continue;
        const auto& statistics = ObjectField(entry, "statistics");
        const auto& score = ObjectField(entry, "score");
        m_snapshot.entries.push_back({
            .rank = static_cast<int>(NumberField(entry, "rank")),
            .combiId = TextField(entry, "combiId"),
            .name = TextField(entry, "name"),
            .attemptId = TextField(entry, "attemptId"),
            .runRef = TextField(entry, "runRef"),
            .total = NumberField(score, "total"),
            .driverTimeMs = NumberField(statistics, "driverTimeMs"),
            .initialParts = NumberField(statistics, "initialParts")
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
