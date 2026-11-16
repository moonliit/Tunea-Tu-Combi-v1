#include "physics/engine/World.hpp"
#include "physics/engine/common/BodyPairHash.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/step-pipeline/01_BroadPhase.hpp"
#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/step-pipeline/03_VelocitiesResolution.hpp"
#include "physics/engine/step-pipeline/04_PositionsCorrection.hpp"
#include "physics/engine/step-pipeline/05_JointBreakageCleanup.hpp"
#include "app/game/Part.hpp"
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_set>
using namespace std;
using namespace Math;
using namespace Physics;

void World::ApplyExplosion(
    const Vector2D& center,
    float breakImpulseForce,
    float launchImpulseForce,
    float radius
) {
    m_pendingExplosions.push_back(PendingExplosion{
        .center = center,
        .breakImpulseForce = breakImpulseForce,
        .launchImpulseForce = launchImpulseForce,
        .radius = radius
    });
}

std::vector<World::PendingLaunchImpulse> World::ProcessPendingExplosions() {
    std::vector<PendingLaunchImpulse> pendingLaunches;
    if (m_pendingExplosions.empty()) return pendingLaunches;

    constexpr float BlastJointBreakImpulse = 80.0f;
    constexpr float ChainActivationImpulse = 80.0f;
    std::unordered_set<const I_RigidBody*> blastAffectedBodies;

    const std::vector<PendingExplosion> explosions = std::move(m_pendingExplosions);
    m_pendingExplosions.clear();

    for (const PendingExplosion& explosion : explosions) {
        for (auto& body: m_bodies) {
            if (!body || body->GetBodyType() == BodyType::Static) continue;

            Vector2D bodyPos = body->GetTransform().position;
            Vector2D toBody = bodyPos - explosion.center;

            RealNum distance = toBody.Length();
            if (distance >= explosion.radius || distance.IsZero()) continue;
            Vector2D direction = toBody.Normal().value();

            // distance attenuation with linear falloff
            // (1 at center, 0 at radius)
            RealNum attenuation = 1 - distance / explosion.radius;
            RealNum finalBreakImpulse = explosion.breakImpulseForce * attenuation;
            RealNum finalLaunchImpulse = explosion.launchImpulseForce * attenuation;

            Vector2D launchImpulse = direction * finalLaunchImpulse;
            pendingLaunches.push_back(PendingLaunchImpulse{
                .body = body,
                .impulse = launchImpulse
            });

            if (finalBreakImpulse >= BlastJointBreakImpulse) {
                blastAffectedBodies.insert(body.get());
            }
            Part* impactedPart = body->GetPart();
            if (impactedPart != nullptr) {
                if (finalBreakImpulse >= ChainActivationImpulse) {
                    if (impactedPart->GetType() == PartType::TNTBarrel) {
                        impactedPart->Activate();
                    } else if (impactedPart->HasContainedPart() &&
                               impactedPart->GetContainedPart()->GetType() ==
                                   PartType::TNTBarrel) {
                        impactedPart->GetContainedPart()->Activate();
                    }
                }
            }
        }
    }

    if (blastAffectedBodies.empty()) return pendingLaunches;

    std::queue<std::list<std::shared_ptr<BaseJoint>>::iterator> brokenJointIters;
    for (auto it = m_joints.begin(); it != m_joints.end(); ++it) {
        auto& joint = *it;
        if (!joint || joint->IsBroken()) continue;

        auto bodyA = joint->GetBodyA();
        auto bodyB = joint->GetBodyB();
        if (!bodyA || !bodyB) continue;

        const bool affected =
            blastAffectedBodies.find(bodyA.get()) != blastAffectedBodies.end() ||
            blastAffectedBodies.find(bodyB.get()) != blastAffectedBodies.end();
        if (!affected) continue;

        auto cacheKey = MakeBodyPairKey(bodyA, bodyB);
        auto cacheIt = m_jointConnectedCache.find(cacheKey);
        if (cacheIt != m_jointConnectedCache.end()) {
            m_jointConnectedCache.erase(cacheIt);
        }
        joint->SetIsBroken(true);
        Math::Vector2D midPos = (
            bodyA->GetTransform().position + bodyB->GetTransform().position
        ) / 2;
        m_breakagePositions.push_back(midPos);
        brokenJointIters.push(it);
    }

    while (!brokenJointIters.empty()) {
        auto popIter = brokenJointIters.front();
        brokenJointIters.pop();
        m_joints.erase(popIter);
    }

    return pendingLaunches;
}

void World::Step(float deltaTime) {
    constexpr float FirstLandingProtectionSeconds = 0.35f;
    m_impactEvents.clear();
    if (m_subStepCount <= 0 || deltaTime <= 0.0f) return;
    
    float subDt = deltaTime / m_subStepCount;
    bool protectFirstLanding = m_firstLandingProtectionRemaining > 0.0f;

    const float linearDamping = std::max(0.0f, m_linearDamping);
    const float angularDamping = std::max(0.0f, m_angularDamping);
    const float linearDampFactor = std::exp(-linearDamping * subDt);
    const float angularDampFactor = std::exp(-angularDamping * subDt);

    std::vector<PendingLaunchImpulse> pendingLaunches = ProcessPendingExplosions();

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
            const RealNum invMass = body->GetMassData().invMass;
            const Vector2D force = body->GetForce();

            vel += force * invMass * subDt;
            vel += m_gravity * subDt;

            // exponential damping (frame-rate/substep independent)
            vel *= linearDampFactor;
            body->GetAngularVelocityMut() *= angularDampFactor;

            // integration phase
            Transform& t = body->GetTransformMut();
            t.position += vel * subDt;
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

        for (const Manifold& manifold : m_manifolds) {
            const auto bodyA = manifold.bodyA.lock();
            const auto bodyB = manifold.bodyB.lock();
            if (!bodyA || !bodyB) continue;

            float impulse = 0.0f;
            for (const CollisionPoint& point : manifold.collisionPoints) {
                impulse += std::abs(static_cast<float>(point.normalImpulse));
            }
            if (impulse <= 0.0f) continue;
            const auto isTerrain = [](const BaseRigidBody& body) {
                return body.GetRbShape() == RbShape::Edge ||
                    body.GetRbShape() == RbShape::TerrainMesh;
            };
            if (!m_firstTerrainContactSeen &&
                (isTerrain(*bodyA) != isTerrain(*bodyB))) {
                m_firstTerrainContactSeen = true;
                m_firstLandingProtectionRemaining = FirstLandingProtectionSeconds;
                protectFirstLanding = true;
            }
            if (bodyA->GetPart() != nullptr) {
                m_impactEvents.push_back({bodyA->GetPart(), impulse});
            }
            if (bodyB->GetPart() != nullptr) {
                m_impactEvents.push_back({bodyB->GetPart(), impulse});
            }
        }
    }

    for (const PendingLaunchImpulse& launch : pendingLaunches) {
        auto body = launch.body.lock();
        if (!body || body->GetBodyType() == BodyType::Static) continue;
        body->ApplyLinearImpulse(launch.impulse);
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

    // Keep collision impulses and effects, but ignore joint stress from the
    // first touchdown and its immediate bounces.
    if (protectFirstLanding) {
        for (const auto& joint : m_joints) {
            if (joint) joint->ResetStress();
        }
    } else {
        CleanupBrokenJoints(m_joints, m_jointConnectedCache, m_breakagePositions);
    }
    m_firstLandingProtectionRemaining = std::max(
        0.0f, m_firstLandingProtectionRemaining - deltaTime
    );
}

std::vector<PhysicsImpact> World::ConsumeImpactEvents() {
    std::vector<PhysicsImpact> events = std::move(m_impactEvents);
    m_impactEvents.clear();
    return events;
}

std::vector<Math::Vector2D> World::ConsumeBreakagePositions() {
    std::vector<Math::Vector2D> positions = std::move(m_breakagePositions);
    m_breakagePositions.clear();
    return positions;
}
