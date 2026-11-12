#include "rendering/WorldRenderer.hpp"
#include "rlgl.h"
#include <cassert>
#include <cmath>

namespace {

Color ToRaylibColor(
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue,
    std::uint8_t alpha
) {
    return Color{red, green, blue, alpha};
}

void RenderTerrainMesh(
    const TextureLibrary& textures,
    const Math::TerrainMesh& mesh,
    const Math::TerrainVisualDefinition& visual
) {
    if (mesh.vertices.size() < 3 || mesh.indices.size() < 3) {
        return;
    }
    Texture2D terrainTexture = textures.GetTerrainTexture();
    constexpr float textureScale = 0.005f;

    const Color tint = ToRaylibColor(
        visual.fillR,
        visual.fillG,
        visual.fillB,
        visual.fillA
    );
    const Color outline = ToRaylibColor(
        visual.outlineR,
        visual.outlineG,
        visual.outlineB,
        visual.outlineA
    );

    rlBegin(RL_TRIANGLES);
    rlSetTexture(terrainTexture.id);

    rlColor4ub(tint.r, tint.g, tint.b, tint.a);
    for (size_t index = 0; index + 2 < mesh.indices.size(); index += 3) {
        size_t i0 = mesh.indices[index];
        size_t i1 = mesh.indices[index + 1];
        size_t i2 = mesh.indices[index + 2];

        Vector2 v0 = mesh.vertices[i0].ToRayVec();
        Vector2 v1 = mesh.vertices[i1].ToRayVec();
        Vector2 v2 = mesh.vertices[i2].ToRayVec();

        Vector2 uv0 = { v0.x * textureScale, v0.y * textureScale };
        Vector2 uv1 = { v1.x * textureScale, v1.y * textureScale };
        Vector2 uv2 = { v2.x * textureScale, v2.y * textureScale };

        // inverted winding order
        rlTexCoord2f(uv0.x, uv0.y);
        rlVertex2f(v0.x, v0.y);

        rlTexCoord2f(uv2.x, uv2.y);
        rlVertex2f(v2.x, v2.y);

        rlTexCoord2f(uv1.x, uv1.y);
        rlVertex2f(v1.x, v1.y);
    }
    rlEnd();
    rlSetTexture(0);

    float lineWidth = 4.0f;
    for (size_t index = 0; index + 1 < mesh.top.size(); ++index) {
        DrawLineEx(
            mesh.top[index].ToRayVec(),
            mesh.top[index + 1].ToRayVec(),
            lineWidth,
            outline
        );
    }
    if (mesh.closed && mesh.top.size() >= 3) {
        DrawLineEx(
            mesh.top.back().ToRayVec(),
            mesh.top.front().ToRayVec(),
            lineWidth,
            outline
        );
    }
}

void RenderBackground(
    const Texture2D& background,
    const Camera2D& camera,
    float originX
) {
    const float screenWidth = static_cast<float>(GetScreenWidth());
    const float screenHeight = static_cast<float>(GetScreenHeight());
    const float tileWidth = static_cast<float>(background.width) *
        screenHeight / static_cast<float>(background.height);
    const float scroll =
        (camera.target.x - originX) * camera.zoom * BackgroundParallaxFactor;

    const int firstTile = static_cast<int>(std::floor(scroll / tileWidth));
    const int lastTile =
        static_cast<int>(std::floor((scroll + screenWidth) / tileWidth));
    for (int tile = firstTile; tile <= lastTile; ++tile) {
        // integer edges so neighbouring tiles never leave a gap
        const float left = std::floor(static_cast<float>(tile) * tileWidth - scroll);
        const float right =
            std::floor(static_cast<float>(tile + 1) * tileWidth - scroll);
        const bool mirrored = (tile % 2) != 0;
        const float sourceWidth = static_cast<float>(background.width);
        DrawTexturePro(
            background,
            Rectangle{
                0.0f,
                0.0f,
                mirrored ? -sourceWidth : sourceWidth,
                static_cast<float>(background.height)
            },
            Rectangle{left, 0.0f, right - left, screenHeight},
            Vector2{0.0f, 0.0f},
            0.0f,
            WHITE
        );
    }
}

}

void WorldRenderer::Render(
    const Camera2D& camera,
    GameState state,
    const EditorGrid& grid,
    const PlaySession& playSession,
    const std::optional<PartType>& selectedPartType,
    PartOrientation selectedOrientation,
    const std::optional<PartType>& repositionedPartType,
    PartOrientation repositionedOrientation
) const {
    RenderBackground(
        m_textures.GetBackground(m_background),
        camera,
        m_backgroundOriginX
    );
    BeginMode2D(camera);

    for (const Math::TerrainMesh& mesh : playSession.GetTerrainMeshes()) {
        RenderTerrainMesh(m_textures, mesh, playSession.GetTerrainVisual());
    }

    if (state == GameState::Build || state == GameState::View) {
        grid.DrawSelf();

        for (const Part* part : grid.GetActiveParts()) {
            if (part->HasContainedPart()) {
                m_partRenderer.RenderContained(
                    *part->GetContainedPart(),
                    grid,
                    part->GetGridCoord()
                );
            }
            m_partRenderer.Render(part->GetRenderState(grid));
        }
        for (const Part* part : grid.GetActiveParts()) {
            m_partRenderer.RenderConnectors(*part, grid);
        }

        const auto& ghostPartType = repositionedPartType.has_value()
            ? repositionedPartType
            : selectedPartType;
        const PartOrientation ghostOrientation = repositionedPartType.has_value()
            ? repositionedOrientation
            : selectedOrientation;
        if (state == GameState::Build && ghostPartType.has_value()) {
            const Vector2 mouseScreen = GetMousePosition();
            if (mouseScreen.y <= 520.0f) {
                const Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera);
                m_partRenderer.RenderGhostAtPosition(
                    ghostPartType.value(),
                    Math::Vector2D{mouseWorld.x, mouseWorld.y},
                    grid.GetCellSize(),
                    ghostOrientation
                );
                const auto gridCoord = grid.WorldToCoord(
                    Math::Vector2D{mouseWorld.x, mouseWorld.y}
                );
                if (gridCoord.has_value()) {
                    const Math::Vector2D cellCenter =
                        grid.CoordToWorld(gridCoord.value());
                    const float cellSize = grid.GetCellSize();
                    DrawRectangle(
                        static_cast<int>(cellCenter.x - cellSize / 2.0f),
                        static_cast<int>(cellCenter.y - cellSize / 2.0f),
                        static_cast<int>(cellSize),
                        static_cast<int>(cellSize),
                        Fade(WHITE, 0.45f)
                    );
                    m_partRenderer.RenderGhost(
                        ghostPartType.value(),
                        gridCoord.value(),
                        grid,
                        ghostOrientation
                    );
                }
            }
        }
    } else if (state == GameState::Play) {
        const EditorGrid& runtimeGrid = playSession.GetRuntimeGrid();
        for (const Part* part : runtimeGrid.GetActiveParts()) {
            if (part->HasContainedPart()) {
                m_partRenderer.RenderContained(
                    *part->GetContainedPart(),
                    runtimeGrid,
                    part->GetGridCoord()
                );
            }
            m_partRenderer.Render(part->GetRenderState(runtimeGrid));
        }

    }

    EndMode2D();
}
