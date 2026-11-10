#include "math/SafeReal.hpp"
#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/common/CollisionPolygon.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
using namespace std;
using namespace Math;

bool EvaluateCircleAgainstPolygon(
    const Vector2D& center,
    const Physics::CollisionPolygon& poly,
    Math::RealNum radius,
    bool isCCW,
    Physics::Manifold& subManifold
) {
    Vector2D closestPoint;
    Vector2D closestOutwardNormal;
    RealNum minDistanceSq = RealNum::Infinity();
    bool inside = true;

    size_t n = poly.vertices.size();
    for (size_t i = 0; i < n; ++i) {
        const Vector2D& v1 = poly.vertices[i];
        const Vector2D& v2 = poly.vertices[(i + 1) % n];

        Vector2D edge = v2 - v1;
        RealNum edgeLengthSq = edge.LengthSquared();
        if (edgeLengthSq <= RealNum::epsilon) continue;

        Vector2D outwardNormal = poly.GetOutwardNormal(i, isCCW);
        if (!outwardNormal.TryNormalize()) continue;

        RealNum signedDistance = (center - v1).Dot(outwardNormal);
        if (signedDistance > RealNum::epsilon) inside = false;

        RealNum t = (center - v1).Dot(edge) / edgeLengthSq;
        t = clamp(t, RealNum(0), RealNum(1));

        Vector2D projection = v1 + edge * t;
        RealNum distanceSq = (center - projection).LengthSquared();
        if (distanceSq < minDistanceSq) {
            minDistanceSq = distanceSq;
            closestPoint = projection;
            closestOutwardNormal = outwardNormal;
        }
    }
    if (minDistanceSq == RealNum::Infinity()) return false;

    RealNum distance = sqrt(minDistanceSq);
    Vector2D normal;
    RealNum penetrationDepth;

    if (inside) {
        normal = closestOutwardNormal;
        penetrationDepth = radius + distance;
    } else {
        if (distance >= radius) return false;
        penetrationDepth = radius - distance;
        normal = (distance > RealNum::epsilon)
            ? (center - closestPoint) / distance
            : closestOutwardNormal;
    }

    Physics::CollisionPoint coll{
        .position = closestPoint,
        .normal = normal,
        .depth = penetrationDepth,
        .normalImpulse = 0.0f,
        .tangentImpulse = 0.0f
    };
    subManifold.collisionPoints.push_back(coll);
    return true;
};

bool Physics::NarrowPhase::DetectPolygonCapsuleCollision(
    const PolygonRigidBody& bodyA,
    const CapsuleRigidBody& bodyB,
    Manifold& outManifold
) {
    outManifold.collisionPoints.clear();

    Transform transformB = bodyB.GetTransform();
    RealNum totalHeight = bodyB.GetHeight();
    RealNum radius = bodyB.GetRadius();
    RealNum halfSegment = max(RealNum(0.0f), (totalHeight * 0.5f) - radius);

    Vector2D localTop(0.0f, halfSegment);
    Vector2D localBottom(0.0f, -halfSegment);
    Vector2D capsuleBotWorld = transformB.ToWorldPoint(localBottom); // capsule bottom center
    Vector2D capsuleTopWorld = transformB.ToWorldPoint(localTop); // capsule top center

    CollisionPolygon polyA = bodyA.GetCollisionPolygon();
    const auto& vertices = polyA.vertices;
    size_t n = vertices.size();

    RealNum signedArea = polyA.GetSignedArea();
    if (abs(signedArea) <= RealNum::epsilon) return false;
    bool isCCW = signedArea > 0.0f;

    // test both end caps of the capsule against the polygon
    Manifold manifoldBottom, manifoldTop;

    bool hitBottom = EvaluateCircleAgainstPolygon(
        capsuleBotWorld,
        polyA,
        radius,
        isCCW,
        manifoldBottom
    );
    bool hitTop = EvaluateCircleAgainstPolygon(
        capsuleTopWorld,
        polyA,
        radius,
        isCCW,
        manifoldTop
    );

    if (hitBottom) {
        for (const auto& coll: manifoldBottom.collisionPoints) {
            outManifold.collisionPoints.push_back(coll);
        }
    }
    if (hitTop) {
        for (const auto& coll: manifoldTop.collisionPoints) {
            bool duplicate = false;
            for (const auto& existing : outManifold.collisionPoints) {
                if ((existing.position - coll.position).LengthSquared() < 1.0f) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                outManifold.collisionPoints.push_back(coll);
            }
        }
    }
    if (outManifold.collisionPoints.empty()) return false;

    outManifold.material = Material::FromCombined(
        bodyA.GetMaterial(),
        bodyB.GetMaterial()
    );
    return true;
}
