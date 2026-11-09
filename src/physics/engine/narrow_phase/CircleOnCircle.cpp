#include "physics/engine/narrow_phase/NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/Common.hpp"
#include <cmath>
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectCircleCircleCollision(
     const CircleRigidBody& bodyA,
     const CircleRigidBody& bodyB,
     Manifold& outManifold
) {
    Vector2D p1 = bodyA.GetTransform().position;
    Vector2D p2 = bodyB.GetTransform().position;
    float r1 = bodyA.GetRadius();
    float r2 = bodyB.GetRadius();

    Vector2D dir = p2 - p1;
    float distanceSq = dir.LengthSquared();
    float radiusSum = r1 + r2;

    // if distance squared is greater than sum of radius squared, no collision
    if (distanceSq >= radiusSum * radiusSum) {
        return false;
    }
    float distance = sqrt(distanceSq);

    CollisionPoint coll;

    if (distance == 0.0f) {
        // edge case: concentric circles
        coll.normal = Vector2D::Right();
        coll.depth = radiusSum;
        coll.position = p1;
    } else {
        coll.normal = dir.Normal().value(); // guaranteed to be non-zero since distance > 0
        coll.depth = radiusSum - distance;
        coll.position = p1 + coll.normal * r1;
    }

    outManifold.collisionPoints.push_back(coll);
    return true;
}
