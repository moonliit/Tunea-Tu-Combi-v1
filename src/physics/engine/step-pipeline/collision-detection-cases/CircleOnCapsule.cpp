#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/common/Common.hpp"
#include <algorithm>
#include <cmath>

using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectCircleCapsuleCollision(
    const CircleRigidBody& bodyA,
    const CapsuleRigidBody& bodyB,
    Manifold& outManifold
) {
    outManifold.collisionPoints.clear();

    Vector2D circleCenter = bodyA.GetTransform().position;
    RealNum circleRadius = bodyA.GetRadius();

    Transform transformB = bodyB.GetTransform();
    RealNum totalHeight = bodyB.GetHeight();
    RealNum capsuleRadius = bodyB.GetRadius();
    RealNum halfSegment = max(RealNum(0), (totalHeight * 0.5f) - capsuleRadius);

    Vector2D localTop(0.0f, halfSegment);
    Vector2D localBottom(0.0f, -halfSegment);
    Vector2D p1 = transformB.ToWorldPoint(localBottom);
    Vector2D p2 = transformB.ToWorldPoint(localTop);

    // find closest point from circle center to capsule line segment
    // (p1 - p2)
    Vector2D ab = p2 - p1;
    RealNum abLenSq = ab.LengthSquared();

    Vector2D closestPointOnSegment;
    RealNum t = 0.0f;

    if (abLenSq <= RealNum::epsilon) {
        closestPointOnSegment = p1;
    } else {
        t = (circleCenter - p1).Dot(ab) / abLenSq;
        t = clamp(t, RealNum(0), RealNum(1));
        closestPointOnSegment = p1 + ab * t;
    }

    // compute distance from circle center to the closest point on the capsule segment
    Vector2D dir = circleCenter - closestPointOnSegment;
    RealNum distSq = dir.LengthSquared();
    RealNum combinedRadius = circleRadius + capsuleRadius;
    if (distSq >= combinedRadius * combinedRadius) return false;

    RealNum dist = sqrt(distSq);
    RealNum penetrationDepth = combinedRadius - dist;

    Vector2D normal;
    if (dist > RealNum::epsilon) {
        // normal points from circle to capsule
        normal = dir / dist;
    } else {
        // centers overlap completely
        // fallback to perpendicular to segment or right vector
        Vector2D perp(-ab.y, ab.x);
        normal = perp.Normal().value_or(Vector2D::Right());
    }

    // construct the contact point (approximate contact point between surfaces)
    Vector2D contactPosition = closestPointOnSegment + normal * capsuleRadius;

    CollisionPoint coll{
        .position = contactPosition,
        .normal = normal,
        .depth = penetrationDepth,
        .normalImpulse = 0.0f,
        .tangentImpulse = 0.0f
    };
    outManifold.collisionPoints.push_back(coll);

    outManifold.material = Material::FromCombined(
        bodyA.GetMaterial(),
        bodyB.GetMaterial()
    );
    return true;
}
