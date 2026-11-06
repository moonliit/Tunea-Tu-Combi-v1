#include "rendering/GameUiRenderer.hpp"

#include "math/Vector2D.hpp"
#include "ui/UiLayout.hpp"

#include <format>

void GameUiRenderer::Render(
    int screenWidth,
    int screenHeight,
    GameState state,
    const BuildSession& buildSession,
    const EditorGrid& grid,
    const Camera2D& camera,
    const ActivationRegistry& activationRegistry
) const {
    DrawRectangle(0, 520, screenWidth, 80, LIGHTGRAY);
    DrawRectangleLines(0, 520, screenWidth, 80, GRAY);

    const auto controls = UiLayout::BuildControls(
        state,
        activationRegistry,
        grid,
        screenWidth,
        screenHeight
    );
    const auto& selectedPartType = buildSession.GetSelectedPartType();
    for (const UiControl& control : controls) {
        Color color = control.color;
        if (control.type == UiControlType::ToolbarPart &&
            selectedPartType.has_value() &&
            control.toolbarPart.has_value() &&
            static_cast<PartType>(control.toolbarPart.value()) ==
                selectedPartType.value()) {
            color = control.color;
        } else if (control.type == UiControlType::ToolbarPart) {
            color = DARKGRAY;
        } else if (control.type == UiControlType::Activation &&
                   control.active) {
            color = IsFanAction(control.activationAction.value())
                ? DARKGREEN
                : RED;
        } else if (control.type == UiControlType::Editor) {
            color = control.color;
        }
        DrawRectangle(
            static_cast<int>(control.bounds.x),
            static_cast<int>(control.bounds.y),
            static_cast<int>(control.bounds.width),
            static_cast<int>(control.bounds.height),
            color
        );
        DrawText(
            control.label.data(),
            static_cast<int>(control.bounds.x) + control.labelOffsetX,
            static_cast<int>(control.bounds.y) + control.labelOffsetY,
            control.type == UiControlType::ToolbarPart
                ? 14
                : control.labelFontSize,
            WHITE
        );
    }

    const std::string modeText = state == GameState::Build
        ? "MODE: EDIT (Left Click: Place, Right Click: Delete, ENTER: Play)"
        : state == GameState::View
        ? "MODE: VIEW (Drag to move camera, ESC: Edit)"
        : "MODE: PLAY (Press ENTER to Edit)";
    DrawText(
        modeText.c_str(),
        20,
        20,
        18,
        state == GameState::Build ? DARKGREEN : MAROON
    );

    const Vector2 mouseScreen = GetMousePosition();
    const Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera);
    const auto coord = grid.WorldToCoord({mouseWorld.x, mouseWorld.y});
    const std::string coordText = coord.has_value()
        ? std::format("{}, {}", coord->x, coord->y)
        : "none";
    const std::string worldText = coord.has_value()
        ? std::format(
            "{}, {}",
            static_cast<float>(grid.CoordToWorld(coord.value()).x),
            static_cast<float>(grid.CoordToWorld(coord.value()).y)
        )
        : "none";

    DrawText(
        std::format(
            "({}, {})\n{}\n{}\nActive parts: {}",
            mouseWorld.x,
            mouseWorld.y,
            coordText,
            worldText,
            grid.GetPartCount()
        ).c_str(),
        20,
        45,
        14,
        DARKGRAY
    );
}
