#ifndef PART_HPP
#define PART_HPP

#include "math/Vector2D.hpp"
#include "math/GridCoord.hpp"
#include "physics/IPhysics.hpp"
#include <memory>

class EditorGrid;

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

    void SetPhysicsBody(const EditorGrid& grid, std::unique_ptr<IPhysicsWorld>& physics);
    void ResetPhysicsBody() { m_physicsBody = nullptr; };

    void DrawSelf(const EditorGrid& grid) const;
    static void DrawGhost(PartType type, Math::GridCoord coord, const EditorGrid& grid);

private:
    Math::GridCoord m_gridCoord;
    PartType m_type;
    size_t m_activeCacheIndex;

    std::shared_ptr<IRigidBody> m_physicsBody = nullptr; // Physics handle (only valid when in Play mode)

    friend class EditorGrid;
};

#endif
