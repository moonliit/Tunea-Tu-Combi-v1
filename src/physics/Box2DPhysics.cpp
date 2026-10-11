#include "physics/Box2DPhysics.hpp"
#include <box2d-lite/World.h>
#include <box2d-lite/Joint.h>
#include <box2d-lite/Body.h>
#include <cfloat>

// --- RigidBody Implementation ---
Box2DRigidBody::Box2DRigidBody(Body* body, Vector2D size)
    : m_body(body), m_isCircle(false), m_size(size), m_radius(0.0f) {}

Box2DRigidBody::Box2DRigidBody(Body* body, float radius)
    : m_body(body), m_isCircle(true), m_size{radius * 2.0f, radius * 2.0f}, m_radius(radius) {}

Vector2D Box2DRigidBody::GetPosition() const {
    // Assuming box2d-lite body stores position as a Vec2 (position.x, position.y)
    return Vector2D{ m_body->position.x, m_body->position.y };
}

float Box2DRigidBody::GetRotation() const {
    return m_body->rotation; // Assuming angle/rotation field
}

// --- Joint Implementation ---
Box2DJoint::Box2DJoint(Joint* joint) : m_joint(joint) {}

// --- World Implementation ---
Box2DWorld::Box2DWorld(Vector2D gravity, int iterations)
    : m_world(new World(Vec2(gravity.x, gravity.y), iterations)) {}

Box2DWorld::~Box2DWorld() {
    if (m_world) delete m_world;
}

void Box2DWorld::Step(float deltaTime) {
    m_world->Step(deltaTime);
}

std::shared_ptr<IRigidBody> Box2DWorld::CreateBox(Vector2D position, float rotation, Vector2D size, float mass, BodyType type) {
    auto* body = new Body();

    if (type == BodyType::Static) {
        body->Set(Vec2(size.x, size.y), FLT_MAX);
        body->position.Set(position.x, position.y);
        body->rotation = rotation;
    } else {
        body->Set(Vec2(size.x, size.y), mass);
        body->position.Set(position.x, position.y);
        body->rotation = rotation;
    }

    m_world->Add(body);
    return std::make_shared<Box2DRigidBody>(body, size);
}

std::shared_ptr<IRigidBody> Box2DWorld::CreateCircle(Vector2D position, float radius, float mass, BodyType type) {
    auto* body = new Body();

    // In box2d-lite, circles are often instantiated by passing width/height equal to diameter,
    // or via a shape flag depending on the fork. Adjust if your fork has a dedicated circle Set method.
    float diameter = radius * 2.0f;
    if (type == BodyType::Static) {
        body->Set(Vec2(diameter, diameter), FLT_MAX);
    } else {
        body->Set(Vec2(diameter, diameter), mass);
    }
    body->position.Set(position.x, position.y);

    m_world->Add(body);
    return std::make_shared<Box2DRigidBody>(body, radius);
}

std::shared_ptr<IJoint> Box2DWorld::CreateRevoluteJoint(
    std::shared_ptr<IRigidBody> bodyA,
    std::shared_ptr<IRigidBody> bodyB,
    Vector2D worldAnchor
) {
    auto bA = std::dynamic_pointer_cast<Box2DRigidBody>(bodyA);
    auto bB = std::dynamic_pointer_cast<Box2DRigidBody>(bodyB);

    if (!bA || !bB) return nullptr;

    auto* joint = new Joint();
    joint->Set(bA->GetInternalBody(), bB->GetInternalBody(), Vec2(worldAnchor.x, worldAnchor.y));

    m_world->Add(joint); // Assuming m_world has an Add(Joint*) method in your box2d-lite version
    return std::make_shared<Box2DJoint>(joint);
}
