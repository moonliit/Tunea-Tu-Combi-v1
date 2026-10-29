#ifndef BOX2D_PHYSICS_HPP
#define BOX2D_PHYSICS_HPP

#include "I_Physics.hpp"
#include "math/Vector2D.hpp"
#include <box2d/box2d.h>

class Box2D_IRigidBody : public I_RigidBody {
public:
    virtual ~Box2D_IRigidBody() = default;
    virtual b2BodyId GetInternalId() const = 0;

    Math::Vector2D GetPosition() const override;
    Math::RealNum GetRotation() const override;
};

class Box2D_BoxRigidBody : public Box2D_IRigidBody {
private:
    b2BodyId m_bodyId;
    Math::Vector2D m_size;

public:
    explicit Box2D_BoxRigidBody(b2BodyId bodyId, Math::Vector2D size);
    virtual ~Box2D_BoxRigidBody() override;

    Math::Vector2D GetSize() const { return m_size; }
    b2BodyId GetInternalId() const override { return m_bodyId; }
};

class Box2D_CircleRigidBody : public Box2D_IRigidBody {
private:
    b2BodyId m_bodyId;
    float m_radius;

public:
    explicit Box2D_CircleRigidBody(b2BodyId bodyId, float radius);
    virtual ~Box2D_CircleRigidBody() override;

    float GetRadius() const { return m_radius; }
    b2BodyId GetInternalId() const override { return m_bodyId; }
};

class Box2D_CapsuleRigidBody : public Box2D_IRigidBody {
private:
    b2BodyId m_bodyId;
    float m_height;
    float m_radius;

public:
    explicit Box2D_CapsuleRigidBody(b2BodyId bodyId, float height, float radius);
    virtual ~Box2D_CapsuleRigidBody() override;

    float GetHeight() const { return m_height; }
    float GetRadius() const { return m_radius; }
    b2BodyId GetInternalId() const override { return m_bodyId; }
};

class Box2D_TriangleRigidBody : public Box2D_IRigidBody {
private:
    b2BodyId m_bodyId;
    Math::Vector2D m_size;

public:
    explicit Box2D_TriangleRigidBody(b2BodyId bodyId, Math::Vector2D size);
    virtual ~Box2D_TriangleRigidBody() override;

    Math::Vector2D GetSize() const { return m_size; }
    b2BodyId GetInternalId() const override { return m_bodyId; }
};

class Box2D_Joint : public I_Joint {
private:
    b2JointId m_jointId;
public:
    explicit Box2D_Joint(b2JointId jointId);
    b2JointId GetInternalId() const { return m_jointId; }
};

class Box2D_PhysicsWorld : public I_PhysicsWorld {
private:
    b2WorldId m_worldId;

public:
    explicit Box2D_PhysicsWorld(Math::Vector2D gravity, int subStepCount = 4);
    ~Box2D_PhysicsWorld() override;
    void Step(float deltaTime) override;

    std::shared_ptr<I_RigidBody> CreateBox(
        Math::Vector2D position,
        float rotation,
        Math::Vector2D size,
        float mass,
        BodyType type
    ) override;

    std::shared_ptr<I_RigidBody> CreateCircle(
        Math::Vector2D position,
        float radius,
        float mass,
        BodyType type
    ) override;

    std::shared_ptr<I_RigidBody> CreateCapsule(
        Math::Vector2D position,
        float rotation,
        float height,
        float radius,
        float mass,
        BodyType type
    ) override;

    std::shared_ptr<I_RigidBody> CreateTriangle(
        Math::Vector2D position,
        float rotation,
        Math::Vector2D size,
        float mass,
        BodyType type
    ) override;

    std::vector<Math::Vector2D> CreateTerrainChain(
        const std::vector<Math::Vector2D>& controlPoints
    ) override;

    std::shared_ptr<I_Joint> CreateRevoluteJoint(
        std::shared_ptr<I_RigidBody> bodyA,
        std::shared_ptr<I_RigidBody> bodyB,
        Math::Vector2D anchor
    ) override;
};

#endif
