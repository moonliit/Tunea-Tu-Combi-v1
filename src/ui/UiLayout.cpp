#include "ui/UiLayout.hpp"

#include "app/game/EditorGrid.hpp"
#include "ui/UiButton.hpp"

#include <algorithm>

namespace {

constexpr float ControlStartX = 20.0f;
constexpr float ControlWidth = 110.0f;
constexpr float ControlHeight = 50.0f;
constexpr float ControlGap = 10.0f;
constexpr float EditorControlStartX = 390.0f;
constexpr float EditorControlStartY = 20.0f;
constexpr float EditorControlWidth = 62.0f;
constexpr float EditorControlHeight = 32.0f;
constexpr float EditorControlGap = 6.0f;

UiControl MakeToolbarControl(
    int index,
    ToolbarPart part,
    std::string_view label,
    Color color,
    float controlWidth,
    float controlStartX,
    float controlStartY,
    bool enabled = true
) {
    return UiControl{
        .type = UiControlType::ToolbarPart,
        .bounds = {
            controlStartX + static_cast<float>(index) *
                (controlWidth + ControlGap),
            controlStartY,
            controlWidth,
            ControlHeight
        },
        .label = label,
        .color = color,
        .toolbarPart = part,
        .activationAction = std::nullopt,
        .editorAction = std::nullopt,
        .enabled = enabled,
        .active = false
    };
}

UiControl MakeEditorControl(
    int index,
    EditorAction action,
    std::string_view label,
    Color color,
    bool enabled,
    Rectangle bounds = Rectangle{0.0f, 0.0f, 0.0f, 0.0f},
    int labelFontSize = 12,
    int labelOffsetX = 8,
    int labelOffsetY = 15
) {
    if (bounds.width == 0.0f) {
        bounds = {
            EditorControlStartX + static_cast<float>(index) *
                (EditorControlWidth + EditorControlGap),
            EditorControlStartY,
            EditorControlWidth,
            EditorControlHeight
        };
    }
    return UiControl{
        .type = UiControlType::Editor,
        .bounds = bounds,
        .label = label,
        .color = enabled ? color : GRAY,
        .toolbarPart = std::nullopt,
        .activationAction = std::nullopt,
        .editorAction = action,
        .enabled = enabled,
        .active = false,
        .labelFontSize = labelFontSize,
        .labelOffsetX = labelOffsetX,
        .labelOffsetY = labelOffsetY
    };
}

}

std::vector<UiControl> UiLayout::BuildControls(
    GameState state,
    const ActivationRegistry& activationRegistry,
    const EditorGrid& grid,
    int screenWidth,
    int screenHeight
) {
    std::vector<UiControl> controls;
    const float controlStartY = screenHeight - UIBarHeight + 15.0f;
    if (state == GameState::Play) {
        const auto actions = activationRegistry.GetVisibleActions();
        const float actionGap = 6.0f;
        const float actionWidth = std::min(
            ControlWidth,
            (static_cast<float>(screenWidth) - 40.0f -
             actionGap * (static_cast<float>(actions.size()) - 1.0f)) /
                static_cast<float>(std::max<std::size_t>(1, actions.size()))
        );
        for (std::size_t index = 0; index < actions.size(); ++index) {
            const ActivationAction action = actions[index];
            const bool fan = IsFanAction(action);
            controls.push_back(UiControl{
                .type = UiControlType::Activation,
                .bounds = {
                    ControlStartX + static_cast<float>(index) *
                        (actionWidth + actionGap),
                    controlStartY,
                    actionWidth,
                    ControlHeight
                },
                .label = GetActivationLabel(action),
                .color = fan ? DARKGRAY : DARKBLUE,
                .toolbarPart = std::nullopt,
                .activationAction = action,
                .editorAction = std::nullopt,
                .enabled = true,
                .active = activationRegistry.IsActive(action),
                .labelFontSize = 12,
                .labelOffsetX = 8,
                .labelOffsetY = 33
            });
        }
    } else if (state == GameState::Build) {
        constexpr ToolbarPart parts[] = {
            ToolbarPart::DriverBox,
            ToolbarPart::SquareBlock,
            ToolbarPart::CircleWheel,
            ToolbarPart::TNTBarrel,
            ToolbarPart::Fan,
            ToolbarPart::WindSail
        };
        constexpr std::string_view labels[] = {
            "Driver Box",
            "Square Box",
            "Wheel",
            "TNT",
            "Fan",
            "Sail"
        };
        constexpr Color colors[] = {
            Color{106, 90, 205, 255},
            BLUE,
            GREEN,
            MAROON,
            ORANGE,
            YELLOW
        };

        const float toolbarWidth = std::min(
            110.0f,
            (static_cast<float>(screenWidth) - 40.0f -
             ControlGap * (static_cast<float>(std::size(parts)) - 1.0f)) /
                static_cast<float>(std::size(parts))
        );
        for (int index = 0; index < 6; ++index) {
            // disable driver box placement if one is already present
            const bool driverBoxEnabled = !(
                parts[index] == ToolbarPart::DriverBox &&
                grid.ContainsPartType(PartType::DriverBox)
            );
            controls.push_back(MakeToolbarControl(
                index,
                parts[index],
                labels[index],
                colors[index],
                toolbarWidth,
                ControlStartX,
                controlStartY,
                driverBoxEnabled
            ));
        }

        const float editorTotalWidth =
            4.0f * (EditorControlWidth + EditorControlGap) - EditorControlGap;
        const float editorStartX =
            screenWidth - editorTotalWidth - 120;
        const auto canMoveUp = grid.CanShiftContents({0, -1});
        const auto canMoveRight = grid.CanShiftContents({1, 0});
        const auto canMoveDown = grid.CanShiftContents({0, 1});
        const auto canMoveLeft = grid.CanShiftContents({-1, 0});
        for (int index = 0; index < 4; ++index) {
            const EditorAction actions[] = {
                EditorAction::MoveLeft,
                EditorAction::MoveDown,
                EditorAction::MoveUp,
                EditorAction::MoveRight
            };
            const std::string_view labels[] = {"Left", "Down", "Up", "Right"};
            const bool enabled[] = {canMoveLeft, canMoveDown, canMoveUp, canMoveRight};
            Rectangle bounds{
                editorStartX + static_cast<float>(index) *
                    (EditorControlWidth + EditorControlGap),
                EditorControlStartY,
                EditorControlWidth,
                EditorControlHeight
            };
            controls.push_back(MakeEditorControl(
                index, actions[index], labels[index], BLUE, enabled[index],
                bounds
            ));
        }
    }

    const Rectangle largeSideButton =
        UiButton::PlaySideButton(screenWidth, screenHeight);
    if (state == GameState::Build || state == GameState::Play) {
        controls.push_back(MakeEditorControl(
            static_cast<int>(controls.size()),
            state == GameState::Build
                ? EditorAction::StartPlay
                : EditorAction::ReturnToBuild,
            state == GameState::Build ? "PLAY" : "EDIT",
            DARKGREEN,
            state == GameState::Play || grid.ContainsPartType(PartType::DriverBox),
            largeSideButton,
            24,
            32,
            32
        ));
    }
    if (state == GameState::View) {
        controls.push_back(MakeEditorControl(
            0,
            EditorAction::ReturnToBuild,
            "EDIT",
            DARKGREEN,
            true,
            Rectangle{
                static_cast<float>(screenWidth - 140),
                static_cast<float>(screenHeight) / 2 + 155,
                110.0f,
                40.0f
            },
            14,
            17,
            13
        ));
        return controls;
    }
    if (state == GameState::Build) {
        controls.push_back(MakeEditorControl(
            6,
            EditorAction::ClearSelection,
            "DROP",
            DARKGRAY,
            true,
            Rectangle{
                static_cast<float>(screenWidth - 140),
                static_cast<float>(screenHeight) / 2 + 55,
                110.0f,
                40.0f
            },
            14,
            12,
            13
        ));
        controls.push_back(MakeEditorControl(
            7,
            EditorAction::ClearBuild,
            "BASURA",
            MAROON,
            true,
            Rectangle{
                20.0f,
                static_cast<float>(screenHeight) / 2 - 20,
                110.0f,
                40.0f
            },
            14,
            17,
            13
        ));
        controls.push_back(MakeEditorControl(
            8,
            EditorAction::EnterView,
            "VIEW",
            DARKBLUE,
            true,
            Rectangle{
                static_cast<float>(screenWidth - 140),
                static_cast<float>(screenHeight) / 2 + 155,
                110.0f,
                40.0f
            }
        ));
    }
    return controls;
}
