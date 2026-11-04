#include "game/Part.hpp"

#include "game/EditorGrid.hpp"
#include "game/PartDefinition.hpp"

#include <algorithm>
#include <cmath>
#include "raylib.h"
#include "raymath.h"

using namespace Math;

namespace {

constexpr float FanForce = 750.0f;
constexpr float WindSailImpulse = 1500.0f;
constexpr float WindSailDelay = 0.75f;

Vector2D RotateLocalOffset(Vector2D offset, PartOrientation orientation) {
    switch (orientation) {
        case PartOrientation::Up:
            return offset;
        case PartOrientation::Right:
            return Vector2D{-offset.y, offset.x};
        case PartOrientation::Down:
            return Vector2D{-offset.x, -offset.y};
        case PartOrientation::Left:
            return Vector2D{offset.y, -offset.x};
    }
    return offset;
}

}

void Part::Activate() {
    if (!m_physicsBody) return;

    switch (m_type) {
        case PartType::Fan:
            m_activationActive = !m_activationActive;
            m_propulsionActive = m_activationActive;
            break;
        case PartType::WindSail:
            if (m_activationCooldown > 0.0f) return;
            {
                const Vector2D direction = Vector2D::FromAngle(
                    m_physicsBody->GetRotation()
                );
                m_physicsBody->ApplyLinearImpulse({
                    direction.x * WindSailImpulse,
                    direction.y * WindSailImpulse
                });
            }
            m_activationActive = true;
            m_propulsionActive = true;
            m_activationCooldown = WindSailDelay;
            break;
        case PartType::TNTBarrel:
        case PartType::SquareBlock:
        case PartType::CircleWheel:
            break;
    }
}

void Part::UpdateActivation(float deltaTime) {
    if (!m_physicsBody) return;

    if (m_activationCooldown > 0.0f) {
        m_activationCooldown = std::max(
            0.0f,
            m_activationCooldown - deltaTime
        );
        if (m_activationCooldown <= 0.0f &&
            m_type == PartType::WindSail) {
            m_activationActive = false;
        }
    }

    m_propulsionActive = m_activationActive;
    if (m_type != PartType::Fan || !m_activationActive) return;

    const Vector2D direction = Vector2D::FromAngle(
        m_physicsBody->GetRotation()
    );
    m_physicsBody->ApplyForce({
        direction.x * FanForce,
        direction.y * FanForce
    });
}

Part::Part(
    GridCoord gridCoord,
    PartType type,
    PartOrientation orientation,
    PartId id
):
    m_gridCoord(gridCoord),
    m_type(type),
    m_orientation(orientation),
    m_id(id),
    m_activeCacheIndex(0) {}

void Part::SetPhysicsBody(
    const EditorGrid& grid,
    std::unique_ptr<I_PhysicsWorld>& physics
) {
    const Vector2D worldPos = grid.CoordToWorld(m_gridCoord);
    const float cellSize = grid.GetCellSize();
    const PartDefinition& definition = GetPartDefinition(m_type);
    Vector2D physicsPosition = worldPos;
    if (m_type == PartType::CircleWheel) {
        const Vector2D offset = RotateLocalOffset(
            Vector2D{0.0f, cellSize * definition.attachmentOffsetScale},
            m_orientation
        );
        physicsPosition.x += offset.x;
        physicsPosition.y += offset.y;
    }
    std::shared_ptr<I_RigidBody> body;

    switch (m_type) {
        case PartType::SquareBlock:
            body = physics->CreateBox(
                physicsPosition,
                PartOrientationRadians(m_orientation),
                Vector2D{cellSize, cellSize},
                definition.mass,
                BodyType::Dynamic
            );
            break;
        case PartType::CircleWheel:
            body = physics->CreateCircle(
                physicsPosition,
                cellSize * definition.radiusScale,
                definition.mass,
                BodyType::Dynamic
            );
            break;
        case PartType::Fan:
            body = physics->CreateCapsule(
                physicsPosition,
                PartOrientationRadians(m_orientation),
                cellSize,
                cellSize / 4.0f,
                definition.mass,
                BodyType::Dynamic
            );
            break;
        case PartType::WindSail:
            body = physics->CreateTriangle(
                physicsPosition,
                PartOrientationRadians(m_orientation),
                Vector2D{cellSize, cellSize},
                definition.mass,
                BodyType::Dynamic
            );
            break;
        case PartType::TNTBarrel:
            body = physics->CreateBox(
                physicsPosition,
                PartOrientationRadians(m_orientation),
                Vector2D{cellSize, cellSize},
                definition.mass,
                BodyType::Dynamic
            );
            break;
    }

    m_physicsBody = body;
}

PartRenderState Part::GetRenderState(const EditorGrid& grid) const {
    const float cellSize = grid.GetCellSize();
    Vector2D position = m_physicsBody
        ? m_physicsBody->GetPosition()
        : grid.CoordToWorld(m_gridCoord);
    if (!m_physicsBody) {
        const PartDefinition& definition = GetPartDefinition(m_type);
        const Vector2D offset = RotateLocalOffset(
            Vector2D{0.0f, grid.GetCellSize() * definition.attachmentOffsetScale},
            m_orientation
        );
        position.x += offset.x;
        position.y += offset.y;
    }
    RealNum rotation = m_physicsBody
        ? m_physicsBody->GetRotation()
        : RealNum{PartOrientationRadians(m_orientation)};
    if (m_type == PartType::CircleWheel && m_physicsBody) {
        rotation += PartOrientationRadians(m_orientation);
    }
    const RealNum attachmentRotation = m_attachmentBody
        ? m_attachmentBody->GetRotation() + m_attachmentRotationOffset
        : RealNum{PartOrientationRadians(m_orientation)};

    return PartRenderState{
        .type = m_type,
        .position = position,
        .rotation = rotation,
        .attachmentRotation = attachmentRotation,
        .propulsionActive = m_propulsionActive,
        .orientation = m_orientation,
        .cellSize = cellSize,
        .capsuleHeight = cellSize,
        .capsuleRadius = cellSize * 0.25f
    };
}
