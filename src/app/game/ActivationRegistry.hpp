#ifndef GAME_ACTIVATION_REGISTRY_HPP
#define GAME_ACTIVATION_REGISTRY_HPP

#include "app/game/Part.hpp"

#include <array>
#include <cstddef>
#include <vector>

class EditorGrid;

enum class ActivationAction : std::size_t {
    FanUp,
    FanRight,
    FanDown,
    FanLeft,
    WindSailUp,
    WindSailRight,
    WindSailDown,
    WindSailLeft,
    TNT,
    Count
};

class ActivationRegistry {
public:
    using ActionList = std::array<ActivationAction, static_cast<std::size_t>(
        ActivationAction::Count
    )>;

    void Clear();
    void Subscribe(ActivationAction action, Part& part);
    bool IsActive(ActivationAction action) const;
    bool HasSubscribers(ActivationAction action) const;
    const std::vector<Part*>& GetSubscribers(ActivationAction action) const;
    std::vector<ActivationAction> GetVisibleActions() const;
    static std::vector<ActivationAction> GetActionsForGrid(const EditorGrid& grid);

    void Publish(ActivationAction action);
    void Update(float deltaTime);

    static const ActionList& GetAllActions();

private:
    struct ActionState {
        std::vector<Part*> subscribers;
    };

    std::array<ActionState, static_cast<std::size_t>(
        ActivationAction::Count
    )> m_actions;
};

bool IsFanAction(ActivationAction action);
bool IsWindSailAction(ActivationAction action);
ActivationAction GetFanAction(PartOrientation orientation);
ActivationAction GetWindSailAction(PartOrientation orientation);
PartOrientation GetPropulsionFacingOrientation(PartOrientation orientation);
const char* GetActivationLabel(ActivationAction action);

#endif
