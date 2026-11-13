#include "rendering/GameUiRenderer.hpp"

#include "app/game/PartDefinition.hpp"
#include "math/Vector2D.hpp"
#include "raylib.h"
#include "ui/UiButton.hpp"
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
    DrawRectangle(
        0, static_cast<int>(screenHeight - UIBarHeight), screenWidth,
        static_cast<int>(UIBarHeight), LIGHTGRAY
    );
    DrawRectangleLines(
        0, static_cast<int>(screenHeight - UIBarHeight), screenWidth,
        static_cast<int>(UIBarHeight), GRAY
    );

    const auto controls = UiLayout::BuildControls(
        state,
        activationRegistry,
        grid,
        screenWidth,
        screenHeight
    );
    const auto& selectedPartType = buildSession.GetSelectedPartType();
    int activationIndex = 0;
    int toolbarIndex = 0;
    for (const UiControl& control : controls) {
        const bool containedPartToolbar = control.type == UiControlType::ToolbarPart &&
            control.toolbarPart == ToolbarPart::TNTBarrel;
        Color color = control.color;
        if (!control.enabled && control.type == UiControlType::ToolbarPart) {
            color = Color{50, 50, 50, 255};
        } else if (control.type == UiControlType::ToolbarPart &&
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
            control.enabled ? WHITE : GRAY
        );
        if (containedPartToolbar) {
            const std::string containerName =
                std::string(GetPartDefinition(PartType::SquareBlock).name);
            const std::string containerHint = std::format(
                "(inside {})",
                containerName
            );
            int hintFontSize = 7;
            const int maxHintWidth = static_cast<int>(control.bounds.width) - 8;
            while (hintFontSize > 5 &&
                   MeasureText(containerHint.c_str(), hintFontSize) > maxHintWidth) {
                --hintFontSize;
            }
            DrawText(
                containerHint.c_str(),
                static_cast<int>(control.bounds.x +
                    (control.bounds.width - MeasureText(containerHint.c_str(), hintFontSize)) / 2),
                static_cast<int>(control.bounds.y) + 31,
                hintFontSize,
                WHITE
            );
        }
        if (control.type == UiControlType::Activation) {
            ++activationIndex;
            const int hotkey = activationIndex == 10 ? 0 : activationIndex;
            DrawRectangle(
                static_cast<int>(control.bounds.x + control.bounds.width) - 23,
                static_cast<int>(control.bounds.y) + 5,
                18,
                18,
                BLACK
            );
            DrawText(
                TextFormat("%d", hotkey),
                static_cast<int>(control.bounds.x + control.bounds.width) - 18,
                static_cast<int>(control.bounds.y) + 7,
                12,
                WHITE
            );
        }
        if (control.type == UiControlType::ToolbarPart) {
            // Show the number key (1-6) that activates this editor part.
            const int hotkey = toolbarIndex + 1;
            DrawRectangle(
                static_cast<int>(control.bounds.x + control.bounds.width) - 23,
                static_cast<int>(control.bounds.y) + 5,
                18,
                18,
                BLACK
            );
            DrawText(
                TextFormat("%d", hotkey),
                static_cast<int>(control.bounds.x + control.bounds.width) - 18,
                static_cast<int>(control.bounds.y) + 7,
                12,
                WHITE
            );
            ++toolbarIndex;
        }
    }

    if (state == GameState::Build || state == GameState::Play) {
        const Rectangle modeButton = UiButton::PlaySideButton(screenWidth, screenHeight);
        const Rectangle enterBadge{
            modeButton.x + modeButton.width - 42.0f,
            modeButton.y + 4.0f,
            38.0f,
            16.0f
        };
        DrawRectangleRec(enterBadge, Fade(BLACK, 0.65f));
        DrawText(
            "ENTER",
            static_cast<int>(enterBadge.x + 3.0f),
            static_cast<int>(enterBadge.y + 2.0f),
            11,
            WHITE
        );
    }

    if (state == GameState::Build) {
        const auto actions = ActivationRegistry::GetActionsForGrid(grid);
        if (!actions.empty()) {
            const int columns = 3;
            const int rows = (static_cast<int>(actions.size()) + columns - 1) / columns;
            const int panelX = screenWidth - 196;
            const int panelY = 60;
            const int panelHeight = 24 + rows * 22;
            DrawRectangle(panelX, panelY, 184, panelHeight, Fade(DARKGRAY, 0.82f));
            DrawText("EN PLAY (teclas 1-0)", panelX + 8, panelY + 5, 12, WHITE);
            for (std::size_t index = 0; index < actions.size(); ++index) {
                const int column = static_cast<int>(index) % columns;
                const int row = static_cast<int>(index) / columns;
                const int x = panelX + 8 + column * 58;
                const int y = panelY + 25 + row * 22;
                const int hotkey = index == 9 ? 0 : static_cast<int>(index) + 1;
                DrawText(
                    TextFormat("%d %s", hotkey, GetActivationLabel(actions[index])),
                    x,
                    y,
                    12,
                    IsFanAction(actions[index]) ? ORANGE : GOLD
                );
            }
        }
    }

    std::string modeText, modeControlsText;
    Color modeTextColor;

    if (state == GameState::Build) {
        modeText = "MODE: EDIT";
        modeControlsText = "(Left Click: Place, Right Click: Delete, ENTER: Play)";
        modeTextColor = DARKGREEN;
    } else if (state == GameState::View) {
        modeText = "MODE: VIEW";
        modeControlsText = "(Drag to move camera, ESC: Edit)";
        modeTextColor = DARKPURPLE;
    } else {
        modeText = "MODE: PLAY";
        modeControlsText = "(Press ENTER to Edit)";
        modeTextColor = MAROON;
    }

    const int modeTextFontSize = 30;
    const int modeControlsFontSize = 18;
    const int modeTextX = 20;
    const int modeTextY = 20;
    const int verticalPadding = 4;
    const int modeControlsTextY = modeTextY + verticalPadding + modeTextFontSize;

    DrawText(
        modeText.c_str(),
        modeTextX,
        modeTextY,
        modeTextFontSize,
        modeTextColor
    );
    DrawText(
        modeControlsText.c_str(),
        modeTextX,
        modeControlsTextY,
        modeControlsFontSize,
        modeTextColor
    );
}
