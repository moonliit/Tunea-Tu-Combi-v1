#ifndef UI_UI_CONTROL_HPP
#define UI_UI_CONTROL_HPP

#include "game/ActivationRegistry.hpp"

#include <raylib.h>

#include <optional>
#include <string_view>

enum class UiControlType {
    ToolbarPart,
    Activation
};

enum class ToolbarPart {
    SquareBlock,
    CircleWheel,
    TNTBarrel,
    Fan,
    WindSail
};

struct UiControl {
    UiControlType type;
    Rectangle bounds;
    std::string_view label;
    Color color;
    std::optional<ToolbarPart> toolbarPart;
    std::optional<ActivationAction> activationAction;
    bool active = false;
};

#endif
