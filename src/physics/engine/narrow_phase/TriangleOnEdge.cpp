#include "physics/engine/narrow_phase/NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/Common.hpp"
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectTriangleEdgeCollision(
    const TriangleRigidBody& bodyA,
    const EdgeRigidBody& bodyB,
    Manifold& outManifold
) {
    // TODO:
    return false;
}
