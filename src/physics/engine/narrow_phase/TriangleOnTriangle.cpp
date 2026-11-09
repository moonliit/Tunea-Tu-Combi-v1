#include "physics/engine/narrow_phase/NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/Common.hpp"
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectTriangleTriangleCollision(
    const TriangleRigidBody& bodyA,
    const TriangleRigidBody& bodyB,
    Manifold& outManifold
) {
    // TODO:
    return false;
}
