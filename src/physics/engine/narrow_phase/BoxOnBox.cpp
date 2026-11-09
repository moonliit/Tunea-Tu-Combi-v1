#include "physics/engine/narrow_phase/NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/Common.hpp"
#include <cmath>
#include <algorithm>
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectBoxBoxCollision(
    const BoxRigidBody& bodyA,
    const BoxRigidBody& bodyB,
    Manifold& outManifold
) {
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

    // helper lambda to project a box onto an axis and get min/max intervals
    auto projectBox = [](
        const Vector2D& center,
        const Vector2D& axis,
        const Vector2D& halfSize,
        const Vector2D& ax0,
        const Vector2D& ax1
    ) {
        RealNum p0 = center.Dot(axis);
        RealNum r = halfSize.x * abs(ax0.Dot(axis)) + halfSize.y * (ax1.Dot(axis));
        return make_pair(p0 - r, p0 + r);
    };

    // test all 4 potential separating axes using separating axis theorem
    for (int i = 0; i < 4; ++i) {
        Vector2D axis = axes[i];
        if (!axis.TryNormalize()) continue;

        auto [minA, maxA] = projectBox(centerA, axis, halfA, axisA0, axisA1);
        auto [minB, maxB] = projectBox(centerB, axis, halfB, axisB0, axisB1);

        // check for overlap on this axis
        float overlap = min(maxA, maxB) - max(minA, minB);
        if (overlap < 0.0f) {
            // found a separating axis (no collision)
            return false;
        }
        if (overlap < minDepth) {
            minDepth = overlap;
            bestAxis = axis;
        }
    }

    // ensure normal points from bodyA to bodyB
    if (distanceVector.Dot(bestAxis) < 0.0f) {
        bestAxis = bestAxis * -1.0f;
    }

    CollisionPoint coll;
    coll.normal = bestAxis;
    coll.depth = minDepth;
    coll.position = centerA + bestAxis * (halfA.Dot(bestAxis));

    outManifold.collisionPoints.push_back(coll);
    return true;
}
