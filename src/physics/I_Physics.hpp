#ifndef I_PHYSICS_HPP
#define I_PHYSICS_HPP

#include "math/Vector2D.hpp"
#include <vector>
#include <memory>

class Part;

struct PhysicsImpact {
    Part* part;
    float impulse;
};

enum class BodyType {
    Static,
    Dynamic
};

// abstract representation of a rigid body
class I_RigidBody {
protected:
    Part* m_part = nullptr; // associated part
    friend class Part;
public:
    virtual ~I_RigidBody() = default;
    virtual Math::Vector2D GetPosition() const = 0;
    virtual Math::RealNum GetRotation() const = 0;
    virtual void AddMass(float mass) = 0;
    virtual void ApplyForce(Math::Vector2D force) = 0;
    virtual void ApplyLinearImpulse(Math::Vector2D impulse) = 0;
    Part* GetPart() const { return m_part; }
};

// abstract representation of a joint
class I_Joint {
public:
    virtual ~I_Joint() = default;
};

// abstract representation of the physics simulation world
class I_PhysicsWorld {
public:
    virtual ~I_PhysicsWorld() = default;
    virtual void Step(float deltaTime) = 0;
    virtual std::vector<PhysicsImpact> ConsumeImpactEvents() = 0;
    virtual void RemoveBody(
        const std::shared_ptr<I_RigidBody>& body
    ) = 0;
    virtual void ApplyExplosion(
        const Math::Vector2D& center,
        float impulseForce,
        float radius
    ) = 0;

    // rigidbody creation
    virtual std::shared_ptr<I_RigidBody> CreateBox(
        Math::Vector2D position,
        float rotation,
        Math::Vector2D size,
        float mass,
        BodyType type
    ) = 0;

    virtual std::shared_ptr<I_RigidBody> CreateCircle(
        Math::Vector2D position,
        float radius,
        float mass,
        BodyType type
    ) = 0;

    virtual std::shared_ptr<I_RigidBody> CreateCapsule(
        Math::Vector2D position,
        float rotation,
        float height,
        float radius,
        float mass,
        BodyType type
    ) = 0;

    virtual std::shared_ptr<I_RigidBody> CreateTriangle(
        Math::Vector2D position,
        float rotation,
        Math::Vector2D size,
        float mass,
        BodyType type
    ) = 0;

    // terrain generation (polylines)
    virtual std::vector<Math::Vector2D> CreateTerrainChain(
        const std::vector<Math::Vector2D>& controlPoints,
        bool isLoop = false
    ) = 0;

    virtual bool CreateTerrainPolygon(
        const std::vector<Math::Vector2D>& points
    ) = 0;

    // joints creation (to connect chasis and wheels)
    virtual std::shared_ptr<I_Joint> CreateRevoluteJoint(
        std::shared_ptr<I_RigidBody> bodyA,
        std::shared_ptr<I_RigidBody> bodyB,
        Math::Vector2D anchor
    ) = 0;

    virtual std::shared_ptr<I_Joint> CreateWeldJoint(
        std::shared_ptr<I_RigidBody> bodyA,
        std::shared_ptr<I_RigidBody> bodyB,
        Math::Vector2D anchor
    ) = 0;

};

#endif
