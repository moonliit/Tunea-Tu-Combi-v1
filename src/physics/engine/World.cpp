#include "physics/engine/World.hpp"
using namespace std;
using namespace Math;
using namespace Physics;

World::World(Math::Vector2D gravity, int subStepCount) {

}

World::~World() {

}

void World::Step(float deltaTime) {

}

std::shared_ptr<I_RigidBody> CreateBox(
    Math::Vector2D position,
    float rotation,
    Math::Vector2D size,
    float mass,
    BodyType type
) {

}

std::shared_ptr<I_RigidBody> CreateCircle(
    Math::Vector2D position,
    float radius,
    float mass,
    BodyType type
) {

}

std::shared_ptr<I_RigidBody> CreateCapsule(
    Math::Vector2D position,
    float rotation,
    float height,
    float radius,
    float mass,
    BodyType type
) {

}

std::shared_ptr<I_RigidBody> CreateTriangle(
    Math::Vector2D position,
    float rotation,
    Math::Vector2D size,
    float mass,
    BodyType type
) {

}

std::vector<Math::Vector2D> CreateTerrainChain(
    const std::vector<Math::Vector2D>& controlPoints,
    bool isLoop = false
) {

}

std::shared_ptr<I_Joint> CreateRevoluteJoint(
    std::shared_ptr<I_RigidBody> bodyA,
    std::shared_ptr<I_RigidBody> bodyB,
    Math::Vector2D anchor
) {

}

std::shared_ptr<I_Joint> CreateWeldJoint(
    std::shared_ptr<I_RigidBody> bodyA,
    std::shared_ptr<I_RigidBody> bodyB,
    Math::Vector2D anchor
) {

}
