#ifndef GAME_RUN_CAPTURE_HPP
#define GAME_RUN_CAPTURE_HPP

#include "app/game/BuildModel.hpp"
#include "scoring/ScoreRules.hpp"

#include <cstdint>
#include <json.hpp>
#include <unordered_set>

class RunCapture {
    nlohmann::json m_data;
    std::int64_t m_nextFrameMs = 0;
    std::unordered_set<PartId> m_visibleParts;

public:
    void Start(const BuildModel& model, std::size_t passengerCount);
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
};

#endif
