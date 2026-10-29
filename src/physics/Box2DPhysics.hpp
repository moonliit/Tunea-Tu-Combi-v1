#ifndef BOX2D_PHYSICS_HPP
#define BOX2D_PHYSICS_HPP

#include "IPhysics.hpp"
#include <box2d/box2d.h>

class Box2DRigidBody : public IRigidBody {
private:
    b2BodyId m_bodyId;
    bool m_isCircle;
    Math::    Vector2D m_size;
    float m_radius;

public:
    explicit Box2DRigidBody(b2BodyId bodyId, Math::Vector2D size);
    explicit Box2DRigidBody(b2BodyId bodyId, float radius);
    virtual ~Box2DRigidBody() override;

    Math::    Vector2D GetPosition() const override;
    float GetRotation() const override;
    Math::    Vector2D GetSize() const override { return m_size; }
    float GetRadius() const override { return m_radius; }

    b2BodyId GetInternalId() const { return m_bodyId; }
};

class Box2DJoint : public IJoint {
private:
    b2JointId m_jointId;
public:
    explicit Box2DJoint(b2JointId jointId);
    b2JointId GetInternalId() const { return m_jointId; }
};

class Box2DWorld : public IPhysicsWorld {
private:
    b2WorldId m_worldId;

public:
    explicit Box2DWorld(Math::Vector2D gravity, int subStepCount = 4);
    ~Box2DWorld() override;
    void Step(float deltaTime) override;

    std::shared_ptr<IRigidBody> CreateBox(
        Math::Vector2D position,
        float rotation,
        Math::Vector2D size,
        float mass,
        BodyType type
    ) override;
    std::shared_ptr<IRigidBody> CreateCircle(
        Math::Vector2D position,
        float radius,
        float mass,
        BodyType type
    ) override;

    std::vector<Math::Vector2D> CreateTerrainChain(
        const std::vector<Math::Vector2D>& controlPoints
    ) override;

    std::shared_ptr<IJoint> CreateRevoluteJoint(
        std::shared_ptr<IRigidBody> bodyA,
        std::shared_ptr<IRigidBody> bodyB,
        Math::Vector2D anchor
    ) override;
};

#endif
