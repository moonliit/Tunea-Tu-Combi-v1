#include "game/ActivationRegistry.hpp"

#include <algorithm>

namespace {

constexpr std::size_t ActionIndex(ActivationAction action) {
    return static_cast<std::size_t>(action);
}

}

void ActivationRegistry::Clear() {
    for (ActionState& action : m_actions) {
        action.subscribers.clear();
    }
}

void ActivationRegistry::Subscribe(ActivationAction action, Part& part) {
    auto& subscribers = m_actions[ActionIndex(action)].subscribers;
    if (std::find(subscribers.begin(), subscribers.end(), &part) ==
        subscribers.end()) {
        subscribers.push_back(&part);
    }
}

bool ActivationRegistry::IsActive(ActivationAction action) const {
    for (Part* part : GetSubscribers(action)) {
        if (part->IsActivationActive()) return true;
    }
    return false;
}

void ActivationRegistry::Publish(ActivationAction action) {
    for (Part* part : GetSubscribers(action)) {
        part->Activate();
    }
}

void ActivationRegistry::Update(float deltaTime) {
    for (ActionState& action : m_actions) {
        for (Part* part : action.subscribers) {
            part->UpdateActivation(deltaTime);
        }
    }
}

bool ActivationRegistry::HasSubscribers(ActivationAction action) const {
    return !m_actions[ActionIndex(action)].subscribers.empty();
}

const std::vector<Part*>& ActivationRegistry::GetSubscribers(
    ActivationAction action
) const {
    return m_actions[ActionIndex(action)].subscribers;
}

std::vector<ActivationAction> ActivationRegistry::GetVisibleActions() const {
    std::vector<ActivationAction> visible;
    for (const ActivationAction action : GetAllActions()) {
        if (HasSubscribers(action)) visible.push_back(action);
    }
    return visible;
}

const ActivationRegistry::ActionList& ActivationRegistry::GetAllActions() {
    static constexpr ActionList actions = {
        ActivationAction::FanUp,
        ActivationAction::FanRight,
        ActivationAction::FanDown,
        ActivationAction::FanLeft,
        ActivationAction::WindSailUp,
        ActivationAction::WindSailRight,
        ActivationAction::WindSailDown,
        ActivationAction::WindSailLeft,
        ActivationAction::TNT
    };
    return actions;
}

bool IsFanAction(ActivationAction action) {
    return action >= ActivationAction::FanUp &&
        action <= ActivationAction::FanLeft;
}

bool IsWindSailAction(ActivationAction action) {
    return action >= ActivationAction::WindSailUp &&
        action <= ActivationAction::WindSailLeft;
}

ActivationAction GetFanAction(PartOrientation orientation) {
    return static_cast<ActivationAction>(
        static_cast<std::size_t>(
            GetPropulsionFacingOrientation(orientation)
        )
    );
}

ActivationAction GetWindSailAction(PartOrientation orientation) {
    return static_cast<ActivationAction>(
        static_cast<std::size_t>(ActivationAction::WindSailUp) +
        static_cast<std::size_t>(
            GetPropulsionFacingOrientation(orientation)
        )
    );
}

PartOrientation GetPropulsionFacingOrientation(PartOrientation orientation) {
    switch (orientation) {
        case PartOrientation::Up: return PartOrientation::Right;
        case PartOrientation::Right: return PartOrientation::Down;
        case PartOrientation::Down: return PartOrientation::Left;
        case PartOrientation::Left: return PartOrientation::Up;
    }
    return PartOrientation::Right;
}

const char* GetActivationLabel(ActivationAction action) {
    switch (action) {
        case ActivationAction::FanUp: return "Fan Up";
        case ActivationAction::FanRight: return "Fan Right";
        case ActivationAction::FanDown: return "Fan Down";
        case ActivationAction::FanLeft: return "Fan Left";
        case ActivationAction::WindSailUp: return "Sail Up";
        case ActivationAction::WindSailRight: return "Sail Right";
        case ActivationAction::WindSailDown: return "Sail Down";
        case ActivationAction::WindSailLeft: return "Sail Left";
        case ActivationAction::TNT: return "TNT";
        case ActivationAction::Count: break;
    }
    return "Action";
}
