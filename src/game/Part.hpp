#ifndef PART_HPP
#define PART_HPP

#include "math/GridCoord.hpp"
#include "physics/IPhysics.hpp"
#include <memory>

enum class PartType {
    SquareBlock,
    CircleWheel,
    TNTBarrel,    // Future item with box collision but custom rendering/behavior
    Fan           // Future item
};

class Part {
public:
    Part(Math::GridCoord gridCoord, PartType type);
    bool IsActive() const { return m_physicsBody != nullptr; }

    void SetPhysicsBody(float cellSize, std::unique_ptr<IPhysicsWorld>& physics);
    void ResetPhysicsBody() { m_physicsBody = nullptr; };

    void Render(float cellSize) const;

private:
    Math::GridCoord m_gridCoord;
    PartType m_type;
    size_t m_activeCacheIndex;

    std::shared_ptr<IRigidBody> m_physicsBody = nullptr; // Physics handle (only valid when in Play mode)

    friend class EditorGrid;
};

#endif
