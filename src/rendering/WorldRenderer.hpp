#ifndef RENDERING_WORLD_RENDERER_HPP
#define RENDERING_WORLD_RENDERER_HPP

#include "app/game/GameState.hpp"
#include "app/game/EditorGrid.hpp"
#include "app/game/PlaySession.hpp"
#include "rendering/Background.hpp"
#include "rendering/PartRenderer.hpp"
#include "rendering/TextureLibrary.hpp"

#include <raylib.h>
#include <optional>

class WorldRenderer {
private:
    const TextureLibrary& m_textures;
    PartRenderer m_partRenderer;
    BackgroundId m_background;

public:
    WorldRenderer(const TextureLibrary& textures, BackgroundId background):
        m_textures(textures),
        m_partRenderer(textures),
        m_background(background) {}

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
