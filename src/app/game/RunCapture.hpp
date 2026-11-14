#ifndef GAME_RUN_CAPTURE_HPP
#define GAME_RUN_CAPTURE_HPP

#include "app/game/BuildModel.hpp"
#include "scoring/ScoreRules.hpp"

#include <cstdint>
#include <json.hpp>
#include <string>
#include <unordered_set>

class RunCapture {
    nlohmann::json m_data;
    std::string m_attemptId;
    std::int64_t m_nextFrameMs = 0;
    std::unordered_set<PartId> m_visibleParts;

public:
    void Start(
        const BuildModel& model,
        std::size_t passengerCount,
        const std::string& levelId,
        const std::string& combiId,
        const nlohmann::json& levelDefinition,
        const nlohmann::json& draftSnapshot
    );
    void RecordFrame(
        std::int64_t timeMs,
        const EditorGrid& grid,
        bool force = false
    );
    void RecordPassengerPickup(std::int64_t timeMs, std::size_t passengerId);
    void RecordDetach(std::int64_t timeMs, PartId first, PartId second);
    void Finish(
        std::int64_t timeMs,
        const Scoring::RunStatistics& statistics,
        const Scoring::ScoreBreakdown& score
    );
    void Clear();
    const std::string& GetAttemptId() const { return m_attemptId; }
    const nlohmann::json& GetData() const { return m_data; }
};

#endif
