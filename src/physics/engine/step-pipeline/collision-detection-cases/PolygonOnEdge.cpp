#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/common/ContactCandidate.hpp"
#include "physics/engine/common/CollisionPolygon.hpp"
#include <algorithm>
#include <vector>
using namespace std;
using namespace Math;

bool Physics::NarrowPhase::DetectPolygonEdgeCollision(
    const PolygonRigidBody& bodyA,
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

    CollisionPolygon polyA = bodyA.GetCollisionPolygon();
    const auto& vertices = polyA.vertices;
    size_t count = vertices.size();
    if (count < 2) return false;

    vector<ContactCandidate> candidates;

    // evaluate all polygon vertices against the edge
    for (size_t i = 0; i < count; ++i) {
        EvaluatePoint(
            vertices[i],
            p1, p2, edgeNormal,
            edgeDir, edgeLength, candidates
        );
    }

    // evaluate segment intersections for broad/narrow proximity checking
    for (size_t i = 0; i < count; ++i) {
        Vector2D a = vertices[i];
        Vector2D b = vertices[(i + 1) % count];

        RealNum minX_poly = min(a.x, b.x);
        RealNum maxX_poly = max(a.x, b.x);
        RealNum minY_poly = min(a.y, b.y);
        RealNum maxY_poly = max(a.y, b.y);

        RealNum minX_terr = min(p1.x, p2.x);
        RealNum maxX_terr = max(p1.x, p2.x);
        RealNum minY_terr = min(p1.y, p2.y);
        RealNum maxY_terr = max(p1.y, p2.y);

        if (!(maxX_poly < minX_terr || minX_poly > maxX_terr || maxY_poly < minY_terr || minY_poly > maxY_terr)) {
            EvaluatePoint(a, p1, p2, edgeNormal, edgeDir, edgeLength, candidates);
            EvaluatePoint(b, p1, p2, edgeNormal, edgeDir, edgeLength, candidates);
        }
    }

    if (candidates.empty()) return false;

    // sort along edge tangent to select contact span extremes
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
