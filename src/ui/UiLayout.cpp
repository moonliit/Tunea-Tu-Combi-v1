#include "ui/UiLayout.hpp"

namespace {

constexpr float ControlStartX = 20.0f;
constexpr float ControlStartY = 535.0f;
constexpr float ControlWidth = 110.0f;
constexpr float ControlHeight = 50.0f;
constexpr float ControlGap = 10.0f;

UiControl MakeToolbarControl(
    int index,
    ToolbarPart part,
    std::string_view label,
    Color color
) {
    return UiControl{
        .type = UiControlType::ToolbarPart,
        .bounds = {
            ControlStartX + static_cast<float>(index) *
                (ControlWidth + ControlGap),
            ControlStartY,
            ControlWidth,
            ControlHeight
        },
        .label = label,
        .color = color,
        .toolbarPart = part,
        .activationAction = std::nullopt,
        .active = false
    };
}

}

std::vector<UiControl> UiLayout::BuildControls(
    AppState state,
    const ActivationRegistry& activationRegistry
) {
    std::vector<UiControl> controls;
    if (state == AppState::Play) {
        const auto actions = activationRegistry.GetVisibleActions();
        controls.reserve(actions.size());
        for (std::size_t index = 0; index < actions.size(); ++index) {
            const ActivationAction action = actions[index];
            const bool fan = IsFanAction(action);
            controls.push_back(UiControl{
                .type = UiControlType::Activation,
                .bounds = {
                    ControlStartX + static_cast<float>(index) *
                        (ControlWidth + ControlGap),
                    ControlStartY,
                    ControlWidth,
                    ControlHeight
                },
                .label = GetActivationLabel(action),
                .color = fan ? DARKGRAY : DARKBLUE,
                .toolbarPart = std::nullopt,
                .activationAction = action,
                .active = activationRegistry.IsActive(action)
            });
        }
        return controls;
    }

    constexpr ToolbarPart parts[] = {
        ToolbarPart::SquareBlock,
        ToolbarPart::CircleWheel,
        ToolbarPart::TNTBarrel,
        ToolbarPart::Fan,
        ToolbarPart::WindSail
    };
    constexpr std::string_view labels[] = {
        "Square",
        "Wheel",
        "TNT",
        "Fan",
        "Sail"
    };
    constexpr Color colors[] = {
        BLUE,
        GREEN,
        MAROON,
        ORANGE,
        YELLOW
    };

    controls.reserve(5);
    for (int index = 0; index < 5; ++index) {
        controls.push_back(MakeToolbarControl(
            index,
            parts[index],
            labels[index],
            colors[index]
        ));
    }
    return controls;
}
