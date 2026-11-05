#ifndef UI_UI_LAYOUT_HPP
#define UI_UI_LAYOUT_HPP

#include "app/game/GameState.hpp"
#include "app/game/ActivationRegistry.hpp"
#include "ui/UiControl.hpp"

#include <vector>

class UiLayout {
public:
    static std::vector<UiControl> BuildControls(
        GameState state,
        const ActivationRegistry& activationRegistry,
        const EditorGrid& grid,
        int screenWidth,
        int screenHeight
    );
};

#endif
