#include "app/game/GoalWindowTracker.hpp"

#include <algorithm>

namespace {

bool ReachedGoal(const GoalPartSample& part, double goalX) {
    if (part.currentX + part.horizontalRadius >= goalX) return true;
    const double sweptLeft =
        std::min(part.previousX, part.currentX) - part.horizontalRadius;
    const double sweptRight =
        std::max(part.previousX, part.currentX) + part.horizontalRadius;
    return sweptLeft <= goalX && sweptRight >= goalX;
}

}

GoalWindowUpdate GoalWindowTracker::Update(
    double previousTimeMs,
    double currentTimeMs,
    double goalX,
    const std::vector<GoalPartSample>& parts,
    const std::vector<GoalPassengerSample>& passengers
) {
    const auto driver = std::find_if(
        parts.begin(), parts.end(),
        [](const GoalPartSample& part) { return part.driver; }
    );
    if (driver == parts.end() || driver->currentX <= goalX) {
        DiscardWindow();
        return {};
    }

    if (m_startMs < 0.0) {
        m_deliveredParts.clear();
        m_deliveredPassengers.clear();
        m_startMs = previousTimeMs;
        if (driver->previousX <= goalX &&
            driver->currentX > driver->previousX) {
            const double fraction = std::clamp(
                (goalX - driver->previousX) /
                    (driver->currentX - driver->previousX),
                0.0,
                1.0
            );
            m_startMs += (currentTimeMs - previousTimeMs) * fraction;
        }
    }

    for (const GoalPartSample& part : parts) {
        if (part.initial && ReachedGoal(part, goalX)) {
            m_deliveredParts.insert(part.id);
        }
    }
    for (const GoalPassengerSample& passenger : passengers) {
        if (m_deliveredParts.contains(passenger.hostId)) {
            m_deliveredPassengers.insert(passenger.id);
        }
    }

    return {
        .active = true,
        .completed = currentTimeMs - m_startMs >= 3000.0,
        .startMs = m_startMs
    };
}

void GoalWindowTracker::Reset() {
    DiscardWindow();
}

void GoalWindowTracker::DiscardWindow() {
    m_startMs = -1.0;
    m_deliveredParts.clear();
    m_deliveredPassengers.clear();
}
