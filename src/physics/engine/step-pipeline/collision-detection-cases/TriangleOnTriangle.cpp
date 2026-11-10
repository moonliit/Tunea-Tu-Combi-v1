#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/common/Clipping.hpp"
#include "math/SafeReal.hpp"
#include "math/Interval.hpp"
#include <array>
#include <cmath>
using namespace std;
using namespace Math;

RealNum GetSignedArea(const array<Vector2D, 3>& vertices) {
    RealNum area = 0.0f;
    for (int i = 0; i < 3; ++i) {
        int next = (i + 1) % 3;
        area += vertices[i].Cross(vertices[next]);
    }
    return area;
}

Vector2D GetOutwardNormal(const array<Vector2D, 3>& vertices, int edgeIndex, bool isCCW) {
    int next = (edgeIndex + 1) % 3;
    Vector2D edge = vertices[next] - vertices[edgeIndex];
    Vector2D normal = isCCW
        ? Vector2D(edge.y, -edge.x)
        : Vector2D(-edge.y, edge.x);

    normal.TryNormalize();
    return normal;
}

Interval ProjectTriangle(const array<Vector2D, 3>& vertices, const Vector2D& axis) {
    RealNum minProjection = RealNum::Infinity();
    RealNum maxProjection = -RealNum::Infinity();

    for (const Vector2D& vertex : vertices) {
        RealNum projection = vertex.Dot(axis);
        minProjection = min(minProjection, projection);
        maxProjection = max(maxProjection, projection);
    }
    return Interval(minProjection, maxProjection);
}

// sat axis testing helper
bool TestAxes(
    const array<Vector2D, 3>& sourceVertices,
    const array<Vector2D, 3>& otherVertices,
    const array<Vector2D, 3>& verticesA,
    const array<Vector2D, 3>& verticesB,
    bool sourceIsA,
    RealNum& minDepth,
    int& bestEdgeIndex,
    bool& bestEdgeFromA
) {
    for (int i = 0; i < 3; ++i) {
        int next = (i + 1) % 3;
        Vector2D edge = sourceVertices[next] - sourceVertices[i];
        Vector2D axis(-edge.y, edge.x);

        if (!axis.TryNormalize())
            continue;

        Interval intervalA = ProjectTriangle(verticesA, axis);
        Interval intervalB = ProjectTriangle(verticesB, axis);

        auto overlapOpt = intervalA.GetOverlap(intervalB);
        if (!overlapOpt.has_value())
            return false;

        RealNum overlap = overlapOpt.value();
        if (overlap < minDepth) {
            minDepth = overlap;
            bestEdgeIndex = i;
            bestEdgeFromA = sourceIsA;
        }
    }
    return true;
};

int FindIncidentEdge(const array<Vector2D, 3>& vertices, const Vector2D& referenceNormal, bool isCCW) {
    RealNum minDot = RealNum::Infinity();
    int bestEdge = -1;

    for (int i = 0; i < 3; ++i) {
        Vector2D normal = GetOutwardNormal(vertices, i, isCCW);
        RealNum dot = normal.Dot(referenceNormal);
        if (dot < minDot) {
            minDot = dot;
            bestEdge = i;
        }
    }
    return bestEdge;
}

bool Physics::NarrowPhase::DetectTriangleTriangleCollision(
    const TriangleRigidBody& bodyA,
    const TriangleRigidBody& bodyB,
    Manifold& outManifold
) {
    outManifold.collisionPoints.clear();

    // get world vertices and check windings
    array<Vector2D, 3> verticesA = bodyA.GetWorldVertices();
    array<Vector2D, 3> verticesB = bodyB.GetWorldVertices();

    RealNum areaA = GetSignedArea(verticesA);
    RealNum areaB = GetSignedArea(verticesB);

    if (abs(areaA) <= RealNum::epsilon || abs(areaB) <= RealNum::epsilon)
        return false;

    bool isCCWA = areaA > 0.0f;
    bool isCCWB = areaB > 0.0f;

    RealNum minDepth = RealNum::Infinity();
    int bestEdgeIndex = -1;
    bool bestEdgeFromA = false;

    bool test1 = TestAxes(
        verticesA, // source
        verticesB, // other
        verticesA, // verticesA
        verticesB, // verticesB
        true,
        minDepth,
        bestEdgeIndex,
        bestEdgeFromA
    );
    if (!test1) return false;

    bool test2 = TestAxes(
        verticesB, // source
        verticesA, // other
        verticesA, // verticesA
        verticesB, // verticesB
        false,
        minDepth,
        bestEdgeIndex,
        bestEdgeFromA
    );
    if (!test2) return false;

    if (bestEdgeIndex < 0) return false;

    const auto& refVertices = bestEdgeFromA ? verticesA : verticesB;
    const auto& incVertices = bestEdgeFromA ? verticesB : verticesA;

    // determine the reference and incident edges then perform clipping
    bool refIsCCW = bestEdgeFromA ? isCCWA : isCCWB;
    bool incIsCCW = bestEdgeFromA ? isCCWB : isCCWA;

    int refNext = (bestEdgeIndex + 1) % 3;
    Vector2D refV0 = refVertices[bestEdgeIndex];
    Vector2D refV1 = refVertices[refNext];
    Vector2D refNormal = GetOutwardNormal(refVertices, bestEdgeIndex, refIsCCW);

    int incidentEdge = FindIncidentEdge(incVertices, refNormal, incIsCCW);
    if (incidentEdge < 0) return false;

    int incNext = (incidentEdge + 1) % 3;
    Vector2D incidentFace[2] = {
        incVertices[incidentEdge],
        incVertices[incNext]
    };

    Vector2D tangent = refV1 - refV0;
    if (!tangent.TryNormalize())
        return false;

    RealNum sideOffset1 = refV0.Dot(tangent);
    RealNum sideOffset2 = refV1.Dot(tangent);

    // clip incident edge against side planes
    Vector2D clipped1[2];
    int count = ClipSegmentToLine(incidentFace, clipped1, tangent * -1.0f, -sideOffset1);
    if (count < 1) return false;

    Vector2D clipped2[2];
    count = ClipSegmentToLine(clipped1, clipped2, tangent, sideOffset2);
    if (count < 1) return false;

    RealNum frontPlaneOffset = refV0.Dot(refNormal);
    Vector2D collisionNormal = bestEdgeFromA ? refNormal : refNormal * -1.0f;

    for (int i = 0; i < count; ++i) {
        RealNum separation = clipped2[i].Dot(refNormal) - frontPlaneOffset;
        if (separation > 0.0f) continue;

        CollisionPoint coll;
        coll.position = clipped2[i];
        coll.normal = collisionNormal;
        coll.depth = abs(separation);
        coll.normalImpulse = 0.0f;
        coll.tangentImpulse = 0.0f;

        outManifold.collisionPoints.push_back(coll);
        if (outManifold.collisionPoints.size() >= 2) break;
    }

    outManifold.material = Material::FromCombined(
        bodyA.GetMaterial(),
        bodyB.GetMaterial()
    );
    return !outManifold.collisionPoints.empty();
}
