#ifndef UI_UI_LAYOUT_HPP
#define UI_UI_LAYOUT_HPP

#include "game/AppState.hpp"
#include "game/ActivationRegistry.hpp"
#include "ui/UiControl.hpp"

#include <vector>

class UiLayout {
public:
    static std::vector<UiControl> BuildControls(
        AppState state,
        const ActivationRegistry& activationRegistry
    );
};

#endif
