#ifndef PART_HPP
#define PART_HPP

#include "math/Vector2D.hpp"
#include "math/GridCoord.hpp"
#include "physics/I_Physics.hpp"
#include <memory>
#include <cstdint>

class EditorGrid;

enum class PartType {
    SquareBlock,
    CircleWheel,
    TNTBarrel,    // Future item with box collision but custom rendering/behavior
    Fan,          // Capsule shape
    WindSail      // Triangle shape
};

enum class PartShape {
    Box,
    Circle,
    Capsule,
    Triangle
};

enum class PartRole {
    Normal,
    Container,
    Containable
};

enum class PartOrientation : std::uint8_t {
    Up,
    Right,
    Down,
    Left
};

constexpr PartOrientation RotatePartOrientation(
    PartOrientation orientation
) {
    switch (orientation) {
        case PartOrientation::Up: return PartOrientation::Right;
        case PartOrientation::Right: return PartOrientation::Down;
        case PartOrientation::Down: return PartOrientation::Left;
        case PartOrientation::Left: return PartOrientation::Up;
    }
    return PartOrientation::Up;
}

constexpr float PartOrientationRadians(PartOrientation orientation) {
    switch (orientation) {
        case PartOrientation::Up: return 0.0f;
        case PartOrientation::Right: return 1.5707963f;
        case PartOrientation::Down: return 3.1415927f;
        case PartOrientation::Left: return 4.7123890f;
    }
    return 0.0f;
}

using PartId = std::uint32_t;

struct PartRenderState {
    PartType type;
    Math::Vector2D position;
    Math::RealNum rotation;
    Math::RealNum attachmentRotation;
    bool propulsionActive;
    PartOrientation orientation;
    float cellSize;
    float capsuleHeight;
    float capsuleRadius;
};

class Part {
public:
    Part(
        Math::GridCoord gridCoord,
        PartType type,
        PartOrientation orientation = PartOrientation::Up,
        PartId id = 0
    );
    bool IsActive() const { return m_physicsBody != nullptr; }
    std::shared_ptr<I_RigidBody> GetPhysicsBody() const {
        return m_physicsBody;
    }
    void SetAttachmentBody(std::shared_ptr<I_RigidBody> body);
    void SetContainedBody(std::shared_ptr<I_RigidBody> body);
    void SetPropulsionActive(bool active) { m_propulsionActive = active; }
    void Activate();
    void UpdateActivation(float deltaTime);
    bool IsActivationActive() const { return m_activationActive; }

    void SetPhysicsBody(const EditorGrid& grid, std::unique_ptr<I_PhysicsWorld>& physics);
    void ResetPhysicsBody() {
        m_physicsBody = nullptr;
        m_attachmentBody = nullptr;
        m_attachmentRotationOffset = 0.0f;
        m_propulsionActive = false;
        m_activationActive = false;
        m_activationCooldown = 0.0f;
        if (m_containedPart) {
            m_containedPart->ResetPhysicsBody();
        }
    };

    PartType GetType() const { return m_type; }
    PartOrientation GetOrientation() const { return m_orientation; }
    PartId GetId() const { return m_id; }
    Math::GridCoord GetGridCoord() const { return m_gridCoord; }
    PartRenderState GetRenderState(const EditorGrid& grid) const;
    bool HasContainedPart() const { return m_containedPart != nullptr; }
    Part* GetContainedPart() { return m_containedPart.get(); }
    const Part* GetContainedPart() const { return m_containedPart.get(); }
    bool CanContain(const Part& part) const;
    bool ContainPart(std::unique_ptr<Part> part);
    std::unique_ptr<Part> RemoveContainedPart();

private:
    Math::GridCoord m_gridCoord;
    PartType m_type;
    PartOrientation m_orientation;
    PartId m_id;
    size_t m_activeCacheIndex;

    std::shared_ptr<I_RigidBody> m_physicsBody = nullptr; // Physics handle (only valid when in Play mode)
    std::shared_ptr<I_RigidBody> m_attachmentBody = nullptr;
    Math::RealNum m_attachmentRotationOffset = 0.0f;
    bool m_propulsionActive = false;
    bool m_activationActive = false;
    float m_activationCooldown = 0.0f;
    std::unique_ptr<Part> m_containedPart;

    friend class EditorGrid;
};

#endif
