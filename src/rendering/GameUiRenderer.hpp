#ifndef RENDERING_GAME_UI_RENDERER_HPP
#define RENDERING_GAME_UI_RENDERER_HPP

#include "game/AppState.hpp"
#include "game/BuildSession.hpp"
#include "game/ActivationRegistry.hpp"

#include <raylib.h>

class GameUiRenderer {
public:
    void Render(
        int screenWidth,
        int screenHeight,
        AppState state,
        const BuildSession& buildSession,
        const EditorGrid& grid,
        const Camera2D& camera,
        const ActivationRegistry& activationRegistry
    ) const;
};

#endif
