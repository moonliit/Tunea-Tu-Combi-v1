#include "physics/engine/narrow_phase/NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/Common.hpp"
#include <algorithm>
#include <cmath>
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectBoxCircleCollision(
    const BoxRigidBody& bodyA,
    const CircleRigidBody& bodyB,
    Manifold& outManifold
) {
    Transform tA = bodyA.GetTransform();
    Vector2D sizeA = bodyA.GetSize();
    Vector2D halfA = sizeA * 0.5f;

    Vector2D circleCenterWorld = bodyB.GetTransform().position;
    float radiusB = bodyB.GetRadius();

    // get circle center in box local space
    Vector2D localCirclePos = tA.ToLocalPoint(circleCenterWorld);

    // find the closest local point on the box to the circle center
    Vector2D localNearest;
    localNearest.x = clamp(localCirclePos.x, -halfA.x, halfA.x);
    localNearest.y = clamp(localCirclePos.y, -halfA.y, halfA.y);

    bool inside = false;
    if (localCirclePos.x == localNearest.x && localCirclePos.y == localNearest.y) {
        inside = true;
        float dx = halfA.x - abs(localCirclePos.x);
        float dy = halfA.y - abs(localCirclePos.y);
        if (dx < dy) {
            localNearest.x = (localCirclePos.x > 0.0f) ? halfA.x : -halfA.x;
        } else {
            localNearest.y = (localCirclePos.y > 0.0f) ? halfA.y : -halfA.y;
        }
    }

    Vector2D worldNearest = tA.ToWorldPoint(localNearest);
    Vector2D dir = circleCenterWorld - worldNearest;
    float distanceSq = dir.LengthSquared();

    if (!inside && distanceSq >= radiusB * radiusB) {
        return false;
    }

    float distance = std::sqrt(distanceSq);
    CollisionPoint coll;

    if (inside) {
        Vector2D normalLocal = (localCirclePos - localNearest);
        float len = normalLocal.Length();
        if (len > 0.0f) {
            coll.normal = tA.ToWorldVector(normalLocal).Normal().value_or(Vector2D::Right());
        } else {
            coll.normal = Vector2D::Right();
        }
        coll.depth = radiusB + len;
        coll.position = worldNearest;
    } else {
        if (distance == 0.0f) {
            coll.normal = Vector2D::Right();
            coll.depth = radiusB;
            coll.position = worldNearest;
        } else {
            coll.normal = dir * (1.0f / distance);
            coll.depth = radiusB - distance;
            coll.position = worldNearest;
        }
    }

    outManifold.collisionPoints.push_back(coll);
    return true;
}
