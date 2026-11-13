#include "app/game/Part.hpp"
#include "app/game/EditorGrid.hpp"
#include "app/game/PartDefinition.hpp"
#include <algorithm>
using namespace Math;

namespace {

constexpr float FanForce = 750.0f;

constexpr float WindSailForce = 1750.0f;
constexpr float WindSailActiveDuration = 0.75f;
constexpr float WindSailCooldownDuration = 1.25f;

constexpr float TntExplosionForce = 1000.0f;
constexpr float TntExplosionRadius = 750.0f;
constexpr float ContainedTntExplosionMultiplier = 1.75f;

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
    if (!m_physicsBody && !m_attachmentBody) return;

    switch (m_type) {
        case PartType::Fan:
            m_activationActive = !m_activationActive;
            m_propulsionActive = m_activationActive;
            break;
        case PartType::WindSail:
            if (m_cooldownRemaining > 0.0f) return;

            m_activationActive = true;
            m_propulsionActive = true;
            m_activeTimeRemaining = WindSailActiveDuration;
            m_cooldownRemaining = WindSailCooldownDuration;
            break;
        case PartType::TNTBarrel:
            m_activationActive = true;
        case PartType::SquareBlock:
        case PartType::CircleWheel:
        case PartType::Driver:
        case PartType::DriverBox:
            break;
    }
}

void Part::SetAttachmentBody(std::shared_ptr<I_RigidBody> body) {
    if (!body || m_attachmentBody) return;
    m_attachmentRotationOffset =
        PartOrientationRadians(m_orientation) - body->GetRotation();
    m_attachmentBody = std::move(body);
}

void Part::SetContainedBody(
    std::shared_ptr<I_RigidBody> body,
    I_PhysicsWorld* physicsWorld
) {
    if (!body || m_physicsBody) return;
    m_attachmentRotationOffset =
        PartOrientationRadians(m_orientation) - body->GetRotation();
    m_attachmentBody = std::move(body);
    m_physicsWorld = physicsWorld;
}

bool Part::CanContain(const Part& part) const {
    if (GetPartDefinition(m_type).role != PartRole::Container) return false;
    if (GetPartDefinition(part.m_type).role != PartRole::Containable) return false;
    // The driver box is dedicated to the driver: it can only contain a driver,
    // and a driver can only live inside a driver box (never a square box).
    if (m_type == PartType::DriverBox) {
        return part.m_type == PartType::Driver;
    }
    if (m_type == PartType::SquareBlock) {
        return part.m_type != PartType::Driver;
    }
    return true;
}

bool Part::ContainPart(std::unique_ptr<Part> part) {
    if (!part || !CanContain(*part)) return false;
    m_containedPart = std::move(part);
    return true;
}

std::unique_ptr<Part> Part::RemoveContainedPart() {
    return std::move(m_containedPart);
}

void Part::ApplyPropulsionForce(float magnitude) {
    const Vector2D direction = Vector2D::FromAngle(
        m_physicsBody->GetRotation()
    );
    m_physicsBody->ApplyForce({
        direction.x * magnitude,
        direction.y * magnitude
    });
}

void Part::ApplyRadialImpulseForce(float magnitude, float radius) {
    if (!m_physicsWorld || (!m_physicsBody && !m_attachmentBody)) return;
    const std::shared_ptr<I_RigidBody>& body =
        m_physicsBody ? m_physicsBody : m_attachmentBody;
    const Vector2D position = body->GetPosition();
    const Vector2D center = m_attachmentBody
        ? m_attachmentBody->GetPosition()
        : position;
    const float explosionMagnitude = m_attachmentBody
        ? magnitude * ContainedTntExplosionMultiplier
        : magnitude;
    m_physicsWorld->ApplyExplosion(
        center,
        explosionMagnitude,
        radius
    );
    m_removalRequested = true;
}

void Part::UpdateActivation(float deltaTime) {
    if (!m_physicsBody && !m_attachmentBody) return;

    if (m_cooldownRemaining > 0.0f) {
        m_cooldownRemaining = std::max(
            0.0f, m_cooldownRemaining - deltaTime
        );
    }

    if (m_activeTimeRemaining > 0.0f) {
        m_activeTimeRemaining = std::max(
            0.0f, m_activeTimeRemaining - deltaTime
        );
        if (m_activeTimeRemaining <= 0.0f) {
            m_activationActive = false;
        }
    }

    m_propulsionActive = m_activationActive;
    if (!m_activationActive) return;

    switch (m_type) {
        case PartType::Fan:
            ApplyPropulsionForce(FanForce);
            break;
        case PartType::WindSail:
            ApplyPropulsionForce(WindSailForce);
            break;
        case PartType::TNTBarrel:
            ApplyRadialImpulseForce(TntExplosionForce, TntExplosionRadius);
            m_activationActive = false;
            break;
        default:
            break;
    }
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
    m_id(id) {}

Part::Part(const Part& other):
    m_gridCoord(other.m_gridCoord),
    m_type(other.m_type),
    m_orientation(other.m_orientation),
    m_id(other.m_id),
    m_propulsionActive(false),
    m_activationActive(false),
    m_activeTimeRemaining(0.0f),
    m_cooldownRemaining(0.0f),
    m_removalRequested(false) {
    if (other.m_containedPart) {
        m_containedPart = std::make_unique<Part>(*other.m_containedPart);
    }
}

Part& Part::operator=(const Part& other) {
    if (this == &other) return *this;
    ResetPhysicsBody();
    m_gridCoord = other.m_gridCoord;
    m_type = other.m_type;
    m_orientation = other.m_orientation;
    m_id = other.m_id;
    m_propulsionActive = false;
    m_activationActive = false;
    m_activeTimeRemaining = 0.0f;
    m_cooldownRemaining = 0.0f;
    m_removalRequested = false;
    m_containedPart = other.m_containedPart
        ? std::make_unique<Part>(*other.m_containedPart)
        : nullptr;
    m_physicsWorld = nullptr;
    m_grid = nullptr;
    return *this;
}

void Part::SetPhysicsBody(
    EditorGrid& grid,
    std::unique_ptr<I_PhysicsWorld>& physics
) {
    m_grid = &grid;
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
        case PartType::SquareBlock: {
            const float containedMass = m_containedPart
                ? GetPartDefinition(m_containedPart->GetType()).mass
                : 0.0f;
            body = physics->CreateBox(
                physicsPosition,
                PartOrientationRadians(m_orientation),
                Vector2D{cellSize, cellSize},
                definition.mass + containedMass,
                BodyType::Dynamic
            );
            break;
        }
        case PartType::DriverBox: {
            const float containedMass = m_containedPart
                ? GetPartDefinition(m_containedPart->GetType()).mass
                : 0.0f;
            body = physics->CreateBox(
                physicsPosition,
                PartOrientationRadians(m_orientation),
                Vector2D{cellSize, cellSize},
                definition.mass + containedMass,
                BodyType::Dynamic
            );
            break;
        }
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
        case PartType::Driver:
            break;
    }

    m_physicsBody = body;
    m_physicsBody->m_part = this;
    m_physicsWorld = physics.get();
}

PartRenderState Part::GetRenderState(const EditorGrid& grid) const {
    const float cellSize = grid.GetCellSize();
    Vector2D position = m_physicsBody
        ? m_physicsBody->GetPosition()
        : m_attachmentBody
        ? m_attachmentBody->GetPosition()
        : grid.CoordToWorld(m_gridCoord);
    if (!m_physicsBody && !m_attachmentBody) {
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
        : m_attachmentBody
        ? m_attachmentBody->GetRotation() + m_attachmentRotationOffset
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
