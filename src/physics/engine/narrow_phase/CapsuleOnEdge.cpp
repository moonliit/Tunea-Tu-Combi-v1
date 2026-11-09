#include "physics/engine/narrow_phase/NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/Common.hpp"
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectCapsuleEdgeCollision(
    const CapsuleRigidBody& bodyA,
    const EdgeRigidBody& bodyB,
    Manifold& outManifold
) {
    // TODO:
    return false;
}
