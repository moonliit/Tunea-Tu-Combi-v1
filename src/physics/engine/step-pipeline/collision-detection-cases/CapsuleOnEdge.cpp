#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/common/ContactCandidate.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/RigidBodies.hpp"
#include <algorithm>
#include <vector>
using namespace std;
using namespace Math;

void EvaluateCapsulePoint(
    const Vector2D& pt,
    RealNum radius,
    const Vector2D& p1,
    const Vector2D& p2,
    const Vector2D& edgeNormal,
    const Vector2D& edgeDir,
    RealNum edgeLength,
    vector<Physics::ContactCandidate>& candidates
) {
    constexpr RealNum contactTolerance = 2.5f;
    constexpr RealNum edgeDistThreshold = 0.1f;

    RealNum signedDist = (pt - p1).Dot(edgeNormal);
    RealNum depth = radius - signedDist;
    if (depth < -contactTolerance) return;

    // find closest point projected onto the edge segment
    RealNum tanPos = (pt - p1).Dot(edgeDir);
    RealNum clampedTanPos = clamp(tanPos, RealNum(0), edgeLength);
    if (tanPos < -edgeDistThreshold || tanPos > edgeLength + edgeDistThreshold) return;

    Vector2D closestPointOnEdge = p1 + edgeDir * clampedTanPos;

    bool duplicate = false;
    for (const auto& c: candidates) {
        if ((c.position - closestPointOnEdge).LengthSquared() < RealNum::epsilon) {
            duplicate = true;
            break;
        }
    }
    if (!duplicate) {
        candidates.push_back({
            closestPointOnEdge,
            max(RealNum(0), depth),
            clampedTanPos
        });
    }
}

bool Physics::NarrowPhase::DetectCapsuleEdgeCollision(
    const CapsuleRigidBody& bodyA,
    const EdgeRigidBody& bodyB,
    Manifold& outManifold
) {
    outManifold.collisionPoints.clear();

    Transform transformA = bodyA.GetTransform();
    RealNum totalHeight = bodyA.GetHeight();
    RealNum radius = bodyA.GetRadius();
    RealNum halfSegment = std::max(RealNum(0), (totalHeight * 0.5f) - radius);

    Vector2D localTop(0.0f, halfSegment);
    Vector2D localBottom(0.0f, -halfSegment);
    Vector2D capsuleBotWorld = transformA.ToWorldPoint(localBottom); // capsule bottom center
    Vector2D capsuleTopWorld = transformA.ToWorldPoint(localTop); // capsule top center

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

    vector<ContactCandidate> candidates;

    // evaluate bottom and top circle centers against the edge
    EvaluateCapsulePoint(capsuleBotWorld, radius, p1, p2, edgeNormal, edgeDir, edgeLength, candidates);
    EvaluateCapsulePoint(capsuleTopWorld, radius, p1, p2, edgeNormal, edgeDir, edgeLength, candidates);

    if (candidates.empty()) return false;

    // sort along edge tangent and select contact span extremes
    sort(
        candidates.begin(), candidates.end(),
        [](const ContactCandidate& a, const ContactCandidate& b) {
            return a.tangentPos < b.tangentPos;
        }
    );

    // select up to two contact points
    int contactCount = min(2, static_cast<int>(candidates.size()));
    for (int i = 0; i < contactCount; ++i) {
        const ContactCandidate& candidate = candidates[i];

        CollisionPoint coll = CollisionPoint {
            .position = candidate.position,
            .normal = edgeNormal,
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
    return true;
}
