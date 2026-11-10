#include "physics/engine/BaseRigidBody.hpp"
#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/common/Common.hpp"
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectBoxTriangleCollision(
    const BoxRigidBody& bodyA,
    const TriangleRigidBody& bodyB,
    Manifold& outManifold
) {
    const PolygonRigidBody& polyA = bodyA;
    const PolygonRigidBody& polyB = bodyB;
    return DetectPolygonPolygonCollision(polyA, polyB, outManifold);
}
