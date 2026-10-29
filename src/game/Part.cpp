#include "game/Part.hpp"
#include "game/EditorGrid.hpp"
#include "math/GridCoord.hpp"
#include "math/Vector2D.hpp"
#include "physics/I_Physics.hpp"
#include "raylib.h"
#include "rlgl.h"

using namespace Math;

Part::Part(GridCoord gridCoord, PartType type):
    m_gridCoord(gridCoord), m_type(type), m_activeCacheIndex(0) {}

void Part::SetPhysicsBody(const EditorGrid& grid, std::unique_ptr<I_PhysicsWorld>& physics) {
    Vector2D worldPos = grid.CoordToWorld(m_gridCoord);
    const float cellSize = grid.GetCellSize();

    std::shared_ptr<I_RigidBody> rb = nullptr;

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
        case PartType::Fan:
            rb = physics->CreateCapsule(
                worldPos, 0.f,
                cellSize, cellSize / 4.f,
                1.f, BodyType::Dynamic
            );
            break;
        case PartType::WindSail:
            rb = physics->CreateTriangle(
                worldPos, 0.f,
                Vector2D{cellSize, cellSize}, 1.f, BodyType::Dynamic
            );
            break;
        default:
            break;
    }

    m_physicsBody = rb;
}

#include "physics/Box2D_Physics.hpp"

void Part::DrawSelf(const EditorGrid& grid) const {
    const float cellSize = grid.GetCellSize();
    Vector2D pos = m_physicsBody ? m_physicsBody->GetPosition() : grid.CoordToWorld(m_gridCoord);
    RealNum rot = m_physicsBody ? m_physicsBody->GetRotation() * RAD2DEG : RealNum {0.f};

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

        case PartType::Fan: {
            float capsuleHeight = cellSize;
            float capsuleRadius = cellSize * 0.25f;
            if (auto capsuleBody = std::dynamic_pointer_cast<Box2D_CapsuleRigidBody>(m_physicsBody)) {
                capsuleHeight = capsuleBody->GetHeight();
                capsuleRadius = capsuleBody->GetRadius();
            }

            float width = capsuleRadius * 2.0f;
            float halfHeight = capsuleHeight / 2.0f;
            float halfSegment = std::max(0.0f, halfHeight - capsuleRadius);

            // Use Raylib's matrix stack to handle rotation and translation as a single unit
            rlPushMatrix();
            rlTranslatef(static_cast<float>(pos.x), static_cast<float>(pos.y), 0.0f);
            rlRotatef(rot, 0.0f, 0.0f, 1.0f); // rot is already in degrees

            // 1. Draw the central connecting rectangle (between the two semi-circle caps)
            DrawRectangleRec(
                Rectangle{ -width / 2.0f, -halfSegment, width, halfSegment * 2.0f },
                ORANGE
            );

            // 2. Draw the top and bottom circular caps
            DrawCircle(0.0f, -halfSegment, capsuleRadius, ORANGE);
            DrawCircle(0.0f,  halfSegment, capsuleRadius, ORANGE);

            rlPopMatrix();
            break;
        }

        case PartType::WindSail: {
            float halfSize = cellSize / 2.0f;

            // Define local vertices relative to the center (0,0)
            // Matching the physics body: top tip pointing up (-Y), base at the bottom (+Y)
            Vector2 localV1 = { 0.0f, -halfSize };          // Top tip
            Vector2 localV2 = { -halfSize, halfSize };      // Bottom-left
            Vector2 localV3 = {  halfSize, halfSize };      // Bottom-right

            rlPushMatrix();
            rlTranslatef(static_cast<float>(pos.x), static_cast<float>(pos.y), 0.0f);
            rlRotatef(rot, 0.0f, 0.0f, 1.0f); // rot is in degrees

            // Draw using local coordinates; rotation is handled by the matrix stack
            DrawTriangle(localV1, localV2, localV3, YELLOW);
            DrawTriangleLines(localV1, localV2, localV3, BLACK);

            rlPopMatrix();
            break;
        }

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
