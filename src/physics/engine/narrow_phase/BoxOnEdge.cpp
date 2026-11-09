#include "math/SafeReal.hpp"
#include "physics/engine/narrow_phase/NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/Common.hpp"
#include <algorithm>
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectBoxEdgeCollision(
    const BoxRigidBody& bodyA,
    const EdgeRigidBody& bodyB,
    Manifold& outManifold
) {
    Transform tA = bodyA.GetTransform();
    Vector2D sizeA = bodyA.GetSize();
    Vector2D halfA = sizeA * 0.5f;

    // transform local axes to world space
    Vector2D axisA0 = tA.ToWorldVector(Vector2D(1.0f, 0.0f));
    Vector2D axisA1 = tA.ToWorldVector(Vector2D(0.0f, 1.0f));

    Vector2D p1 = bodyB.GetP1();
    Vector2D p2 = bodyB.GetP2();
    Vector2D edgeNormal = bodyB.GetNormal();

    // generate box vertices in world space
    Vector2D localVertices[4] = {
        Vector2D(-halfA.x, -halfA.y),
        Vector2D( halfA.x, -halfA.y),
        Vector2D( halfA.x,  halfA.y),
        Vector2D(-halfA.x,  halfA.y)
    };

    Vector2D worldVertices[4];
    for (int i = 0; i < 4; ++i) {
        worldVertices[i] = tA.ToWorldPoint(localVertices[i]);
    }

    // axes to test via separating axes theorem (edge normal + box local axes)
    Vector2D axes[3] = { edgeNormal, axisA0, axisA1 };

    RealNum minDepth = RealNum::Infinity();
    Vector2D bestAxis(0.0f, 0.0f);

    for (int i = 0; i < 3; ++i) {
        Vector2D axis = axes[i];
        if (!axis.TryNormalize()) continue;

        // project box vertices onto axis
        RealNum minBox = RealNum::Infinity();
        RealNum maxBox = - RealNum::Infinity();
        for (int v = 0; v < 4; ++v) {
            RealNum proj = worldVertices[v].Dot(axis);
            minBox = std::min(minBox, proj);
            maxBox = std::max(maxBox, proj);
        }

        // project edge endpoints onto axis
        RealNum p1Proj = p1.Dot(axis);
        RealNum p2Proj = p2.Dot(axis);
        RealNum minEdge = min(p1Proj, p2Proj);
        RealNum maxEdge = max(p1Proj, p2Proj);

        // check interval overlap
        RealNum overlap = min(maxBox, maxEdge) - max(minBox, minEdge);
        if (overlap < 0.0f) {
            // separating axis found (no collision)
            return false;
        }

        if (overlap < minDepth) {
            minDepth = overlap;
            bestAxis = axis;
        }
    }

    // ensure normal points outward from the terrain edge toward the box
    Vector2D edgeCenter = (p1 + p2) * 0.5f;
    Vector2D boxCenter = tA.position;
    if ((boxCenter - edgeCenter).Dot(bestAxis) < 0.0f) {
        bestAxis = bestAxis * -1.0f;
    }

    CollisionPoint coll;
    coll.normal = bestAxis;
    coll.depth = minDepth;
    coll.position = boxCenter + bestAxis * (halfA.Dot(bestAxis));

    outManifold.collisionPoints.push_back(coll);
    return true;
}
