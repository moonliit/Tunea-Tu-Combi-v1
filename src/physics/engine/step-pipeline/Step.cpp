#include "physics/engine/World.hpp"
#include "physics/engine/common/BodyPairHash.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/step-pipeline/01_BroadPhase.hpp"
#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/step-pipeline/03_VelocitiesResolution.hpp"
#include "physics/engine/step-pipeline/04_PositionsCorrection.hpp"
#include "physics/engine/step-pipeline/05_JointBreakageCleanup.hpp"
#include "physics/engine/step-pipeline/0x_EmergencyFallbackCorrection.hpp"
using namespace std;
using namespace Math;
using namespace Physics;

void World::Step(float deltaTime) {
    float subDt = deltaTime / m_subStepCount;

    for (auto& joint : m_joints) {
        if (!joint) continue;
        joint->ResetStress();
    }
    m_structuralGraph.Rebuild(m_bodies, m_joints);

    for (int i = 0; i < m_subStepCount; ++i) {
        m_manifolds.clear();

        for (auto& body: m_bodies) {
            if (body->GetBodyType() == BodyType::Static) continue;

            Vector2D& vel = body->GetLinearVelocityMut();
            RealNum invMass = body->GetMassData().invMass;
            Vector2D force = body->GetForceMut();

            vel.x += body->GetForce().x * invMass * subDt;
            vel.y += body->GetForce().y * invMass * subDt;
            vel += m_gravity * subDt;

            // integration phase
            Transform& t = body->GetTransformMut();
            t.position += body->GetLinearVelocity() * subDt;
            t.rotation += body->GetAngularVelocityMut() * subDt;

            body->UpdateAABB();
        }

        std::vector<CollisionPair> candidatePairs =
            BroadPhase::FindPotentialCollisions(m_bodies);

        for (const auto& pair: candidatePairs) {
            Manifold manifold;

            auto cacheKey = MakeBodyPairKey(pair.first, pair.second);
            if (m_jointConnectedCache.find(cacheKey) != m_jointConnectedCache.end()) {
                // Skip collision detection for bodies connected by a joint
                continue;
            }

            if (NarrowPhase::DetectCollision(pair.first, pair.second, manifold)) {
                m_manifolds.push_back(manifold);
            }
        }

        for (int j = 0; j < m_velocityIterations; ++j) {
            ResolveJointVelocities(m_joints, subDt);
            ResolveManifoldVelocities(m_manifolds, subDt);
        }
        for (int j = 0; j < m_positionIterations; ++j) {
            CorrectJointPositions(m_joints);
            CorrectManifoldPositions(m_manifolds);
        }
        // --- EMERGENCY SAFETY & VELOCITY DAMPENING PASS ---
        // NOTE: currently disabled as physics seem stable for now
        // EmergencyFallbackCorrection(m_manifolds, subDt);
    }

    for (auto& joint : m_joints) {
        if (!joint) continue;
        auto bodyA = joint->GetBodyA();
        auto bodyB = joint->GetBodyB();
        if (!bodyA || !bodyB) continue;

        auto load = m_structuralGraph.Evaluate(*joint, *bodyA, *bodyB);
        joint->UpdateLocalStress(load);
    }

    // clear accumlated forces after the step
    for (auto& body: m_bodies) {
        body->GetForceMut() = Vector2D::Zero();
    }

    CleanupBrokenJoints(m_joints, m_jointConnectedCache);
}
