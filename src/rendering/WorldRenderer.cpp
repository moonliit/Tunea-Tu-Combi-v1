#include "rendering/WorldRenderer.hpp"
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
    const Math::TerrainMesh& mesh,
    const Math::TerrainVisualDefinition& visual
) {
    if (mesh.vertices.size() < 3 || mesh.indices.size() < 3) {
        return;
    }
    const Color fill = ToRaylibColor(
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
    for (size_t index = 0; index + 2 < mesh.indices.size(); index += 3) {
        // invert winding order
        DrawTriangle(
            mesh.vertices[mesh.indices[index]].ToRayVec(),
            mesh.vertices[mesh.indices[index + 2]].ToRayVec(),
            mesh.vertices[mesh.indices[index + 1]].ToRayVec(),
            fill
        );
    }
    for (size_t index = 0; index + 1 < mesh.top.size(); ++index) {
        DrawLineV(
            mesh.top[index].ToRayVec(),
            mesh.top[index + 1].ToRayVec(),
            outline
        );
    }
    if (mesh.closed && mesh.top.size() >= 3) {
        DrawLineV(
            mesh.top.back().ToRayVec(),
            mesh.top.front().ToRayVec(),
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
        RenderTerrainMesh(mesh, playSession.GetTerrainVisual());
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

    }

    EndMode2D();
}
