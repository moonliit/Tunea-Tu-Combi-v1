#include "app/game/GoalWindowTracker.hpp"

#include <algorithm>

namespace {

Math::AABB PartBounds(double centerX, double centerY, double radius) {
    return Math::AABB(
        Math::Vector2D(centerX - radius, centerY - radius),
        Math::Vector2D(centerX + radius, centerY + radius)
    );
}

bool TouchesGoal(const GoalPartSample& part, const Math::AABB& goalArea) {
    if (PartBounds(part.currentX, part.currentY, part.horizontalRadius).Overlaps(goalArea)) {
        return true;
    }
    const double minX =
        std::min(part.previousX, part.currentX) - part.horizontalRadius;
    const double maxX =
        std::max(part.previousX, part.currentX) + part.horizontalRadius;
    const double minY =
        std::min(part.previousY, part.currentY) - part.horizontalRadius;
    const double maxY =
        std::max(part.previousY, part.currentY) + part.horizontalRadius;
    return Math::AABB(Math::Vector2D(minX, minY), Math::Vector2D(maxX, maxY))
        .Overlaps(goalArea);
}

}

GoalWindowUpdate GoalWindowTracker::Update(
    double previousTimeMs,
    double currentTimeMs,
    const Math::AABB& goalArea,
    const std::vector<GoalPartSample>& parts,
    const std::vector<GoalPassengerSample>& passengers
) {
    const auto driver = std::find_if(
        parts.begin(), parts.end(),
        [](const GoalPartSample& part) { return part.driver; }
    );
    if (driver == parts.end()) {
        if (m_startMs < 0.0) return {};
        // goal already reached: keep the win/countdown even if driver disappears
    } else if (m_startMs < 0.0) {
        if (!TouchesGoal(*driver, goalArea)) {
            DiscardWindow();
            return {};
        }
        m_deliveredParts.clear();
        m_deliveredPassengers.clear();
        m_startMs = previousTimeMs;
        if (!PartBounds(driver->previousX, driver->previousY, driver->horizontalRadius)
                 .Overlaps(goalArea)) {
            const double dx = driver->currentX - driver->previousX;
            const double dy = driver->currentY - driver->previousY;
            double enterFraction = 1.0;
            bool found = false;
            const auto consider = [&](double boundary, double previous, double delta) {
                if (std::abs(delta) < 1e-9) return;
                const double fraction = (boundary - previous) / delta;
                if (fraction < 0.0 || fraction > 1.0) return;
                const double x = driver->previousX + dx * fraction;
                const double y = driver->previousY + dy * fraction;
                if (!PartBounds(x, y, driver->horizontalRadius).Overlaps(goalArea)) return;
                if (!found || fraction < enterFraction) {
                    enterFraction = fraction;
                    found = true;
                }
            };
            consider(goalArea.min.x + driver->horizontalRadius, driver->previousX, dx);
            consider(goalArea.max.x - driver->horizontalRadius, driver->previousX, dx);
            consider(goalArea.min.y + driver->horizontalRadius, driver->previousY, dy);
            consider(goalArea.max.y - driver->horizontalRadius, driver->previousY, dy);
            if (found) {
                m_startMs += (currentTimeMs - previousTimeMs) *
                    std::clamp(enterFraction, 0.0, 1.0);
            }
        }
    }
    // once started, do not cancel if the driver leaves the goal area

    for (const GoalPartSample& part : parts) {
        if (part.initial && TouchesGoal(part, goalArea)) {
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
