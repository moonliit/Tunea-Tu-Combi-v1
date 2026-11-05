#ifndef RENDERING_GAME_UI_RENDERER_HPP
#define RENDERING_GAME_UI_RENDERER_HPP

#include "app/game/GameState.hpp"
#include "app/game/BuildSession.hpp"
#include "app/game/ActivationRegistry.hpp"

#include <raylib.h>

class GameUiRenderer {
public:
    void Render(
        int screenWidth,
        int screenHeight,
        GameState state,
        const BuildSession& buildSession,
        const EditorGrid& grid,
        const Camera2D& camera,
        const ActivationRegistry& activationRegistry
    ) const;
};

#endif
