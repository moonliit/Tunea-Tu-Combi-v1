#include "game/Part.hpp"
#include "game/EditorGrid.hpp"
#include "math/GridCoord.hpp"
#include "math/Vector2D.hpp"
#include "raylib.h"

using namespace Math;

Part::Part(GridCoord gridCoord, PartType type):
    m_gridCoord(gridCoord), m_type(type), m_activeCacheIndex(0) {}

void Part::SetPhysicsBody(const EditorGrid& grid, std::unique_ptr<IPhysicsWorld>& physics) {
    Vector2D worldPos = grid.CoordToWorld(m_gridCoord);
    const float cellSize = grid.GetCellSize();

    std::shared_ptr<IRigidBody> rb = nullptr;

    switch (m_type) {
        case PartType::SquareBlock:
            rb = physics->CreateBox(
                worldPos, 0.0f,
                Vector2D{cellSize, cellSize}, 1.0f, BodyType::Dynamic
            );
            break;
        case PartType::CircleWheel:
            rb = physics->CreateCircle(
                worldPos,
                cellSize / 2.0f, 1.0f, BodyType::Dynamic
            );
            break;
        case PartType::TNTBarrel:
            break;
        default:
            break;
    }

    m_physicsBody = rb;
}

void Part::DrawSelf(const EditorGrid& grid) const {
    const float cellSize = grid.GetCellSize();
    Vector2D pos = m_physicsBody ? m_physicsBody->GetPosition() : grid.CoordToWorld(m_gridCoord);
    float rot = m_physicsBody ? m_physicsBody->GetRotation() * RAD2DEG : 0.f;

    switch (m_type) {
        case PartType::SquareBlock:
            DrawRectanglePro(
                Rectangle{static_cast<float>(pos.x), static_cast<float>(pos.y), cellSize, cellSize},
                Vector2{cellSize / 2.0f, cellSize / 2.0f},
                rot, BLUE
            );
            break;

        case PartType::CircleWheel:
            // Even if it has a circular collision shape, we draw it as a wheel with spokes or a distinct color
            DrawCircleLines(pos.x, pos.y, cellSize / 2.0f, BLACK);
            DrawCircle(pos.x, pos.y, cellSize / 2.0f - 2, GREEN);
            // Draw a little rotation indicator line so you can see the wheel spinning
            break;

        case PartType::TNTBarrel:
            // Custom rendering for TNT: Red box with a warning symbol or fuse
            DrawRectanglePro(
                Rectangle{static_cast<float>(pos.x), static_cast<float>(pos.y), cellSize, cellSize},
                Vector2{cellSize / 2.0f, cellSize / 2.0f},
                rot, MAROON
            );
            // DrawText("TNT", pos.x - 15, pos.y - 10, 10, YELLOW);
            break;

        default:
            break;
    }
}

void Part::DrawGhost(PartType type, GridCoord coord, const EditorGrid& grid) {
    Vector2D pos = grid.CoordToWorld(coord);
    const float cellSize = grid.GetCellSize();
    const float halfCell = cellSize / 2.f;

    switch (type) {

                // if (m_currentPartType == PartType::SquareBlock) {
                //     DrawRectangle(previewX, previewY, cellSize, cellSize, Fade(BLUE, 0.2f));
                //     DrawRectangleLines(previewX, previewY, cellSize, cellSize, BLUE);
                // } else {
                //     DrawCircle(previewX + cellSize / 2, previewY + cellSize / 2, cellSize / 2, Fade(GREEN, 0.2f));
                //     DrawCircleLines(previewX + cellSize / 2, previewY + cellSize / 2, cellSize / 2, GREEN);
                // }

        case PartType::SquareBlock:
            DrawRectangleRec(
                Rectangle{pos.x - halfCell, pos.y - halfCell, cellSize, cellSize},
                Fade(BLUE, 0.2f)
            );
            break;

        case PartType::CircleWheel:
            // Even if it has a circular collision shape, we draw it as a wheel with spokes or a distinct color
            DrawCircleLines(pos.x, pos.y, cellSize / 2.0f, Fade(BLACK, 0.2f));
            DrawCircle(pos.x, pos.y, cellSize / 2.0f - 2, Fade(GREEN, 0.2f));
            // Draw a little rotation indicator line so you can see the wheel spinning
            break;

        case PartType::TNTBarrel:
            // Custom rendering for TNT: Red box with a warning symbol or fuse
            DrawRectanglePro(
                Rectangle{static_cast<float>(pos.x), static_cast<float>(pos.y), cellSize, cellSize},
                Vector2{cellSize / 2.0f, cellSize / 2.0f},
                0, MAROON
            );
            // DrawText("TNT", pos.x - 15, pos.y - 10, 10, YELLOW);
            break;

        default:
            break;
    }
}
