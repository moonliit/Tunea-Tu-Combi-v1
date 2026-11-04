#include "input/GameInput.hpp"

#include "ui/UiLayout.hpp"

#include <raylib.h>

GameInputFrame GameInput::Capture(
    float deltaTime,
    AppState state,
    const ActivationRegistry& activationRegistry
) const {
    GameInputFrame frame;
    frame.toggleMode = IsKeyPressed(KEY_ENTER);
    frame.rotatePart = IsKeyPressed(KEY_R) || GetMouseWheelMove() != 0.0f;
    frame.mouseScreenPosition = {
        GetMousePosition().x,
        GetMousePosition().y
    };

    const float cameraSpeed = 300.0f * deltaTime;
    if (IsKeyDown(KEY_W)) frame.cameraMovement.y -= cameraSpeed;
    if (IsKeyDown(KEY_S)) frame.cameraMovement.y += cameraSpeed;
    if (IsKeyDown(KEY_A)) frame.cameraMovement.x -= cameraSpeed;
    if (IsKeyDown(KEY_D)) frame.cameraMovement.x += cameraSpeed;

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        frame.removePart = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        return frame;
    }

    if (frame.mouseScreenPosition.y > 520.0f) {
        const auto controls = UiLayout::BuildControls(
            state,
            activationRegistry
        );
        for (const UiControl& control : controls) {
            if (!CheckCollisionPointRec(
                    frame.mouseScreenPosition.ToRayVec(),
                    control.bounds
                )) {
                continue;
            }
            if (control.type == UiControlType::Activation) {
                frame.activationAction = control.activationAction;
                return frame;
            }
            if (control.type == UiControlType::ToolbarPart) {
                frame.toolbarPart = control.toolbarPart;
                return frame;
            }
        }
    } else {
        frame.placePart = true;
    }

    return frame;
}
