#ifndef PHYSICS_RIGIDBODIES_HPP
#define PHYSICS_RIGIDBODIES_HPP

#include "math/Vector2D.hpp"
#include "physics/engine/BaseRigidBody.hpp"
#include <array>

namespace Physics {

class BoxRigidBody : public BaseRigidBody {
private:
    Math::Vector2D m_size;

public:
    explicit BoxRigidBody(
        Math::Vector2D position,
        Math::RealNum rotation,
        Math::Vector2D size,
        Math::RealNum mass,
        BodyType type,
        Material material
    );
    virtual ~BoxRigidBody() override = default;

    Math::Vector2D GetSize() const { return m_size; }
    void UpdateAABB() override;
};

class CircleRigidBody : public BaseRigidBody {
private:
    Math::RealNum m_radius;

public:
    explicit CircleRigidBody(
        Math::Vector2D position,
        Math::RealNum radius,
        Math::RealNum mass,
        BodyType type,
        Material material
    );
    virtual ~CircleRigidBody() override = default;

    float GetRadius() const { return static_cast<float>(m_radius); }
    void UpdateAABB() override;
};

class CapsuleRigidBody : public BaseRigidBody {
private:
    float m_height;
    float m_radius;

public:
    explicit CapsuleRigidBody(
        Math::Vector2D position,
        Math::RealNum rotation,
        Math::RealNum height,
        Math::RealNum radius,
        Math::RealNum mass,
        BodyType type,
        Material material
    );
    virtual ~CapsuleRigidBody() override = default;

    float GetHeight() const { return m_height; }
    float GetRadius() const { return m_radius; }
    void UpdateAABB() override;
};

class TriangleRigidBody : public BaseRigidBody {
private:
    Math::Vector2D m_size;
    std::array<Math::Vector2D, 3> m_vertices;

public:
    explicit TriangleRigidBody(
        Math::Vector2D position,
        Math::RealNum rotation,
        Math::Vector2D size,
        Math::RealNum mass,
        BodyType type,
        Material material
    );
    virtual ~TriangleRigidBody() override = default;

    Math::Vector2D GetSize() const { return m_size; }
    void UpdateAABB() override;
};

}

#endif
