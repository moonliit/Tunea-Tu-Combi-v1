#include "physics/engine/World.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/step-pipeline/01_BroadPhase.hpp"
#include "physics/engine/step-pipeline/02_NarrowPhase.hpp"
#include "physics/engine/step-pipeline/03_VelocitiesResolution.hpp"
#include "physics/engine/step-pipeline/04_PositionsCorrection.hpp"
using namespace std;
using namespace Math;
using namespace Physics;

void World::Step(float deltaTime) {
    float subDt = deltaTime / m_subStepCount;

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
    }

    // clear accumlated forces after the step
    for (auto& body: m_bodies) {
        body->GetForceMut() = Vector2D::Zero();
    }
}
