#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/common/Common.hpp"
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectCircleEdgeCollision(
    const CircleRigidBody& bodyA,
    const EdgeRigidBody& bodyB,
    Manifold& outManifold
) {
    Vector2D center = bodyA.GetTransform().position;
    RealNum radius = bodyA.GetRadius();

    Vector2D p1 = bodyB.GetP1();
    Vector2D p2 = bodyB.GetP2();

    // project circle center onto segment (p1, p2)
    Vector2D ab = p2 - p1;
    RealNum abLenSq = ab.LengthSquared();
    if (abLenSq < RealNum::epsilon) return false;

    RealNum t = (center - p1).Dot(ab) / abLenSq;
    t = clamp(t, RealNum(0), RealNum(1));

    Vector2D closestPoint = p1 + ab * t;
    Vector2D dir = center - closestPoint;
    RealNum distSq = dir.LengthSquared();

    if (distSq >= radius * radius) {
        return false;
    }

    RealNum dist = std::sqrt(distSq);
    CollisionPoint coll;

    if (dist < RealNum::epsilon) {
        coll.normal = bodyB.GetNormal();
        if (coll.normal.y > 0.0f) {
            coll.normal *= -1;
        }
        coll.depth = radius;
        coll.position = closestPoint;
    } else {
        coll.normal = dir * (1.0f / dist);
        // ensure normal points out of the terrain
        if (coll.normal.Dot(bodyB.GetNormal()) < 0.0f) {
            coll.normal = bodyB.GetNormal() * -1.0f;
        }
        coll.depth = radius - dist;
        coll.position = closestPoint;
    }

    outManifold.collisionPoints.push_back(coll);
    return true;
}
