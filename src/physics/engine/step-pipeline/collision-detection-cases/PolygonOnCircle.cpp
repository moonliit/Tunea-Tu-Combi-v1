#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/common/CollisionPolygon.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/RigidBodies.hpp"
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectPolygonCircleCollision(
    const PolygonRigidBody& bodyA,
    const CircleRigidBody& bodyB,
    Manifold& outManifold
) {
    outManifold.collisionPoints.clear();

    const Vector2D center = bodyB.GetTransform().position;
    const RealNum radius = bodyB.GetRadius();

    CollisionPolygon poly = bodyA.GetCollisionPolygon();

    // get polygon winding .
    RealNum signedArea = poly.GetSignedArea();
    if (abs(signedArea) <= RealNum::epsilon) return false;
    const bool isCCW = signedArea > 0.0f;

    // find the closest point on the polygon perimeter.
    Vector2D closestPoint;
    Vector2D closestOutwardNormal;

    RealNum minDistanceSq = RealNum::Infinity();
    bool inside = true;

    size_t n = poly.vertices.size();
    for (size_t i = 0; i < n; ++i) {
        const Vector2D& p1 = poly.vertices[i];
        const Vector2D& p2 = poly.vertices[(i + 1) % n];

        const Vector2D edge = p2 - p1;
        const RealNum edgeLengthSq = edge.LengthSquared();
        if (edgeLengthSq <= RealNum::epsilon)
            continue;

        Vector2D outwardNormal = poly.GetOutwardNormal(i, isCCW);
        if (!outwardNormal.TryNormalize())
            continue;

        // check whether the circle center is inside the polygon.
        const RealNum signedDistance = (center - p1).Dot(outwardNormal);
        if (signedDistance > RealNum::epsilon) inside = false;

        // project the center onto the edge segment.
        RealNum t = (center - p1).Dot(edge) / edgeLengthSq;
        t = clamp(t, RealNum(0), RealNum(1));

        const Vector2D projection = p1 + edge * t;
        const RealNum distanceSq = (center - projection).LengthSquared();

        if (distanceSq < minDistanceSq) {
            minDistanceSq = distanceSq;
            closestPoint = projection;
            closestOutwardNormal = outwardNormal;
        }
    }
    if (minDistanceSq == RealNum::Infinity()) return false;

    // check collision and determine contact normal
    // (pointing from Polygon A to Circle B) and depth.
    const RealNum distance = sqrt(minDistanceSq);

    Vector2D normal;
    RealNum penetrationDepth;

    if (inside) {
        // circle center is inside the polygon
        // normal points outward from the polygon (from A to B).
        normal = closestOutwardNormal;
        penetrationDepth = radius + distance;
    } else {
        // circle center is outside the polygon.
        if (distance >= radius)
            return false;

        penetrationDepth = radius - distance;

        if (distance > RealNum::epsilon) {
            normal = (center - closestPoint) / distance;
        } else {
            // center is exactly on the boundary.
            normal = closestOutwardNormal;
        }
    }

    // Vector2D polyCenter = poly.GetCenter();
    // Vector2D expectedDir = center - polyCenter;
    //
    // if (expectedDir.TryNormalize() && normal.TryNormalize()) {
    //     RealNum alignment = expectedDir.Dot(normal);
    //
    //     if (abs(alignment) < RealNum(0.2)) {
    //         // normal is not aligned with the expected direction.
    //         // flip the normal to point from A to B.
    //         Vector2D fallbackNormal = center - closestPoint;
    //         if (fallbackNormal.TryNormalize()) {
    //             normal = fallbackNormal;
    //         }
    //     }
    //
    //     // Final directional check to ensure it points A -> B
    //     if (expectedDir.Dot(normal) < 0.0f) {
    //         normal = normal * -1.0f;
    //     }
    // }

    CollisionPoint coll{
        .position = closestPoint,
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
