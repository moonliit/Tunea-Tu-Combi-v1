#include "math/SafeReal.hpp"
#include "physics/engine/narrow_phase/NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/Common.hpp"
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

    // ensure edge normal points upward (-Y in Raylib)
    if (edgeNormal.y > 0.0f) {
        edgeNormal = edgeNormal * -1.0f;
    }

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

    // test box vertices against edge normal for penetration depth
    RealNum maxPenetration = -RealNum::Infinity();
    Vector2D deepestVertex(0.0f, 0.0f);
    bool colliding = false;

    Vector2D edgeDir = p2 - p1;
    RealNum edgeLenSq = edgeDir.LengthSquared();

    for (int v = 0; v < 4; ++v) {
        Vector2D vertex = worldVertices[v];

        // signed distance from vertex to edge line along normal
        RealNum dist = (vertex - p1).Dot(edgeNormal);

        // check if vertex has penetrated behind the edge line
        if (dist < 0.0f) {
            RealNum depth = -dist;

            // check interval overlap along edge segment length
            if (edgeLenSq > 1e-6f) {
                RealNum t = (vertex - p1).Dot(edgeDir) / edgeLenSq;
                if (t >= -0.1f && t <= 1.1f) {
                    if (depth > maxPenetration) {
                        maxPenetration = depth;
                        deepestVertex = vertex;
                        colliding = true;
                    }
                }
            }
        }
    }

    if (!colliding || maxPenetration <= 0.0f) {
        // separating axis found (no collision)
        return false;
    }

    // ensure normal points outward from the terrain edge toward the box
    Vector2D edgeCenter = (p1 + p2) * 0.5f;
    Vector2D boxCenter = tA.position;
    if ((boxCenter - edgeCenter).Dot(edgeNormal) < 0.0f) {
        edgeNormal = edgeNormal * -1.0f;
    }

    CollisionPoint coll;
    coll.normal = edgeNormal;
    if (coll.normal.y > 0.0f) {
        coll.normal *= -1.0f;
    }
    coll.depth = maxPenetration;
    coll.position = deepestVertex;

    // assign body references to manifold
    outManifold.collisionPoints.push_back(coll);

    return true;
}
