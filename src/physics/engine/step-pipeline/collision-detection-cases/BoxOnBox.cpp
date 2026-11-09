#include "math/SafeReal.hpp"
#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/common/Clipping.hpp"
#include "math/Interval.hpp"
#include <vector>
#include <cmath>
using namespace std;
using namespace Math;

// function to find the incident face (box face most antiparallel to the reference normal)
void FindIncidentFace(
    const Vector2D& centerB, const Vector2D& halfB,
    const Vector2D& axisB0, const Vector2D& axisB1,
    const Vector2D& referenceNormal,
    Vector2D outVertices[2]
) {
    // get normal in B local space
    Vector2D localNormal = Vector2D(referenceNormal.Dot(axisB0), referenceNormal.Dot(axisB1));

    // choose the incident face based on local normal signs
    Vector2D normalSign(
        localNormal.x < 0.0f ? 1.0f : -1.0f,
        localNormal.y < 0.0f ? 1.0f : -1.0f
    );

    if (std::abs(localNormal.x) > std::abs(localNormal.y)) {
        outVertices[0] = centerB + axisB0 * (halfB.x * normalSign.x) + axisB1 * halfB.y;
        outVertices[1] = centerB + axisB0 * (halfB.x * normalSign.x) - axisB1 * halfB.y;
    } else {
        outVertices[0] = centerB + axisB0 * halfB.x + axisB1 * (halfB.y * normalSign.y);
        outVertices[1] = centerB - axisB0 * halfB.x + axisB1 * (halfB.y * normalSign.y);
    }
}

// function to project a box onto an axis and get min/max intervals
Math::Interval ProjectBox(
    const Vector2D& center,
    const Vector2D& axis,
    const Vector2D& halfSize,
    const Vector2D& ax0,
    const Vector2D& ax1
) {
    RealNum p0 = center.Dot(axis);
    RealNum r = (
        halfSize.x * std::abs(ax0.Dot(axis)) +
        halfSize.y * std::abs(ax1.Dot(axis))
    );
    return Math::Interval(p0 - r, p0 + r);
};

bool Physics::NarrowPhase::DetectBoxBoxCollision(
    const BoxRigidBody& bodyA,
    const BoxRigidBody& bodyB,
    Manifold& outManifold
) {
    outManifold.collisionPoints.clear();

    Transform tA = bodyA.GetTransform();
    Transform tB = bodyB.GetTransform();

    Vector2D sizeA = bodyA.GetSize();
    Vector2D sizeB = bodyB.GetSize();

    Vector2D halfA = sizeA * 0.5f;
    Vector2D halfB = sizeB * 0.5f;

    // transform local axes to world space
    Vector2D axisA0 = tA.ToWorldVector(Vector2D::Right());
    Vector2D axisA1 = tA.ToWorldVector(Vector2D::Up());

    Vector2D axisB0 = tB.ToWorldVector(Vector2D::Right());
    Vector2D axisB1 = tB.ToWorldVector(Vector2D::Up());

    Vector2D axes[4] = { axisA0, axisA1, axisB0, axisB1 };

    Vector2D centerA = tA.position;
    Vector2D centerB = tB.position;
    Vector2D distanceVector = centerB - centerA;

    RealNum minDepth = RealNum::Infinity();
    Vector2D bestAxis(0.0f, 0.0f);
    int bestAxisIndex = -1;

    // test all 4 potential separating axes using separating axis theroem
    for (int i = 0; i < 4; ++i) {
        Vector2D axis = axes[i];
        if (!axis.TryNormalize()) continue;

        Math::Interval intervalA = ProjectBox(centerA, axis, halfA, axisA0, axisA1);
        Math::Interval intervalB = ProjectBox(centerB, axis, halfB, axisB0, axisB1);
        auto overlapOpt = intervalA.GetOverlap(intervalB);

        if (!overlapOpt.has_value()) {
            return false; // found separating axis (no collision)
        }

        RealNum overlap = overlapOpt.value();
        if (overlap < minDepth) {
            minDepth = overlap;
            bestAxis = axis;
            bestAxisIndex = i;
        }
    }

    if (distanceVector.Dot(bestAxis) < 0.0f) {
        bestAxis = bestAxis * -1.0f;
    }

    // identify Reference and incident boxes based on best axis
    bool isBoxAReference = (bestAxisIndex < 2);

    Vector2D refCenter = isBoxAReference ? centerA : centerB;
    Vector2D incCenter = isBoxAReference ? centerB : centerA;
    Vector2D refHalf = isBoxAReference ? halfA : halfB;
    Vector2D incHalf = isBoxAReference ? halfB : halfA;
    Vector2D refAxis0 = isBoxAReference ? axisA0 : axisB0;
    Vector2D refAxis1 = isBoxAReference ? axisA1 : axisB1;
    Vector2D incAxis0 = isBoxAReference ? axisB0 : axisA0;
    Vector2D incAxis1 = isBoxAReference ? axisB1 : axisA1;

    Vector2D refFaceNormal = bestAxis;
    if (!isBoxAReference) refFaceNormal = refFaceNormal * -1.0f;

    // find incident face vertices
    Vector2D incidentFace[2];
    FindIncidentFace(incCenter, incHalf, incAxis0, incAxis1, refFaceNormal, incidentFace);

    // clip the incident face against the two side planes of the reference box
    Vector2D sidePlaneNormal = (bestAxisIndex == 0 || bestAxisIndex == 2) ? refAxis1 : refAxis0;

    float refEdgeCenter = refCenter.Dot(sidePlaneNormal);
    float refExtent = (bestAxisIndex == 0 || bestAxisIndex == 2) ? refHalf.y : refHalf.x;

    // clip against negative side plane
    Vector2D clipped1[2];
    int count = ClipSegmentToLine(incidentFace, clipped1, sidePlaneNormal * -1.0f, -(refEdgeCenter - refExtent));
    if (count < 1) return false;

    // clip against positive side plane
    Vector2D clipped2[2];
    count = ClipSegmentToLine(clipped1, clipped2, sidePlaneNormal, refEdgeCenter + refExtent);
    if (count < 1) return false;

    bool referenceIsX = (bestAxisIndex == 0 || bestAxisIndex == 2);

    RealNum refNormalExtent = referenceIsX
        ? refHalf.x
        : refHalf.y;

    float frontPlaneOffset =
        refCenter.Dot(refFaceNormal) + refNormalExtent;

    // keep only points that are behind the reference face (negative penetration depth)
    for (int i = 0; i < count; ++i) {
        float separation = clipped2[i].Dot(refFaceNormal) - frontPlaneOffset;
        if (separation <= 0.0f) {
            CollisionPoint coll;
            coll.position = clipped2[i];
            coll.normal = bestAxis;
            coll.depth = std::abs(separation);
            coll.normalImpulse = 0.0f;
            coll.tangentImpulse = 0.0f;
            outManifold.collisionPoints.push_back(coll);

            // limit to at most 2 contact points
            if (outManifold.collisionPoints.size() >= 2) {
                break;
            }
        }
    }
    outManifold.material = Material::FromCombined(bodyA.GetMaterial(), bodyB.GetMaterial());
    return !outManifold.collisionPoints.empty();
}
