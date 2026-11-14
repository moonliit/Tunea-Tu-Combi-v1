#include "rendering/PartRenderer.hpp"

#include "app/game/EditorGrid.hpp"
#include "app/game/PartDefinition.hpp"
#include "raylib.h"

#include <cmath>

namespace {

// reddish tint while a fan or sail is active
constexpr Color ActiveTint{255, 110, 110, 255};
constexpr float GhostAlpha = 0.5f;

Vector2 RotateLocalOffset(Vector2 offset, float rotation) {
    return Vector2{
        offset.x * std::cos(rotation) - offset.y * std::sin(rotation),
        offset.x * std::sin(rotation) + offset.y * std::cos(rotation)
    };
}

// from the cell anchor to the wheel center
Vector2 WheelCenterOffset(float cellSize, float attachmentRotation) {
    const float radiusScale =
        GetPartDefinition(PartType::CircleWheel).radiusScale;
    return RotateLocalOffset(
        Vector2{0.0f, cellSize * (0.5f - radiusScale)},
        attachmentRotation
    );
}

// square texture centered on `center`
void DrawSprite(
    const Texture2D& texture,
    Vector2 center,
    float size,
    float rotation,
    Color tint
) {
    DrawTexturePro(
        texture,
        Rectangle{
            0.0f,
            0.0f,
            static_cast<float>(texture.width),
            static_cast<float>(texture.height)
        },
        Rectangle{center.x, center.y, size, size},
        Vector2{size / 2.0f, size / 2.0f},
        rotation * RAD2DEG,
        tint
    );
}

// position is the part center (the disc center for wheels)
// attachmentRotation only turns the wheel mount
void DrawPart(
    const TextureLibrary& textures,
    PartType type,
    Vector2 position,
    float rotation,
    float attachmentRotation,
    float size,
    Color tint
) {
    if (type != PartType::CircleWheel) {
        DrawSprite(textures.GetPartTexture(type), position, size, rotation, tint);
        return;
    }

    const float radiusScale = GetPartDefinition(PartType::CircleWheel).radiusScale;
    const Vector2 offset = WheelCenterOffset(size, attachmentRotation);
    const Vector2 anchor{position.x - offset.x, position.y - offset.y};

    DrawSprite(textures.GetWheelMount(), anchor, size, attachmentRotation, tint);
    DrawSprite(
        textures.GetPartTexture(type),
        position,
        size * radiusScale * 2.0f,
        rotation,
        tint
    );
}

Math::GridCoord ConnectorOffset(ConnectorDirection direction) {
    switch (direction) {
        case ConnectorDirection::Up: return {0, -1};
        case ConnectorDirection::Right: return {1, 0};
        case ConnectorDirection::Down: return {0, 1};
        case ConnectorDirection::Left: return {-1, 0};
    }
    return {0, 0};
}

ConnectorDirection OppositeDirection(ConnectorDirection direction) {
    return static_cast<ConnectorDirection>(
        (static_cast<unsigned>(direction) + 2) % 4
    );
}

const Part* FindPartAt(
    const EditorGrid& grid,
    Math::GridCoord coord
) {
    for (const Part* part : grid.GetActiveParts()) {
        if (part->GetGridCoord().x == coord.x &&
            part->GetGridCoord().y == coord.y) {
            return part;
        }
    }
    return nullptr;
}

}

void PartRenderer::Render(const PartRenderState& state) const {
    // fan uses its capsule height as size
    const float size = state.type == PartType::Fan
        ? state.capsuleHeight
        : state.cellSize;
    DrawPart(
        m_textures,
        state.type,
        state.position.ToRayVec(),
        static_cast<float>(state.rotation),
        static_cast<float>(state.attachmentRotation),
        size,
        state.propulsionActive ? ActiveTint : WHITE
    );
}

void PartRenderer::RenderContained(
    const Part& part,
    const EditorGrid& grid,
    Math::GridCoord containerCoord
) const {
    PartRenderState state = part.GetRenderState(grid);
    state.cellSize = grid.GetCellSize();
    state.capsuleHeight = state.cellSize;
    state.capsuleRadius = state.cellSize * 0.25f;
    Render(state);
}

void PartRenderer::RenderConnectors(
    const Part& part,
    const EditorGrid& grid
) const {
    const PartDefinition& definition = GetPartDefinition(part.GetType());
    const ConnectorMask mask = RotateConnectorMask(
        definition.connectors,
        part.GetOrientation()
    );
    const Math::Vector2D center = grid.CoordToWorld(part.GetGridCoord());
    const float halfCell = grid.GetCellSize() / 2.0f;
    const float lineHalfLength = grid.GetCellSize() * 0.22f;

    for (unsigned index = 0; index < 4; ++index) {
        const auto direction = static_cast<ConnectorDirection>(index);
        if ((mask & ConnectorBit(direction)) == 0) continue;

        const Math::GridCoord offset = ConnectorOffset(direction);
        const Part* neighbor = FindPartAt(
            grid,
            Math::GridCoord{
                part.GetGridCoord().x + offset.x,
                part.GetGridCoord().y + offset.y
            }
        );
        const bool facingConnector = neighbor != nullptr &&
            (RotateConnectorMask(
                GetPartDefinition(neighbor->GetType()).connectors,
                neighbor->GetOrientation()
            ) & ConnectorBit(OppositeDirection(direction))) != 0;
        const bool connected = facingConnector &&
            ArePartsCompatible(part.GetType(), neighbor->GetType());

        Vector2 lineStart;
        Vector2 lineEnd;
        const Vector2 worldCenter = center.ToRayVec();
        if (direction == ConnectorDirection::Up ||
            direction == ConnectorDirection::Down) {
            const float boundaryY = worldCenter.y +
                (direction == ConnectorDirection::Up ? -halfCell : halfCell);
            lineStart = {worldCenter.x - lineHalfLength, boundaryY};
            lineEnd = {worldCenter.x + lineHalfLength, boundaryY};
        } else {
            const float boundaryX = worldCenter.x +
                (direction == ConnectorDirection::Left ? -halfCell : halfCell);
            lineStart = {boundaryX, worldCenter.y - lineHalfLength};
            lineEnd = {boundaryX, worldCenter.y + lineHalfLength};
        }

        DrawLineEx(
            lineStart,
            lineEnd,
            4.0f,
            connected
                ? Color{120, 255, 120, 255}
                : facingConnector
                    ? Color{255, 40, 40, 255}
                    : Color{255, 80, 190, 255}
        );
    }
}

void PartRenderer::RenderGhost(
    PartType type,
    Math::GridCoord coord,
    const EditorGrid& grid,
    PartOrientation orientation
) const {
    const Math::Vector2D anchor = grid.CoordToWorld(coord);
    RenderGhostAtPosition(type, anchor, grid.GetCellSize(), orientation);
}

void PartRenderer::RenderGhostAtPosition(
    PartType type,
    Math::Vector2D anchor,
    float cellSize,
    PartOrientation orientation
) const {
    const float rotation = PartOrientationRadians(orientation);
    Vector2 position = anchor.ToRayVec();
    if (type == PartType::CircleWheel) {
        // the cursor marks the anchor, the wheel sits below it
        const Vector2 offset = WheelCenterOffset(cellSize, rotation);
        position.x += offset.x;
        position.y += offset.y;
    }
    DrawPart(
        m_textures,
        type,
        position,
        rotation,
        rotation,
        cellSize,
        Fade(WHITE, GhostAlpha)
    );
}
