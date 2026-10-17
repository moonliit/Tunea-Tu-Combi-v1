#include "game/Part.hpp"
#include "math/Vector2D.hpp"

Part::Part(Math::GridCoord gridCoord, PartType type):
    m_gridCoord(gridCoord), m_type(type), m_activeCacheIndex(0) {}

void Part::SetPhysicsBody(float cellSize, std::unique_ptr<IPhysicsWorld>& physics) {
    Math::Vector2D worldPos = {
        m_gridCoord.x * cellSize + cellSize / 2.0f,
        m_gridCoord.y * cellSize + cellSize / 2.0f,
    };
    std::shared_ptr<IRigidBody> rb = nullptr;

    switch (m_type) {
        case PartType::SquareBlock:
            rb = physics->CreateBox(
                worldPos, 0.0f,
                Math::Vector2D{cellSize, cellSize}, 1.0f, BodyType::Dynamic
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

void Part::Render(float cellSize) const {
    Math::Vector2D pos = m_physicsBody ? m_physicsBody->GetPosition() : m_gridCoord.ToWorldSpace(cellSize);
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
            DrawCircle(pos.x, pos.y, cellSize / 2.0f - 2, DARKGRAY);
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
