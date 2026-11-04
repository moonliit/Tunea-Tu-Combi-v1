#ifndef INPUT_GAME_INPUT_HPP
#define INPUT_GAME_INPUT_HPP

#include "math/Vector2D.hpp"
#include "game/ActivationRegistry.hpp"
#include "game/AppState.hpp"
#include "ui/UiControl.hpp"

#include <optional>

struct GameInputFrame {
    bool toggleMode = false;
    bool placePart = false;
    bool removePart = false;
    bool rotatePart = false;
    Math::Vector2D cameraMovement = Math::Vector2D::Zero();
    Math::Vector2D mouseScreenPosition = Math::Vector2D::Zero();
    std::optional<ToolbarPart> toolbarPart;
    std::optional<ActivationAction> activationAction;
};

class GameInput {
public:
    GameInputFrame Capture(
        float deltaTime,
        AppState state,
        const ActivationRegistry& activationRegistry
    ) const;
};

#endif
