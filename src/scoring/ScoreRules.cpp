#include "scoring/ScoreRules.hpp"

#include <algorithm>

namespace Scoring {
namespace {

constexpr std::int64_t PickupPoints = 500;
constexpr std::int64_t DeliveryPoints = 1000;
constexpr std::int64_t PartDeliveryPoints = 3000;
constexpr std::int64_t ExtraPartsToHalveScore = 25;
constexpr std::int64_t MinimumBuildParts = 2;
constexpr std::int64_t SpeedPoints = 3000;
constexpr std::int64_t SpeedLimitMs = 30000;
constexpr std::int64_t MaximumCount = 1000000;

std::int64_t RoundHalfUp(std::int64_t numerator, std::int64_t denominator) {
    return (numerator + denominator / 2) / denominator;
}

std::int64_t CeilDivide(std::int64_t numerator, std::int64_t denominator) {
    return (numerator + denominator - 1) / denominator;
}

}

ScoreBreakdown CalculateScore(const RunStatistics& statistics) {
    ScoreBreakdown result;
    if (statistics.initialParts <= 0 ||
        statistics.passengersCollected < 0 ||
        statistics.passengersDelivered < 0 ||
        statistics.partsDelivered < 0 ||
        statistics.driverTimeMs < 0 ||
        statistics.initialParts > MaximumCount ||
        statistics.passengersCollected > MaximumCount ||
        statistics.passengersDelivered > MaximumCount ||
        statistics.partsDelivered > MaximumCount ||
        statistics.passengersDelivered > statistics.passengersCollected ||
        statistics.partsDelivered > statistics.initialParts) {
        return result;
    }

    result.valid = true;
    result.passengerPickup = PickupPoints * statistics.passengersCollected;
    result.passengerDelivery = DeliveryPoints * statistics.passengersDelivered;
    result.partDelivery = RoundHalfUp(
        PartDeliveryPoints * statistics.partsDelivered,
        statistics.initialParts
    );
    result.speed = statistics.driverTimeMs >= SpeedLimitMs
        ? 0
        : SpeedPoints - CeilDivide(
            SpeedPoints * statistics.driverTimeMs,
            SpeedLimitMs
        );
    const std::int64_t earned = result.passengerPickup +
        result.passengerDelivery + result.partDelivery + result.speed;
    const std::int64_t extraParts = std::max<std::int64_t>(
        0,
        statistics.initialParts - MinimumBuildParts
    );
    result.total = RoundHalfUp(
        earned * ExtraPartsToHalveScore,
        ExtraPartsToHalveScore + extraParts
    );
    result.constructionPenalty = result.total - earned;
    return result;
}

}
