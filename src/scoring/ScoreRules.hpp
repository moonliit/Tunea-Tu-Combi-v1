#ifndef SCORING_SCORE_RULES_HPP
#define SCORING_SCORE_RULES_HPP

#include <cstdint>
#include <string_view>

namespace Scoring {

inline constexpr std::string_view RulesVersion = "score-v2";

struct RunStatistics {
    std::int64_t initialParts = 0;
    std::int64_t passengersCollected = 0;
    std::int64_t passengersDelivered = 0;
    std::int64_t partsDelivered = 0;
    std::int64_t driverTimeMs = 0;
};

struct ScoreBreakdown {
    bool valid = false;
    std::int64_t passengerPickup = 0;
    std::int64_t passengerDelivery = 0;
    std::int64_t partDelivery = 0;
    std::int64_t constructionPenalty = 0;
    std::int64_t speed = 0;
    std::int64_t total = 0;
};

ScoreBreakdown CalculateScore(const RunStatistics& statistics);

}

#endif
