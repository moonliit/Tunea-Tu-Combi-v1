#include "physics/engine/narrow_phase/NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/Common.hpp"
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectCapsuleCapsuleCollision(
    const CapsuleRigidBody& bodyA,
    const CapsuleRigidBody& bodyB,
    Manifold& outManifold
) {
    // TODO:
    return false;
}
