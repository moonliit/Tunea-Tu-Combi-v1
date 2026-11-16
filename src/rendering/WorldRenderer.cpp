#include "rendering/WorldRenderer.hpp"
#include "rlgl.h"
#include "math/Spline.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>

namespace {

float DistanceToPolyline(const Math::Vector2D& point,
                         const std::vector<Math::Vector2D>& polyline) {
    float nearest = INFINITY;
    for (size_t i = 1; i < polyline.size(); ++i) {
        const float ax = float(polyline[i-1].x);
        const float ay = float(polyline[i-1].y);
        const float dx = float(polyline[i].x) - ax;
        const float dy = float(polyline[i].y) - ay;
        const float lengthSquared = dx*dx + dy*dy;
        const float t = lengthSquared > 0.0f
            ? std::clamp(((float(point.x)-ax)*dx + (float(point.y)-ay)*dy)
                / lengthSquared, 0.0f, 1.0f)
            : 0.0f;
        nearest = std::min(nearest,
            std::hypot(float(point.x)-ax-t*dx, float(point.y)-ay-t*dy));
    }
    return nearest;
}

struct SamplingComparison {
    size_t fixedSegments = 0;
    size_t adaptiveSegments = 0;
    float fixedError = 0.0f;
    float adaptiveError = 0.0f;
};

SamplingComparison CompareSampling(const Math::TerrainDefinition& terrain) {
    SamplingComparison comparison;
    for (const auto& polygon : terrain.polygons) {
        for (const auto& segment : polygon.segments) {
            if (segment.type != Math::TerrainSegmentType::CatmullRom ||
                segment.points.size() < 2) continue;
            const auto fixed = Math::SampleCatmullRom(
                segment.points, std::max(1, segment.resolution));
            const auto adaptive = Math::SampleCatmullRomAdaptive(
                segment.points, Math::TerrainCurveTolerance);
            comparison.fixedSegments += fixed.size()-1;
            comparison.adaptiveSegments += adaptive.size()-1;
            for (size_t i = 0; i+1 < segment.points.size(); ++i) {
                const auto p0 = segment.points[i == 0 ? i : i-1];
                const auto p1 = segment.points[i];
                const auto p2 = segment.points[i+1];
                const auto p3 = segment.points[
                    i+2 < segment.points.size() ? i+2 : i+1];
                for (int sample = 1; sample < 64; ++sample) {
                    const auto reference = Math::EvaluateCatmullRom(
                        p0,p1,p2,p3,float(sample)/64.0f);
                    comparison.fixedError = std::max(comparison.fixedError,
                        DistanceToPolyline(reference, fixed));
                    comparison.adaptiveError = std::max(comparison.adaptiveError,
                        DistanceToPolyline(reference, adaptive));
                }
            }
        }
    }
    return comparison;
}

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

    rlDrawRenderBatchActive();
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
    PartOrientation repositionedOrientation,
    int screenHeight,
    const std::function<void()>& beforePlayParts,
    bool showParts,
    bool graphicMode
) const {
    RenderBackground(
        m_textures.GetBackground(m_background),
        camera,
        m_backgroundOriginX
    );
    BeginMode2D(camera);

    for (const Math::TerrainMesh& mesh : playSession.GetTerrainMeshes()) {
        RenderTerrainMesh(m_textures, mesh, playSession.GetTerrainVisual());
        if (graphicMode) {
            for (size_t i = 0; i+2 < mesh.indices.size(); i += 3) {
                DrawTriangleLines(
                    mesh.vertices[mesh.indices[i]].ToRayVec(),
                    mesh.vertices[mesh.indices[i+1]].ToRayVec(),
                    mesh.vertices[mesh.indices[i+2]].ToRayVec(),
                    Color{65,190,255,255});
            }
            for (size_t i = 1; i < mesh.top.size(); ++i) {
                DrawLineEx(mesh.top[i-1].ToRayVec(), mesh.top[i].ToRayVec(),
                    2.0f, SKYBLUE);
            }
        }
    }
    if (graphicMode) {
        for (const auto& polygon : playSession.GetTerrainDefinition().polygons) {
            for (const auto& segment : polygon.segments) {
                if (segment.type != Math::TerrainSegmentType::CatmullRom) continue;
                const auto fixed = Math::SampleCatmullRom(
                    segment.points, std::max(1, segment.resolution));
                for (size_t i = 1; i < fixed.size(); ++i) {
                    DrawLineEx(fixed[i-1].ToRayVec(), fixed[i].ToRayVec(),
                        4.0f, ORANGE);
                }
                for (const auto& point : segment.points) {
                    DrawCircleV(point.ToRayVec(), 5.0f, GOLD);
                }
            }
        }
    }

    for (const auto& obstacle : playSession.GetTntObstacles()) {
        m_partRenderer.RenderStaticTnt(
            obstacle.position, grid.GetCellSize()
        );
    }

    if (!showParts) {
        EndMode2D();
        return;
    }

    if (state == GameState::Build || state == GameState::View) {
        grid.DrawSelf();
        if (beforePlayParts) {
            beforePlayParts();
        }

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
            if (mouseScreen.y <= static_cast<float>(screenHeight - UIBarHeight)) {
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
    } else if (state == GameState::Play || state == GameState::Results) {
        if (beforePlayParts) {
            beforePlayParts();
        }
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
        for (const auto& ejected : playSession.GetEjectedParts()) {
            if (ejected) {
                m_partRenderer.Render(ejected->GetRenderState(runtimeGrid));
            }
        }
    }

    EndMode2D();
    if (graphicMode && showParts) {
        const auto comparison = CompareSampling(playSession.GetTerrainDefinition());
        const bool hasCurves = comparison.fixedSegments > 0;
        constexpr int fontSize = 13;
        constexpr int textX = 18;
        constexpr int textY = 148;
        const char* cyanLabel = "Celeste: ";
        const char* cyanDetail = "triángulos del terreno";
        const char* orangeLabel = "Naranja: ";
        const char* orangeDetail = hasCurves
            ? "curvas Catmull-Rom"
            : "curvas (pero este nivel no cuenta con curvas Catmull-Rom)";
        const std::string cyanLine = std::string(cyanLabel) + cyanDetail;
        const std::string orangeLine = std::string(orangeLabel) + orangeDetail;
        const std::string fixedLine = hasCurves
            ? TextFormat("Fijo: %d segmentos, error ~%.2f u",
                static_cast<int>(comparison.fixedSegments), comparison.fixedError)
            : "";
        const std::string adaptiveLine = hasCurves
            ? TextFormat("Adaptativo: %d segmentos, error ~%.2f u",
                static_cast<int>(comparison.adaptiveSegments), comparison.adaptiveError)
            : "";
        const float textWidth = std::max({
            MeasureTextFont(cyanLine.c_str(), fontSize),
            MeasureTextFont(orangeLine.c_str(), fontSize),
            MeasureTextFont(fixedLine.c_str(), fontSize),
            MeasureTextFont(adaptiveLine.c_str(), fontSize)
        });
        DrawRectangle(10, 140, static_cast<int>(std::ceil(textWidth)) + 16,
            hasCurves ? 96 : 53, Fade(BLACK, 0.78f));
        DrawTextFont(cyanLabel, textX, textY, fontSize, SKYBLUE);
        DrawTextFont(cyanDetail,
            textX + static_cast<int>(MeasureTextFont(cyanLabel, fontSize)),
            textY, fontSize, WHITE);
        DrawTextFont(orangeLabel, textX, textY + 21, fontSize, ORANGE);
        DrawTextFont(orangeDetail,
            textX + static_cast<int>(MeasureTextFont(orangeLabel, fontSize)),
            textY + 21, fontSize, WHITE);
        if (hasCurves) {
            DrawTextFont(fixedLine.c_str(), textX, textY + 42, fontSize, ORANGE);
            DrawTextFont(adaptiveLine.c_str(), textX, textY + 63,
                fontSize, SKYBLUE);
        }
    }
}

void WorldRenderer::RenderGhostPlayback(
    const std::vector<GhostRenderState>& states
) const {
    for (const GhostRenderState& state : states) {
        m_partRenderer.RenderPlaybackGhost(
            state.type,
            state.position,
            state.rotation,
            state.attachmentRotation,
            state.cellSize
        );
    }
}
