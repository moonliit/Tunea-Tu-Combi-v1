#ifndef BOX2D_PHYSICS_HPP
#define BOX2D_PHYSICS_HPP

#include "IPhysics.hpp"

class World;
class Body;

// Concrete body wrapper for box2d-lite
class Box2DRigidBody : public IRigidBody {
private:
    Body* m_body = nullptr; // Pointer to underlying box2d-lite body

public:
    explicit Box2DRigidBody(Body* body);
    Vector2D GetPosition() const override;
    Vector2D GetSize() const override;
    float GetRotation() const override;

    Body* GetInternalBody() const { return m_body; }
};

// Concrete world wrapper for box2d-lite
class Box2DWorld : public IPhysicsWorld {
private:
    World* m_world = nullptr;

public:
    explicit Box2DWorld(Vector2D gravity, int iterations = 10);
    ~Box2DWorld();

    void Step(float deltaTime) override;
    std::shared_ptr<IRigidBody> CreateBox(Vector2D position, Vector2D size, float mass, BodyType type) override;
};

#endif
