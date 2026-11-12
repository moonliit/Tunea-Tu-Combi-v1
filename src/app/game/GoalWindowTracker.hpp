#ifndef GAME_GOAL_WINDOW_TRACKER_HPP
#define GAME_GOAL_WINDOW_TRACKER_HPP

#include "app/game/Part.hpp"

#include <cstddef>
#include <unordered_set>
#include <vector>

struct GoalPartSample {
    PartId id;
    double previousX;
    double currentX;
    double horizontalRadius;
    bool initial;
    bool driver;
};

struct GoalPassengerSample {
    std::size_t id;
    PartId hostId;
};

struct GoalWindowUpdate {
    bool active = false;
    bool completed = false;
    double startMs = -1.0;
};

class GoalWindowTracker {
private:
    double m_startMs = -1.0;
    std::unordered_set<PartId> m_deliveredParts;
    std::unordered_set<std::size_t> m_deliveredPassengers;

    void DiscardWindow();

public:
    GoalWindowUpdate Update(
        double previousTimeMs,
        double currentTimeMs,
        double goalX,
        const std::vector<GoalPartSample>& parts,
        const std::vector<GoalPassengerSample>& passengers
    );
    void Reset();

    double GetStartMs() const { return m_startMs; }
    const std::unordered_set<PartId>& GetDeliveredParts() const {
        return m_deliveredParts;
    }
    const std::unordered_set<std::size_t>& GetDeliveredPassengers() const {
        return m_deliveredPassengers;
    }
};

#endif
