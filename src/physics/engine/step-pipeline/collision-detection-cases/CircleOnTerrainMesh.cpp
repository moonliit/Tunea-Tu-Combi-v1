#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/BaseRigidBody.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/common/Common.hpp"
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectCircleTerrainMeshCollision(
    const CircleRigidBody& bodyA,
    const TerrainMeshRigidBody& bodyB,
    Manifold& outManifold
) {
    const PolygonRigidBody& polyB = bodyB;
    return DetectPolygonCircleCollision(polyB, bodyA, outManifold);
}
