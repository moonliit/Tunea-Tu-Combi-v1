#include "physics/engine/Common.hpp"
#include "physics/engine/World.hpp"
#include "physics/engine/BroadPhase.hpp"
#include "physics/engine/narrow_phase/NarrowPhase.hpp"
using namespace std;
using namespace Math;
using namespace Physics;

World::World(Math::Vector2D gravity, int subStepCount):
    m_gravity(gravity), m_subStepCount(subStepCount) {}

void World::Step(float deltaTime) {
    m_manifolds.clear();

    for (auto& body: m_bodies) {
        if (body->GetBodyType() == BodyType::Static) continue;
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
}
