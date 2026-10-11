#include "physics/Box2DPhysics.hpp"
#include <box2d-lite/World.h>
#include <box2d-lite/Body.h>
#include <cfloat>

// --- RigidBody Implementation ---
Box2DRigidBody::Box2DRigidBody(Body* body) : m_body(body) {}

Vector2D Box2DRigidBody::GetPosition() const {
    // Assuming box2d-lite body stores position as a Vec2 (position.x, position.y)
    return Vector2D{ m_body->position.x, m_body->position.y };
}

Vector2D Box2DRigidBody::GetSize() const {
    return Vector2D{ m_body->width.x, m_body->width.y };
}

float Box2DRigidBody::GetRotation() const {
    return m_body->rotation; // Assuming angle/rotation field
}

// --- World Implementation ---
Box2DWorld::Box2DWorld(Vector2D gravity, int iterations)
    : m_world(new World(Vec2(gravity.x, gravity.y), iterations)) {}

Box2DWorld::~Box2DWorld() {
    if (m_world) delete m_world;
}

void Box2DWorld::Step(float deltaTime) {
    m_world->Step(deltaTime);
}

std::shared_ptr<IRigidBody> Box2DWorld::CreateBox(Vector2D position, Vector2D size, float mass, BodyType type) {
    auto* body = new Body();

    if (type == BodyType::Static) {
        body->Set(Vec2(size.x, size.y), FLT_MAX);
        body->position.Set(position.x, position.y);
    } else {
        body->Set(Vec2(size.x, size.y), mass);
        body->position.Set(position.x, position.y);
    }

    m_world->Add(body);
    return std::make_shared<Box2DRigidBody>(body);
}
