#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/common/CollisionPolygon.hpp"
#include "physics/engine/common/ContactCandidate.hpp"
#include <algorithm>
#include <vector>
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectPolygonPolygonCollision(
    const PolygonRigidBody& bodyA,
    const PolygonRigidBody& bodyB,
    Manifold& outManifold
) {
    outManifold.collisionPoints.clear();

    CollisionPolygon polyA = bodyA.GetCollisionPolygon();
    CollisionPolygon polyB = bodyB.GetCollisionPolygon();
    RealNum areaA = polyA.GetSignedArea();
    RealNum areaB = polyB.GetSignedArea();

    if (abs(areaA) <= RealNum::epsilon || abs(areaB) <= RealNum::epsilon)
        return false;

    bool isCCWA = areaA > 0.0f;
    bool isCCWB = areaB > 0.0f;

    Vector2D centerA = polyA.GetCenter();
    Vector2D centerB = polyB.GetCenter();

    RealNum minDepth = RealNum::Infinity();
    int bestEdgeIndex = -1;
    bool bestEdgeFromA = false;

    // test axes via separating axes theorem
    bool test1 = polyA.TestAxes(
        polyA, polyB, polyA, polyB, isCCWA, true, minDepth, bestEdgeIndex, bestEdgeFromA
    );
    if (!test1) return false;

    bool test2 = polyB.TestAxes(
        polyB, polyA, polyA, polyB, isCCWB, false, minDepth, bestEdgeIndex, bestEdgeFromA
    );
    if (!test2) return false;

    if (bestEdgeIndex < 0)
        return false;

    // determine the best edge normal for collision resolution
    Vector2D bestEdge = bestEdgeFromA
        ? polyA.GetOutwardNormal(bestEdgeIndex, isCCWA)
        : polyB.GetOutwardNormal(bestEdgeIndex, isCCWB);

    if ((centerB - centerA).Dot(bestEdge) < 0.0f)
        bestEdge = bestEdge * -1.0f;

    // get reference and incident polygons
    const CollisionPolygon& refPoly = bestEdgeFromA ? polyA : polyB;
    const CollisionPolygon& incPoly = bestEdgeFromA ? polyB : polyA;
    bool refIsCCW = bestEdgeFromA ? isCCWA : isCCWB;
    bool incIsCCW = bestEdgeFromA ? isCCWB : isCCWA;

    Vector2D refNormal = bestEdgeFromA
        ? bestEdge
        : bestEdge * -1.0f;

    // find the reference and incident edges for clipping
    int refIndex = refPoly.FindReferenceEdge(refNormal, refIsCCW);
    int incIndex = incPoly.FindIncidentEdge(refNormal, incIsCCW);
    if (refIndex < 0 || incIndex < 0)
        return false;

    Vector2D refA = refPoly.vertices[refIndex];
    Vector2D refB = refPoly.vertices[(refIndex + 1) % refPoly.vertices.size()];

    Vector2D incA = incPoly.vertices[incIndex];
    Vector2D incB = incPoly.vertices[(incIndex + 1) % incPoly.vertices.size()];

    Vector2D tangent = refB - refA;
    if (!tangent.TryNormalize())
        return false;

    CollisionPolygon clipped = CollisionPolygon::FromVertices({ incA, incB });

    RealNum sideOffsetA = -refA.Dot(tangent);
    RealNum sideOffsetB = refB.Dot(tangent);

    // clip the incident edge against the reference edge side planes
    clipped = clipped.ClipSegmentToLine(tangent * -1.0f, sideOffsetA);
    if (clipped.vertices.size() < 2) return false;

    clipped = clipped.ClipSegmentToLine(tangent, sideOffsetB);
    if (clipped.vertices.empty()) return false;

    RealNum referenceOffset = refA.Dot(refNormal);

    // evaluate the clipped points against the reference
    // edge plane and collect valid contact points
    vector<ContactCandidate> candidates;
    for (const Vector2D& point: clipped.vertices) {
        RealNum separation = point.Dot(refNormal) - referenceOffset;
        if (separation > RealNum::epsilon) continue;

        candidates.push_back({
            point,
            max(RealNum(0), -separation),
            point.Dot(tangent)
        });
    }
    if (candidates.empty()) return false;

    // sort candidates along the tangent direction
    sort(
        candidates.begin(), candidates.end(),
        [](const ContactCandidate& a, const ContactCandidate& b) {
            return a.tangentPos < b.tangentPos;
        }
    );

    int contactCount = min(2, static_cast<int>(candidates.size()));
    for (int i = 0; i < contactCount; ++i) {
        const ContactCandidate& candidate = candidates[i];

        CollisionPoint coll = CollisionPoint {
            .position = candidate.position,
            .normal = bestEdge,
            .depth = candidate.depth,
            .normalImpulse = 0.0f,
            .tangentImpulse = 0.0f
        };
        outManifold.collisionPoints.push_back(coll);
    }

    outManifold.material = Material::FromCombined(
        bodyA.GetMaterial(),
        bodyB.GetMaterial()
    );
    return !outManifold.collisionPoints.empty();
}
