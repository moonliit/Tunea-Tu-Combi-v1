#ifndef BOX2D_PHYSICS_HPP
#define BOX2D_PHYSICS_HPP

#include "IPhysics.hpp"

class Body;
class Joint;
class World;

// Concrete body wrapper for box2d-lite
class Box2DRigidBody : public IRigidBody {
private:
    Body* m_body = nullptr; // Pointer to underlying box2d-lite body
    bool m_isCircle;
    Vector2D m_size;
    float m_radius;

public:
    explicit Box2DRigidBody(Body* body, Vector2D size);
    explicit Box2DRigidBody(Body* body, float radius);

    Vector2D GetPosition() const override;
    float GetRotation() const override;
    Vector2D GetSize() const override { return m_size; }
    float GetRadius() const override { return m_radius; }

    Body* GetInternalBody() const { return m_body; }
};

class Box2DJoint : public IJoint {
private:
    Joint* m_joint;
public:
    explicit Box2DJoint(Joint* joint);
    Joint* GetInternalJoint() const { return m_joint; }
};

// Concrete world wrapper for box2d-lite
class Box2DWorld : public IPhysicsWorld {
private:
    World* m_world = nullptr;

public:
    explicit Box2DWorld(Vector2D gravity, int iterations = 10);
    ~Box2DWorld();
    void Step(float deltaTime) override;

    std::shared_ptr<IRigidBody> CreateBox(Vector2D position, float rotation, Vector2D size, float mass, BodyType type) override;
    std::shared_ptr<IRigidBody> CreateCircle(Vector2D position, float radius, float mass, BodyType type) override;

    std::shared_ptr<IJoint> CreateRevoluteJoint(
        std::shared_ptr<IRigidBody> bodyA,
        std::shared_ptr<IRigidBody> bodyB,
        Vector2D anchor
    ) override;
};

#endif
