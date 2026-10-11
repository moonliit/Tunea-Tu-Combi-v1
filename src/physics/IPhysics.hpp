#ifndef I_PHYSICS_HPP
#define I_PHYSICS_HPP

#include "common/types.hpp"
#include <memory>

enum class BodyType {
    Static,
    Dynamic
};

// abstract representation of a rigid body
class IRigidBody {
public:
    virtual ~IRigidBody() = default;
    virtual Vector2D GetPosition() const = 0;
    virtual float GetRotation() const = 0;
    virtual Vector2D GetSize() const = 0;
    virtual float GetRadius() const = 0;
};

// abstract representation of a joint
class IJoint {
public:
    virtual ~IJoint() = default;
};

// abstract representation of the physics simulation world
class IPhysicsWorld {
public:
    virtual ~IPhysicsWorld() = default;
    virtual void Step(float deltaTime) = 0;
    virtual std::shared_ptr<IRigidBody> CreateBox(Vector2D position, float rotation, Vector2D size, float mass, BodyType type) = 0;
    virtual std::shared_ptr<IRigidBody> CreateCircle(Vector2D position, float radius, float mass, BodyType type) = 0;

    // joints (to connect chasis and wheels)
    virtual std::shared_ptr<IJoint> CreateRevoluteJoint(
        std::shared_ptr<IRigidBody> bodyA,
        std::shared_ptr<IRigidBody> bodyB,
        Vector2D anchor
    ) = 0;
};

#endif
