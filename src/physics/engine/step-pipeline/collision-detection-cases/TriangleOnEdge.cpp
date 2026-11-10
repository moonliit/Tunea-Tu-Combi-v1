#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/common/Common.hpp"
#include <algorithm>
#include <vector>
using namespace std;
using namespace Math;

// helper struct to store candidate contact points
struct ContactCandidate {
    Vector2D position;
    RealNum depth;
    RealNum tangentPos;
};

// helper function to evaluate a point against terrain edge and collect valid candidates
void EvaluatePoint(
    const Vector2D& pt,
    const Vector2D& p1,
    const Vector2D& p2,
    const Vector2D& edgeNormal,
    const Vector2D& edgeDir,
    RealNum edgeLength,
    vector<ContactCandidate>& candidates
) {
    constexpr RealNum contactTolerance = 2.5f;
    constexpr RealNum edgeDistThreshold = 0.1f;

    RealNum signedDist = (pt - p1).Dot(edgeNormal);
    RealNum depth = -signedDist;
    if (depth < -contactTolerance) return;

    RealNum tanPos = (pt - p1).Dot(edgeDir);
    if (tanPos < -edgeDistThreshold || tanPos > edgeLength + edgeDistThreshold) return;

    bool duplicate = false;
    for (const auto& c: candidates) {
        if ((c.position - pt).LengthSquared() < RealNum::epsilon) {
            duplicate = true;
            break;
        }
    }
    if (!duplicate) {
        candidates.push_back({
            pt,
            max(RealNum(0.0f), depth),
            tanPos
        });
    }
}

bool Physics::NarrowPhase::DetectTriangleEdgeCollision(
    const TriangleRigidBody& bodyA,
    const EdgeRigidBody& bodyB,
    Manifold& outManifold
) {
    outManifold.collisionPoints.clear();

    Vector2D p1 = bodyB.GetP1();
    Vector2D p2 = bodyB.GetP2();

    Vector2D edgeDir = p2 - p1;
    RealNum edgeLength = edgeDir.Length();
    if (!edgeDir.TryNormalize()) return false;

    Vector2D edgeNormal = bodyB.GetNormal();
    if (edgeNormal.y > 0.0f) {
        edgeNormal = edgeNormal * -1.0f;
    }
    if (!edgeNormal.TryNormalize()) return false;

    const auto& worldVertices = bodyA.GetWorldVertices();
    vector<ContactCandidate> candidates;

    for (int i = 0; i < 3; ++i) {
        EvaluatePoint(
            worldVertices[i],
            p1, p2, edgeNormal,
            edgeDir, edgeLength, candidates
        );
    }

    for (int i = 0; i < 3; ++i) {
        Vector2D a = worldVertices[i];
        Vector2D b = worldVertices[(i + 1) % 3];

        RealNum minX_tri = min(a.x, b.x);
        RealNum maxX_tri = max(a.x, b.x);
        RealNum minY_tri = min(a.y, b.y);
        RealNum maxY_tri = max(a.y, b.y);

        RealNum minX_terr = min(p1.x, p2.x);
        RealNum maxX_terr = max(p1.x, p2.x);
        RealNum minY_terr = min(p1.y, p2.y);
        RealNum maxY_terr = max(p1.y, p2.y);

        if (!(maxX_tri < minX_terr || minX_tri > maxX_terr || maxY_tri < minY_terr || minY_tri > maxY_terr)) {
            EvaluatePoint(
                a,
                p1, p2, edgeNormal,
                edgeDir, edgeLength, candidates
            );
            EvaluatePoint(
                b,
                p1, p2, edgeNormal,
                edgeDir, edgeLength, candidates
            );
        }
    }

    if (candidates.empty()) return false;

    // sort along edge tangent and pick extremes to lock rotation
    sort(
        candidates.begin(), candidates.end(),
        [](const ContactCandidate& a, const ContactCandidate& b) {
            return a.tangentPos < b.tangentPos;
        }
    );

    int contactCount = min(2, static_cast<int>(candidates.size()));
    const ContactCandidate* selected[2];

    if (candidates.size() >= 2) {
        selected[0] = &candidates.front();
        selected[1] = &candidates.back();
        contactCount = 2;
    } else {
        selected[0] = &candidates[0];
        contactCount = 1;
    }

    for (int i = 0; i < contactCount; ++i) {
        CollisionPoint coll;
        coll.normal = edgeNormal;
        coll.depth = selected[i]->depth;
        coll.position = selected[i]->position;
        coll.normalImpulse = 0.0f;
        coll.tangentImpulse = 0.0f;

        outManifold.collisionPoints.push_back(coll);
    }

    outManifold.material = Material::FromCombined(
        bodyA.GetMaterial(),
        bodyB.GetMaterial()
    );
    return true;
}
