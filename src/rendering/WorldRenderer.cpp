#include "rendering/WorldRenderer.hpp"
#include <cassert>

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
