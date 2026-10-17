#ifndef I_PHYSICS_HPP
#define I_PHYSICS_HPP

#include "math/Vector2D.hpp"
#include <vector>
#include <memory>

enum class BodyType {
    Static,
    Dynamic
};

// abstract representation of a rigid body
class IRigidBody {
public:
    virtual ~IRigidBody() = default;
    virtual Math::Vector2D GetPosition() const = 0;
    virtual float GetRotation() const = 0;
    virtual Math::Vector2D GetSize() const = 0;
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

    // rigidbody creation
    virtual std::shared_ptr<IRigidBody> CreateBox(
        Math::Vector2D position,
        float rotation,
        Math::Vector2D size,
        float mass,
        BodyType type
    ) = 0;
    virtual std::shared_ptr<IRigidBody> CreateCircle(
        Math::Vector2D position,
        float radius,
        float mass,
        BodyType type
    ) = 0;

    // terrain generation (polylines)
    virtual std::vector<Math::Vector2D> CreateTerrainChain(
        const std::vector<Math::Vector2D>& controlPoints,
        int resolutionPerSegment
    ) = 0;

    // joints creation (to connect chasis and wheels)
    virtual std::shared_ptr<IJoint> CreateRevoluteJoint(
        std::shared_ptr<IRigidBody> bodyA,
        std::shared_ptr<IRigidBody> bodyB,
        Math::Vector2D anchor
    ) = 0;
};

#endif
