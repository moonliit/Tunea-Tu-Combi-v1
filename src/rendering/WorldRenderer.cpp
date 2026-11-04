#include "rendering/WorldRenderer.hpp"

void WorldRenderer::Render(
    const Camera2D& camera,
    AppState state,
    const EditorGrid& grid,
    const PlaySession& playSession,
    const std::optional<PartType>& selectedPartType,
    PartOrientation selectedOrientation
) const {
    BeginMode2D(camera);

    const std::vector<Math::Vector2D>& terrainVertices =
        playSession.GetTerrainVertices();
    for (size_t i = 0; i + 1 < terrainVertices.size(); ++i) {
        DrawLineV(
            terrainVertices[i].ToRayVec(),
            terrainVertices[i + 1].ToRayVec(),
            DARKGRAY
        );
    }

    if (state == AppState::Build) {
        grid.DrawSelf();

        for (const Part* part : grid.GetActiveParts()) {
            m_partRenderer.Render(part->GetRenderState(grid));
        }
        for (const Part* part : grid.GetActiveParts()) {
            m_partRenderer.RenderConnectors(*part, grid);
        }

        if (selectedPartType.has_value()) {
            const Vector2 mouseScreen = GetMousePosition();
            if (mouseScreen.y <= 520.0f) {
                const Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera);
                const auto gridCoord = grid.WorldToCoord(
                    Math::Vector2D{mouseWorld.x, mouseWorld.y}
                );
                if (gridCoord.has_value()) {
                    m_partRenderer.RenderGhost(
                        selectedPartType.value(),
                        gridCoord.value(),
                        grid,
                        selectedOrientation
                    );
                }
            }
        }
    } else if (state == AppState::Play) {
        for (const Part* part : grid.GetActiveParts()) {
            m_partRenderer.Render(part->GetRenderState(grid));
        }
    }

    EndMode2D();
}
