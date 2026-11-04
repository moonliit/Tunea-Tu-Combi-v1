#ifndef RENDERING_WORLD_RENDERER_HPP
#define RENDERING_WORLD_RENDERER_HPP

#include "game/AppState.hpp"
#include "game/EditorGrid.hpp"
#include "game/PlaySession.hpp"
#include "rendering/PartRenderer.hpp"

#include <raylib.h>
#include <optional>

class WorldRenderer {
private:
    PartRenderer m_partRenderer;

public:
    void Render(
        const Camera2D& camera,
        AppState state,
        const EditorGrid& grid,
        const PlaySession& playSession,
        const std::optional<PartType>& selectedPartType,
        PartOrientation selectedOrientation
    ) const;
};

#endif
