#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/BaseRigidBody.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/common/Common.hpp"
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectBoxTerrainMeshCollision(
    const BoxRigidBody& bodyA,
    const TerrainMeshRigidBody& bodyB,
    Manifold& outManifold
) {
    const PolygonRigidBody& polyA = bodyA;
    const PolygonRigidBody& polyB = bodyB;
    if (!DetectPolygonPolygonCollision(polyA, polyB, outManifold)) {
        return false;
    }
    for (CollisionPoint& coll : outManifold.collisionPoints) {
        coll.normal = coll.normal * -1;
    }
    return true;
}
