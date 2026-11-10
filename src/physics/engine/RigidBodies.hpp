#ifndef PHYSICS_RIGIDBODIES_HPP
#define PHYSICS_RIGIDBODIES_HPP

#include "math/Vector2D.hpp"
#include "physics/engine/BaseRigidBody.hpp"
#include "physics/engine/common/CollisionPolygon.hpp"

namespace Physics {

class BoxRigidBody : public PolygonRigidBody {
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
    CollisionPolygon GetCollisionPolygon() const override;

    void UpdateAABB() override;
    RbShape GetRbShape() const override { return RbShape::Box; }
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
    RbShape GetRbShape() const override { return RbShape::Circle; }
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
    RbShape GetRbShape() const override { return RbShape::Capsule; }
};

class TriangleRigidBody : public PolygonRigidBody {
private:
    Math::Vector2D m_size;

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
    CollisionPolygon GetCollisionPolygon() const override;

    void UpdateAABB() override;
    RbShape GetRbShape() const override { return RbShape::Triangle; }
};

class EdgeRigidBody : public BaseRigidBody {
private:
    Math::Vector2D m_localPoint1;
    Math::Vector2D m_localPoint2;
    Math::Vector2D m_worldPoint1;
    Math::Vector2D m_worldPoint2;
    Math::Vector2D m_normal;

public:
    explicit EdgeRigidBody(Math::Vector2D p1, Math::Vector2D p2);
    virtual ~EdgeRigidBody() override = default;

    const Math::Vector2D& GetP1() const { return m_worldPoint1; }
    const Math::Vector2D& GetP2() const { return m_worldPoint2; }
    const Math::Vector2D& GetNormal() const { return m_normal; }

    void UpdateAABB() override;
    RbShape GetRbShape() const override { return RbShape::Edge; }
};

}

#endif
