#include "input/GameInput.hpp"
#include "app/game/EditorGrid.hpp"
#include "ui/UiLayout.hpp"
#include <raylib.h>

#include <algorithm>

namespace {
}

GameInputFrame GameInput::Capture(
    float deltaTime,
    GameState state,
    const ActivationRegistry& activationRegistry,
    const EditorGrid& grid,
    const Camera2D& camera,
    const std::optional<PartType>& selectedPartType,
    bool hasRepositionedPart,
    int screenWidth,
    int screenHeight
) const {
    GameInputFrame frame;
    frame.toggleMode = IsKeyPressed(KEY_ENTER);
    frame.clearSelection = IsKeyPressed(KEY_ESCAPE);
    frame.rotatePart = IsKeyPressed(KEY_R) || GetMouseWheelMove() != 0.0f;
    frame.mouseScreenPosition = {
        GetMousePosition().x,
        GetMousePosition().y
    };

    const bool cameraMode = state == GameState::View || state == GameState::Play;
    if (cameraMode) {
        const float cameraSpeed = 300.0f * deltaTime;
        if (IsKeyDown(KEY_W)) frame.cameraMovement.y -= cameraSpeed;
        if (IsKeyDown(KEY_S)) frame.cameraMovement.y += cameraSpeed;
        if (IsKeyDown(KEY_A)) frame.cameraMovement.x -= cameraSpeed;
        if (IsKeyDown(KEY_D)) frame.cameraMovement.x += cameraSpeed;
    }
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        frame.removePart = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            const Vector2 delta = GetMouseDelta();
            if (cameraMode && m_pendingWorldClick) {
                frame.cameraMovement = {-delta.x, -delta.y};
                frame.cameraDragging = true;
                return frame;
            }
            const Math::Vector2D mouseOffset = {
                frame.mouseScreenPosition.x - m_pressScreenPosition.x,
                frame.mouseScreenPosition.y - m_pressScreenPosition.y
            };
            const float distanceSquared =
                mouseOffset.x * mouseOffset.x + mouseOffset.y * mouseOffset.y;
            if (m_pendingWorldClick && m_canDrag && !m_dragging &&
                distanceSquared > 0.0f) {
                m_dragging = true;
                if (m_repositioningPart) {
                    frame.pickupPart = true;
                    frame.interactionCoord = m_pressGridCoord;
                } else if (cameraMode) {
                    frame.cameraMovement = {-mouseOffset.x, -mouseOffset.y};
                    frame.cameraDragging = true;
                }
            } else if (m_dragging) {
                if (!m_repositioningPart && cameraMode) {
                    frame.cameraMovement = {-delta.x, -delta.y};
                    frame.cameraDragging = true;
                }
            }
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (m_pendingWorldClick && !m_dragging) {
                if (m_repositioningPart) {
                    frame.rotatePlacedPart = true;
                } else {
                    frame.placePart = true;
                }
                frame.interactionCoord = m_pressGridCoord;
            } else if (m_repositioningPart && m_dragging) {
                const Vector2 mousePosition =
                    frame.mouseScreenPosition.ToRayVec();
                const auto controls = UiLayout::BuildControls(
                    state,
                    activationRegistry,
                    grid,
                    screenWidth,
                    screenHeight
                );
                const bool releasedOnTrash = std::any_of(
                    controls.begin(),
                    controls.end(),
                    [&mousePosition](const UiControl& control) {
                        return control.type == UiControlType::Editor &&
                            control.editorAction == EditorAction::ClearBuild &&
                            CheckCollisionPointRec(
                                mousePosition,
                                control.bounds
                            );
                    }
                );
                const Vector2 mouseWorld = GetScreenToWorld2D(
                    mousePosition,
                    camera
                );
                const bool releasedInsideGrid = grid.WorldToCoord({
                    mouseWorld.x,
                    mouseWorld.y
                }).has_value();
                if (releasedOnTrash || !releasedInsideGrid) {
                    frame.discardRepositionedPart = true;
                } else {
                    frame.placePart = true;
                }
            }
            m_pendingWorldClick = false;
            m_dragging = false;
            m_canDrag = false;
            m_repositioningPart = false;
        }
        return frame;
    }

    const auto controls = UiLayout::BuildControls(
        state,
        activationRegistry,
        grid,
        screenWidth,
        screenHeight
    );
    for (const UiControl& control : controls) {
        if (!control.enabled ||
            !CheckCollisionPointRec(
                frame.mouseScreenPosition.ToRayVec(),
                control.bounds
            )) {
            continue;
        }

        if (control.type == UiControlType::Activation) {
            frame.activationAction = control.activationAction;
            return frame;
        }
        if (control.type == UiControlType::Editor) {
            frame.editorAction = control.editorAction;
            return frame;
        }
        if (control.type == UiControlType::ToolbarPart) {
            frame.toolbarPart = control.toolbarPart;
            return frame;
        }
    }

    m_pendingWorldClick = cameraMode ||
        (state == GameState::Build && frame.mouseScreenPosition.y <= 520.0f);
    m_pressScreenPosition = frame.mouseScreenPosition;
    m_canDrag = false;
    m_repositioningPart = false;
    if (cameraMode) {
        m_canDrag = true;
    } else if (m_pendingWorldClick) {
        const Vector2 mouseWorld = GetScreenToWorld2D(
            frame.mouseScreenPosition.ToRayVec(),
            camera
        );
        const auto gridCoord = grid.WorldToCoord({
            mouseWorld.x,
            mouseWorld.y
        });
        if (gridCoord.has_value()) {
            m_pressGridCoord = gridCoord;
            const bool occupied = grid.GetPart(gridCoord.value()).has_value();
            m_repositioningPart =
                occupied &&
                !selectedPartType.has_value() &&
                !hasRepositionedPart;
            m_canDrag = cameraMode || m_repositioningPart;
        } else {
            m_pressGridCoord.reset();
            m_canDrag = true;
        }
    }
    m_dragging = false;
    frame.cameraDragging = false;

    return frame;
}
