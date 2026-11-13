#ifndef UI_UI_CONTROL_HPP
#define UI_UI_CONTROL_HPP

#include "app/game/ActivationRegistry.hpp"
#include <raylib.h>
#include <optional>
#include <string_view>

constexpr float UIBarHeight = 80.0f;

enum class UiControlType {
    ToolbarPart,
    Activation,
    Editor
};

enum class EditorAction {
    MoveUp,
    MoveRight,
    MoveDown,
    MoveLeft,
    ClearBuild,
    StartPlay,
    ClearSelection,
    EnterView,
    ReturnToBuild
};

enum class ToolbarPart {
    SquareBlock,
    CircleWheel,
    TNTBarrel,
    Fan,
    WindSail,
    Driver,
    DriverBox
};

struct UiControl {
    UiControlType type;
    Rectangle bounds;
    std::string_view label;
    Color color;
    std::optional<ToolbarPart> toolbarPart;
    std::optional<ActivationAction> activationAction;
    std::optional<EditorAction> editorAction;
    bool enabled = true;
    bool active = false;
    int labelFontSize = 12;
    int labelOffsetX = 8;
    int labelOffsetY = 15;
};

#endif
