#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/common/Common.hpp"
#include <algorithm>
#include <cmath>

using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectCapsuleCapsuleCollision(
    const CapsuleRigidBody& bodyA,
    const CapsuleRigidBody& bodyB,
    Manifold& outManifold
) {
    outManifold.collisionPoints.clear();

    Transform transformA = bodyA.GetTransform();
    RealNum heightA = bodyA.GetHeight();
    RealNum radiusA = bodyA.GetRadius();
    RealNum halfSegA = max(RealNum(0), (heightA * 0.5f) - radiusA);

    Vector2D a1 = transformA.ToWorldPoint(Vector2D(0.0f, -halfSegA));
    Vector2D a2 = transformA.ToWorldPoint(Vector2D(0.0f, halfSegA));

    Transform transformB = bodyB.GetTransform();
    RealNum heightB = bodyB.GetHeight();
    RealNum radiusB = bodyB.GetRadius();
    RealNum halfSegB = max(RealNum(0), (heightB * 0.5f) - radiusB);

    Vector2D b1 = transformB.ToWorldPoint(Vector2D(0.0f, -halfSegB));
    Vector2D b2 = transformB.ToWorldPoint(Vector2D(0.0f, halfSegB));

    // find closest points between segment a1-a2 and segment b1-b2
    // via segment-segment closest points algorithm
    Vector2D d1 = a2 - a1;
    Vector2D d2 = b2 - b1;
    Vector2D r = a1 - b1;

    RealNum a = d1.Dot(d1);
    RealNum e = d2.Dot(d2);
    RealNum f = d2.Dot(r);

    RealNum s = 0.0f;
    RealNum t = 0.0f;

    if (a <= RealNum::epsilon && e <= RealNum::epsilon) {
        // both segments degenerate into points
        s = 0.0f;
        t = 0.0f;
    } else if (a <= RealNum::epsilon) {
        // segment A is a point
        s = 0.0f;
        t = clamp(f / e, RealNum(0), RealNum(1));
    } else {
        RealNum c = d1.Dot(r);
        if (e <= RealNum::epsilon) {
            // Segment B is a point
            t = 0.0f;
            s = clamp(-c / a, RealNum(0), RealNum(1));
        } else {
            RealNum b = d1.Dot(d2);
            RealNum denom = a * e - b * b;

            if (denom != 0.0f) {
                s = clamp((b * f - c * e) / denom, RealNum(0), RealNum(1));
            } else {
                s = 0.0f;
            }

            t = (b * s + f) / e;

            if (t < 0.0f) {
                t = 0.0f;
                s = clamp(-c / a, RealNum(0), RealNum(1));
            } else if (t > 1.0f) {
                t = 1.0f;
                s = clamp((b - c) / a, RealNum(0), RealNum(1));
            }
        }
    }

    Vector2D closestA = a1 + d1 * s;
    Vector2D closestB = b1 + d2 * t;

    // compute distance and depth between closest points
    Vector2D dir = closestA - closestB;
    RealNum distSq = dir.LengthSquared();
    RealNum combinedRadius = radiusA + radiusB;
    if (distSq >= combinedRadius * combinedRadius) return false;

    RealNum dist = sqrt(distSq);
    RealNum penetrationDepth = combinedRadius - dist;

    Vector2D normal;
    if (dist > RealNum::epsilon) {
        // normal points from Capsule A to Capsule B
        normal = (closestB - closestA) / dist;
    } else {
        // segments overlap completely
        Vector2D perp(-d1.y, d1.x);
        normal = perp.Normal().value_or(Vector2D::Right()); // fallback to perpendicular or right
    }

    Vector2D contactPosition = closestA + normal * radiusA;

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
