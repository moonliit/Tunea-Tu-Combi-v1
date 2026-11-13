#include "app/game/RunCapture.hpp"

#include "app/game/Part.hpp"

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
EM_JS(void, SaveRunCaptureWeb, (const char* json), {
    localStorage.setItem('tunea-tu-combi:last-run', UTF8ToString(json));
});
#endif

namespace {

constexpr std::int64_t FrameIntervalMs = 50;

std::string CreateAttemptId() {
    std::array<unsigned char, 16> bytes;
    std::random_device random;
    for (unsigned char& byte : bytes) {
        byte = static_cast<unsigned char>(random());
    }
    bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0f) | 0x40);
    bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3f) | 0x80);
    constexpr char Hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(36);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        if (index == 4 || index == 6 || index == 8 || index == 10) {
            result.push_back('-');
        }
        result.push_back(Hex[bytes[index] >> 4]);
        result.push_back(Hex[bytes[index] & 0x0f]);
    }
    return result;
}

template<typename Callback>
void VisitPart(const Part& part, PartId parent, const Callback& callback) {
    callback(part, parent);
    if (part.HasContainedPart()) {
        VisitPart(*part.GetContainedPart(), part.GetId(), callback);
    }
}

template<typename Callback>
void VisitGrid(const EditorGrid& grid, const Callback& callback) {
    for (const Part* part : grid.GetActiveParts()) {
        VisitPart(*part, 0, callback);
    }
}

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

}

void RunCapture::Start(
    const BuildModel& model,
    std::size_t passengerCount,
    const std::string& levelId
) {
    m_attemptId = CreateAttemptId();
    m_data = {
        {"attemptId", m_attemptId},
        {"formatVersion", "run-v1"},
        {"levelId", levelId},
        {"physicsVersion", "custom-physics-v1"},
        {"rulesVersion", Scoring::RulesVersion},
        {"passengerCount", passengerCount},
        {"construction", nlohmann::json::array()},
        {"connections", nlohmann::json::array()},
        {"frames", nlohmann::json::array()},
        {"events", nlohmann::json::array()}
    };
    m_nextFrameMs = 0;
    m_visibleParts.clear();

    VisitGrid(model.GetGrid(), [this](const Part& part, PartId parent) {
        m_visibleParts.insert(part.GetId());
        m_data["construction"].push_back({
            {"id", part.GetId()},
            {"type", static_cast<int>(part.GetType())},
            {"gridX", part.GetGridCoord().x},
            {"gridY", part.GetGridCoord().y},
            {"orientation", static_cast<int>(part.GetOrientation())},
            {"containedBy", parent}
        });
    });
    for (const PartConnection& connection : model.GetConnections()) {
        m_data["connections"].push_back({
            {"first", connection.first},
            {"second", connection.second}
        });
    }
}

void RunCapture::RecordFrame(
    std::int64_t timeMs,
    const EditorGrid& grid,
    bool force
) {
    if (!force && timeMs < m_nextFrameMs) return;
    m_nextFrameMs = timeMs + FrameIntervalMs;

    nlohmann::json states = nlohmann::json::array();
    std::unordered_set<PartId> current;
    VisitGrid(grid, [&](const Part& part, PartId) {
        current.insert(part.GetId());
        const PartRenderState state = part.GetRenderState(grid);
        states.push_back({
            {"id", part.GetId()},
            {"xMilli", std::llround(static_cast<double>(state.position.x) * 1000.0)},
            {"yMilli", std::llround(static_cast<double>(state.position.y) * 1000.0)},
            {"rotationMilli", std::llround(static_cast<double>(state.rotation) * 1000.0)}
        });
    });
    for (const PartId id : current) {
        if (!m_visibleParts.contains(id)) {
            m_data["events"].push_back({
                {"timeMs", timeMs}, {"type", "appear"}, {"id", id}
            });
        }
    }
    for (const PartId id : m_visibleParts) {
        if (!current.contains(id)) {
            m_data["events"].push_back({
                {"timeMs", timeMs}, {"type", "disappear"}, {"id", id}
            });
        }
    }
    m_visibleParts = std::move(current);
    m_data["frames"].push_back({{"timeMs", timeMs}, {"parts", std::move(states)}});
}

void RunCapture::RecordPassengerPickup(std::int64_t timeMs, std::size_t passengerId) {
    m_data["events"].push_back({
        {"timeMs", timeMs}, {"type", "passengerPickup"}, {"id", passengerId}
    });
}

void RunCapture::RecordDetach(std::int64_t timeMs, PartId first, PartId second) {
    m_data["events"].push_back({
        {"timeMs", timeMs},
        {"type", "detach"},
        {"first", first},
        {"second", second}
    });
}

void RunCapture::Finish(
    std::int64_t timeMs,
    const Scoring::RunStatistics& statistics,
    const Scoring::ScoreBreakdown& score
) {
    m_data["finishedAtMs"] = timeMs;
    m_data["statistics"] = StatisticsJson(statistics);
    m_data["score"] = ScoreJson(score);
    const std::string serialized = m_data.dump();
#ifdef __EMSCRIPTEN__
    SaveRunCaptureWeb(serialized.c_str());
#else
    std::filesystem::create_directories("user_data");
    std::ofstream("user_data/last_run.json") << serialized;
#endif
}

void RunCapture::Clear() {
    m_data = nlohmann::json{};
    m_attemptId.clear();
    m_visibleParts.clear();
    m_nextFrameMs = 0;
}
