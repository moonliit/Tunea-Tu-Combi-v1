#include "physics/engine/Common.hpp"
#include "physics/engine/World.hpp"
#include "physics/engine/BroadPhase.hpp"
#include "physics/engine/narrow_phase/NarrowPhase.hpp"
#include "physics/engine/VelocitiesResolution.hpp"
#include "physics/engine/PositionsCorrection.hpp"
using namespace std;
using namespace Math;
using namespace Physics;

World::World(Math::Vector2D gravity, int subStepCount):
    m_gravity(gravity), m_subStepCount(subStepCount) {}

void World::Step(float deltaTime) {
    float subDt = deltaTime / m_subStepCount;

    for (int i = 0; i < m_subStepCount; ++i) {
        m_manifolds.clear();

        for (auto& body: m_bodies) {
            if (body->GetBodyType() == BodyType::Static) continue;

            Vector2D& vel = body->GetLinearVelocityMut();
            vel += m_gravity * subDt;

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

        ResolveVelocities(m_manifolds, subDt);
        CorrectPositions(m_manifolds);
    }
}
