#ifndef RENDERING_WORLD_RENDERER_HPP
#define RENDERING_WORLD_RENDERER_HPP

#include "app/game/GameState.hpp"
#include "app/game/EditorGrid.hpp"
#include "app/game/PlaySession.hpp"
#include "rendering/PartRenderer.hpp"

#include <raylib.h>
#include <optional>

class WorldRenderer {
private:
    PartRenderer m_partRenderer;

public:
    void Render(
        const Camera2D& camera,
        GameState state,
        const EditorGrid& grid,
        const PlaySession& playSession,
        const std::optional<PartType>& selectedPartType,
        PartOrientation selectedOrientation,
        const std::optional<PartType>& repositionedPartType,
        PartOrientation repositionedOrientation
    ) const;
};

#endif
