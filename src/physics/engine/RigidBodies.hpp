#ifndef PHYSICS_RIGIDBODIES_HPP
#define PHYSICS_RIGIDBODIES_HPP

#include "math/Vector2D.hpp"
#include "physics/engine/BaseRigidBody.hpp"

namespace Physics {

class BoxRigidBody : public BaseRigidBody {
private:
    Math::Vector2D m_size;

public:
    explicit BoxRigidBody(Math::Vector2D size);
    virtual ~BoxRigidBody() override;

    Math::Vector2D GetSize() const { return m_size; }
};

class CircleRigidBody : public BaseRigidBody {
private:
    float m_radius;

public:
    explicit CircleRigidBody(float radius);
    virtual ~CircleRigidBody() override;

    float GetRadius() const { return m_radius; }
};

class CapsuleRigidBody : public BaseRigidBody {
private:
    float m_height;
    float m_radius;

public:
    explicit CapsuleRigidBody(float height, float radius);
    virtual ~CapsuleRigidBody() override;

    float GetHeight() const { return m_height; }
    float GetRadius() const { return m_radius; }
};

class TriangleRigidBody : public BaseRigidBody {
private:
    Math::Vector2D m_size;

public:
    explicit TriangleRigidBody(Math::Vector2D size);
    virtual ~TriangleRigidBody() override;

    Math::Vector2D GetSize() const { return m_size; }
};

}

#endif
