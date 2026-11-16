#include "app/game/GoalWindowTracker.hpp"

#include <algorithm>
#include <optional>

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

std::optional<double> GoalEntryFraction(
    const GoalPartSample& part,
    const Math::AABB& goalArea
) {
    if (!TouchesGoal(part, goalArea)) return std::nullopt;
    if (PartBounds(part.previousX, part.previousY, part.horizontalRadius)
            .Overlaps(goalArea)) {
        return 0.0;
    }

    const double dx = part.currentX - part.previousX;
    const double dy = part.currentY - part.previousY;
    double entryFraction = 1.0;
    bool found = false;
    const auto consider = [&](double boundary, double previous, double delta) {
        if (std::abs(delta) < 1e-9) return;
        const double fraction = (boundary - previous) / delta;
        if (fraction < 0.0 || fraction > 1.0) return;
        const double x = part.previousX + dx * fraction;
        const double y = part.previousY + dy * fraction;
        if (!PartBounds(x, y, part.horizontalRadius).Overlaps(goalArea)) return;
        if (!found || fraction < entryFraction) {
            entryFraction = fraction;
            found = true;
        }
    };

    consider(goalArea.min.x + part.horizontalRadius, part.previousX, dx);
    consider(goalArea.max.x - part.horizontalRadius, part.previousX, dx);
    consider(goalArea.min.y + part.horizontalRadius, part.previousY, dy);
    consider(goalArea.max.y - part.horizontalRadius, part.previousY, dy);
    return found ? std::optional<double>{std::clamp(entryFraction, 0.0, 1.0)}
        : std::optional<double>{0.0};
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
    const bool hasDriverVehicle = std::any_of(
        parts.begin(), parts.end(),
        [](const GoalPartSample& part) { return part.driverVehiclePart; }
    );
    if (driver == parts.end()) {
        if (m_startMs < 0.0) return {};
        // goal already reached: keep the win/countdown even if driver disappears
    } else if (m_startMs < 0.0) {
        std::optional<double> entryFraction;
        if (const auto driverEntry = GoalEntryFraction(*driver, goalArea);
            driverEntry.has_value()) {
            entryFraction = driverEntry;
        }
        if (hasDriverVehicle) {
            for (const GoalPartSample& part : parts) {
                if (!part.driverVehiclePart) continue;
                if (const auto vehicleEntry = GoalEntryFraction(
                        part, goalArea); vehicleEntry.has_value()) {
                    entryFraction = entryFraction.has_value()
                        ? std::min(*entryFraction, *vehicleEntry)
                        : vehicleEntry;
                }
            }
        }
        if (!entryFraction.has_value()) {
            DiscardWindow();
            return {};
        }
        m_deliveredParts.clear();
        m_deliveredPassengers.clear();
        m_startMs = previousTimeMs;
        m_startMs += (currentTimeMs - previousTimeMs) * *entryFraction;
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
