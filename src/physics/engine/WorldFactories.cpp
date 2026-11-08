#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/World.hpp"
#include <memory>
using namespace std;
using namespace Math;
using namespace Physics;

using PhysMat = Physics::Material;

std::shared_ptr<I_RigidBody> World::CreateBox(
    Math::Vector2D position,
    float rotation,
    Math::Vector2D size,
    float mass,
    BodyType type
) {
    PhysMat defaultMaterial;
    auto box = make_shared<BoxRigidBody>(
         position,
         rotation,
         size,
         mass,
         type,
         defaultMaterial
    );
    m_bodies.push_back(box);
    return box;
}

std::shared_ptr<I_RigidBody> World::CreateCircle(
    Math::Vector2D position,
    float radius,
    float mass,
    BodyType type
) {
    PhysMat defaultMaterial;
    auto circle = make_shared<CircleRigidBody>(
         position,
         radius,
         mass,
         type,
         defaultMaterial
    );
    m_bodies.push_back(circle);
    return circle;
}

std::shared_ptr<I_RigidBody> World::CreateCapsule(
    Math::Vector2D position,
    float rotation,
    float height,
    float radius,
    float mass,
    BodyType type
) {
    PhysMat defaultMaterial;
    auto capsule = make_shared<CapsuleRigidBody>(
         position,
         rotation,
         height,
         radius,
         mass,
         type,
         defaultMaterial
    );
    m_bodies.push_back(capsule);
    return capsule;
}

std::shared_ptr<I_RigidBody> World::CreateTriangle(
    Math::Vector2D position,
    float rotation,
    Math::Vector2D size,
    float mass,
    BodyType type
) {
    PhysMat defaultMaterial;
    auto triangle = make_shared<TriangleRigidBody>(
         position,
         rotation,
         size,
         mass,
         type,
         defaultMaterial
    );
    m_bodies.push_back(triangle);
    return triangle;
}

std::vector<Math::Vector2D> World::CreateTerrainChain(
    const std::vector<Math::Vector2D>& controlPoints,
    bool isLoop
) {

}

std::shared_ptr<I_Joint> World::CreateRevoluteJoint(
    std::shared_ptr<I_RigidBody> bodyA,
    std::shared_ptr<I_RigidBody> bodyB,
    Math::Vector2D anchor
) {

}

std::shared_ptr<I_Joint> World::CreateWeldJoint(
    std::shared_ptr<I_RigidBody> bodyA,
    std::shared_ptr<I_RigidBody> bodyB,
    Math::Vector2D anchor
) {

}
