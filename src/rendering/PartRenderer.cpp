#include "rendering/PartRenderer.hpp"

#include "app/game/EditorGrid.hpp"
#include "app/game/PartDefinition.hpp"
#include "raylib.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>

namespace {

Vector2 RotateLocalOffset(Vector2 offset, float rotation) {
    return Vector2{
        offset.x * std::cos(rotation) - offset.y * std::sin(rotation),
        offset.x * std::sin(rotation) + offset.y * std::cos(rotation)
    };
}

void RenderOrientationMarker(Vector2 position, float cellSize, float rotation) {
    const float markerLength = cellSize * 0.4f;
    const float endSize = cellSize * 0.08f;
    const Vector2 markerEnd{
        position.x + std::cos(rotation) * markerLength,
        position.y + std::sin(rotation) * markerLength
    };
    DrawLineEx(position, markerEnd, 3.0f, BLACK);
    DrawCircleV(markerEnd, endSize, BLACK);
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
    const float cellSize = state.cellSize;
    const Vector2 position = state.position.ToRayVec();
    const float rotationRad = static_cast<float>(state.rotation);
    const float rotationDeg = rotationRad * RAD2DEG;

    switch (state.type) {
        case PartType::SquareBlock:
            rlPushMatrix();
            rlTranslatef(position.x, position.y, 0.0f);
            rlRotatef(rotationDeg, 0.0f, 0.0f, 1.0f);
            DrawLineEx(
                {-cellSize / 2.0f, -cellSize / 2.0f},
                {cellSize / 2.0f, -cellSize / 2.0f},
                6.0f,
                BLUE
            );
            DrawLineEx(
                {cellSize / 2.0f, -cellSize / 2.0f},
                {cellSize / 2.0f, cellSize / 2.0f},
                6.0f,
                BLUE
            );
            DrawLineEx(
                {cellSize / 2.0f, cellSize / 2.0f},
                {-cellSize / 2.0f, cellSize / 2.0f},
                6.0f,
                BLUE
            );
            DrawLineEx(
                {-cellSize / 2.0f, cellSize / 2.0f},
                {-cellSize / 2.0f, -cellSize / 2.0f},
                6.0f,
                BLUE
            );
            rlPopMatrix();
            break;
        case PartType::CircleWheel:
        {
            const PartDefinition& definition =
                GetPartDefinition(PartType::CircleWheel);
            const float radius = cellSize * definition.radiusScale;
            const float attachmentRotation =
                static_cast<float>(state.attachmentRotation);
            const Vector2 wheelOffset = RotateLocalOffset(
                Vector2{
                    0.0f,
                    cellSize * (0.5f - definition.radiusScale)
                },
                attachmentRotation
            );
            const Vector2 anchor{
                position.x - wheelOffset.x,
                position.y - wheelOffset.y
            };

            DrawCircleLines(position.x, position.y, radius, BLACK);
            DrawCircle(position.x, position.y, radius - 2, GREEN);

            const float spokeX =
                position.x + std::cos(rotationRad) * (radius - 2);
            const float spokeY =
                position.y + std::sin(rotationRad) * (radius - 2);
            DrawLineV(position, Vector2{spokeX, spokeY}, BLACK);

            const float halfSize = cellSize / 2.0f;
            const float triangleTipY = cellSize * 0.68f - radius;
            const Vector2 baseLeft{-halfSize, -halfSize};
            const Vector2 baseRight{halfSize, -halfSize};
            const Vector2 tip{0.0f, 0 + radius/2};

            rlPushMatrix();
            rlTranslatef(anchor.x, anchor.y, 0.0f);
            rlRotatef(attachmentRotation * RAD2DEG, 0.0f, 0.0f, 1.0f);

            DrawTriangle(tip, baseLeft, baseRight, DARKGRAY);
            DrawTriangleLines(tip, baseLeft, baseRight, BLACK);

            rlPopMatrix();

            break;
        }

        case PartType::TNTBarrel:
            DrawRectanglePro(
                Rectangle{position.x, position.y, cellSize, cellSize},
                Vector2{cellSize / 2.0f, cellSize / 2.0f},
                rotationDeg,
                MAROON
            );
            RenderOrientationMarker(position, cellSize, rotationRad);
            break;
        case PartType::Fan: {
            const float width = state.capsuleRadius * 2.0f;
            const float halfHeight = state.capsuleHeight / 2.0f;
            const float halfSegment =
                std::max(0.0f, halfHeight - state.capsuleRadius);

            rlPushMatrix();
            rlTranslatef(position.x, position.y, 0.0f);
            rlRotatef(rotationDeg, 0.0f, 0.0f, 1.0f);
            const Color fanColor = state.propulsionActive ? RED : ORANGE;
            DrawRectangleRec(
                Rectangle{-width / 2.0f, -halfSegment, width, halfSegment * 2.0f},
                fanColor
            );
            DrawCircle(0.0f, -halfSegment, state.capsuleRadius, fanColor);
            DrawCircle(0.0f, halfSegment, state.capsuleRadius, fanColor);
            rlPopMatrix();
            RenderOrientationMarker(position, cellSize, rotationRad);
            break;
        }
        case PartType::WindSail: {
            const float halfSize = cellSize / 2.0f;
            const Vector2 tip{-halfSize, 0.0f};
            const Vector2 baseTop{halfSize, -halfSize};
            const Vector2 baseBottom{halfSize, halfSize};
            const Color sailColor = state.propulsionActive
                ? RED
                : YELLOW;
            const Color sailOutline = state.propulsionActive
                ? MAROON
                : BLACK;

            rlPushMatrix();
            rlTranslatef(position.x, position.y, 0.0f);
            rlRotatef(rotationDeg, 0.0f, 0.0f, 1.0f);
            DrawTriangle(
                tip,
                baseBottom,
                baseTop,
                sailColor
            );
            DrawTriangleLines(tip, baseBottom, baseTop, sailOutline);
            rlPopMatrix();
            RenderOrientationMarker(position, cellSize, rotationRad);
            break;
        }
        case PartType::Driver: {
            rlPushMatrix();
            rlTranslatef(position.x, position.y, 0.0f);
            rlRotatef(rotationDeg, 0.0f, 0.0f, 1.0f);
            DrawCircleV({-4.0f, -9.0f}, 5.0f, GOLD);
            DrawCircleLines(-4.0f, -9.0f, 5.0f, BLACK);
            DrawLineEx({-4.0f, -4.0f}, {-4.0f, 7.0f}, 2.5f, DARKGRAY);
            DrawLineEx({-4.0f, -1.0f}, {5.0f, -4.0f}, 2.0f, DARKGRAY);
            DrawLineEx({5.0f, -4.0f}, {9.0f, 1.0f}, 2.0f, DARKGRAY);
            DrawCircleLines(9.0f, 2.0f, 5.0f, MAROON);
            DrawLineEx({-4.0f, 7.0f}, {-9.0f, 14.0f}, 2.5f, DARKGRAY);
            DrawLineEx({-4.0f, 7.0f}, {1.0f, 14.0f}, 2.5f, DARKGRAY);
            rlPopMatrix();
            break;
        }
    }
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
    Math::Vector2D position = anchor;
    const float halfCell = cellSize / 2.0f;
    const float rotation = PartOrientationRadians(orientation) * RAD2DEG;
    const PartDefinition& definition = GetPartDefinition(type);
    if (type == PartType::CircleWheel) {
        const Vector2 wheelOffset = RotateLocalOffset(
        Vector2{
            0.0f,
            cellSize * (0.5f - definition.radiusScale)
        },
        PartOrientationRadians(orientation)
        );
        const Vector2 wheelPosition{
            static_cast<float>(anchor.x) + wheelOffset.x,
            static_cast<float>(anchor.y) + wheelOffset.y
        };
        position = {
            static_cast<float>(wheelPosition.x),
            static_cast<float>(wheelPosition.y)
        };
    }

    switch (type) {
        case PartType::SquareBlock:
            rlPushMatrix();
            rlTranslatef(position.x, position.y, 0.0f);
            rlRotatef(rotation, 0.0f, 0.0f, 1.0f);
            DrawLineEx(
                {-cellSize / 2.0f, -cellSize / 2.0f},
                {cellSize / 2.0f, -cellSize / 2.0f},
                6.0f,
                Fade(BLUE, 0.2f)
            );
            DrawLineEx(
                {cellSize / 2.0f, -cellSize / 2.0f},
                {cellSize / 2.0f, cellSize / 2.0f},
                6.0f,
                Fade(BLUE, 0.2f)
            );
            DrawLineEx(
                {cellSize / 2.0f, cellSize / 2.0f},
                {-cellSize / 2.0f, cellSize / 2.0f},
                6.0f,
                Fade(BLUE, 0.2f)
            );
            DrawLineEx(
                {-cellSize / 2.0f, cellSize / 2.0f},
                {-cellSize / 2.0f, -cellSize / 2.0f},
                6.0f,
                Fade(BLUE, 0.2f)
            );
            rlPopMatrix();
            break;
        case PartType::CircleWheel:
        {
            const float radius = cellSize * definition.radiusScale;
            const float wheelX = static_cast<float>(position.x);
            const float wheelY = static_cast<float>(position.y);
            DrawCircleLines(
                wheelX,
                wheelY,
                radius,
                Fade(BLACK, 0.2f)
            );
            DrawCircle(
                wheelX,
                wheelY,
                radius - 2,
                Fade(GREEN, 0.2f)
            );
            const float halfSize = cellSize / 2.0f;
            const float triangleTipY = cellSize * 0.68f - radius;
            const Vector2 baseLeft{-halfSize, -halfSize};
            const Vector2 baseRight{halfSize, -halfSize};
            const Vector2 tip{0.0f, radius / 2.0f};

            rlPushMatrix();
            rlTranslatef(
                static_cast<float>(anchor.x),
                static_cast<float>(anchor.y),
                0.0f
            );
            rlRotatef(rotation, 0.0f, 0.0f, 1.0f);
            DrawTriangle(tip, baseLeft, baseRight, Fade(DARKGRAY, 0.35f));
            DrawTriangleLines(tip, baseLeft, baseRight, Fade(BLACK, 0.35f));
            rlPopMatrix();
            break;
        }
        case PartType::TNTBarrel:
            DrawRectanglePro(
                Rectangle{
                    static_cast<float>(position.x),
                    static_cast<float>(position.y),
                    cellSize,
                    cellSize
                },
                Vector2{halfCell, halfCell},
                0.0f,
                Fade(MAROON, 0.2f)
            );
            break;
        case PartType::Fan: {
            const float width = cellSize * 0.5f;
            const float halfHeight = cellSize / 2.0f;
            const float halfSegment = halfHeight - width / 2.0f;

            rlPushMatrix();
            rlTranslatef(
                static_cast<float>(position.x),
                static_cast<float>(position.y),
                0.0f
            );
            rlRotatef(rotation, 0.0f, 0.0f, 1.0f);
            DrawRectangleRec(
                Rectangle{-width / 2.0f, -halfSegment, width, halfSegment * 2.0f},
                Fade(ORANGE, 0.2f)
            );
            DrawCircle(0.0f, -halfSegment, width / 2.0f, Fade(ORANGE, 0.2f));
            DrawCircle(0.0f, halfSegment, width / 2.0f, Fade(ORANGE, 0.2f));
            rlPopMatrix();
            break;
        }
        case PartType::WindSail: {
            const float halfSize = cellSize / 2.0f;
            const Vector2 tip{-halfSize, 0.0f};
            const Vector2 baseTop{halfSize, -halfSize};
            const Vector2 baseBottom{halfSize, halfSize};

            rlPushMatrix();
            rlTranslatef(
                static_cast<float>(position.x),
                static_cast<float>(position.y),
                0.0f
            );
            rlRotatef(rotation, 0.0f, 0.0f, 1.0f);
            DrawTriangle(tip, baseBottom, baseTop, Fade(YELLOW, 0.2f));
            DrawTriangleLines(
                tip,
                baseBottom,
                baseTop,
                Fade(BLACK, 0.2f)
            );
            rlPopMatrix();
            break;
        }
        case PartType::Driver: {
            rlPushMatrix();
            rlTranslatef(static_cast<float>(anchor.x), static_cast<float>(anchor.y), 0.0f);
            rlRotatef(rotation, 0.0f, 0.0f, 1.0f);
            DrawCircleLines(-4.0f, -9.0f, 5.0f, Fade(BLACK, 0.35f));
            DrawLineEx({-4.0f, -4.0f}, {-4.0f, 7.0f}, 2.0f, Fade(DARKGRAY, 0.35f));
            DrawCircleLines(9.0f, 2.0f, 5.0f, Fade(MAROON, 0.35f));
            rlPopMatrix();
            break;
        }
        default:
            break;
    }
}
