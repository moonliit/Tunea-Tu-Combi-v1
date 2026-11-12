#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/BaseRigidBody.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/common/Common.hpp"
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectCircleTriangleCollision(
    const CircleRigidBody& bodyA,
    const TriangleRigidBody& bodyB,
    Manifold& outManifold
) {
    const PolygonRigidBody& polyB = bodyB;
    if(!DetectPolygonCircleCollision(polyB, bodyA, outManifold)) {
        return false;
    }
    for (CollisionPoint& coll : outManifold.collisionPoints) {
        coll.normal = coll.normal * -1;
    }
    return true;
}
