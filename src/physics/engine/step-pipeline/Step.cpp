#include "physics/engine/World.hpp"
#include "physics/engine/common/BodyPairHash.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/step-pipeline/01_BroadPhase.hpp"
#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/step-pipeline/03_VelocitiesResolution.hpp"
#include "physics/engine/step-pipeline/04_PositionsCorrection.hpp"
#include "physics/engine/step-pipeline/05_JointBreakageCleanup.hpp"
#include "app/game/Part.hpp"
using namespace std;
using namespace Math;
using namespace Physics;

void World::ApplyExplosion(
    const Vector2D& center,
    float impulseForce,
    float radius
) {
    for (auto& body: m_bodies) {
        if (!body || body->GetBodyType() == BodyType::Static) continue;

        Vector2D bodyPos = body->GetTransform().position;
        Vector2D toBody = bodyPos - center;

        RealNum distance = toBody.Length();
        if (distance >= radius || distance.IsZero()) continue;
        Vector2D direction = toBody.Normal().value();

        // distance attenuation with linear falloff
        // (1 at center, 0 at radius)
        RealNum attenuation = 1 - distance / radius;
        RealNum finalImpulseMagnitude = impulseForce * attenuation;

        Vector2D impulse = direction * finalImpulseMagnitude;
        body->ApplyLinearImpulse(impulse);
        Part* impactedPart = body->GetPart();
        if (impactedPart != nullptr &&
            finalImpulseMagnitude >= 80.0f) {
            if (impactedPart->GetType() == PartType::TNTBarrel) {
                impactedPart->Activate();
            } else if (impactedPart->HasContainedPart() &&
                       impactedPart->GetContainedPart()->GetType() ==
                           PartType::TNTBarrel) {
                impactedPart->GetContainedPart()->Activate();
            }
        }

        // explosion spins objects based on where they get hit
        Vector2D r = bodyPos - body->GetTransform().position;
        RealNum invInertia = body->GetMassData().invInertia;
        body->GetAngularVelocityMut() += r.Cross(impulse) * invInertia;
    }
}

void World::Step(float deltaTime) {
    m_impactEvents.clear();
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

        for (const Manifold& manifold : m_manifolds) {
            const auto bodyA = manifold.bodyA.lock();
            const auto bodyB = manifold.bodyB.lock();
            if (!bodyA || !bodyB) continue;

            float impulse = 0.0f;
            for (const CollisionPoint& point : manifold.collisionPoints) {
                impulse += std::abs(static_cast<float>(point.normalImpulse));
            }
            if (impulse <= 0.0f) continue;
            if (bodyA->GetPart() != nullptr) {
                m_impactEvents.push_back({bodyA->GetPart(), impulse});
            }
            if (bodyB->GetPart() != nullptr) {
                m_impactEvents.push_back({bodyB->GetPart(), impulse});
            }
        }
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

    CleanupBrokenJoints(m_joints, m_jointConnectedCache, m_breakagePositions);
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
