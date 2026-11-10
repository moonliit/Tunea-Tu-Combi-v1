#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/BaseRigidBody.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/common/Common.hpp"
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectTriangleEdgeCollision(
    const TriangleRigidBody& bodyA,
    const EdgeRigidBody& bodyB,
    Manifold& outManifold
) {
    const PolygonRigidBody& polyA = bodyA;
    return DetectPolygonEdgeCollision(polyA, bodyB, outManifold);
}
