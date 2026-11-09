#ifndef PHYSICS_BROAD_PHASE_HPP
#define PHYSICS_BROAD_PHASE_HPP

#include <vector>
#include <utility>
#include <memory>
#include "physics/engine/BaseRigidBody.hpp"

namespace Physics {

    // pair of colliding candidate bodies for the narrow phase
    using CollisionPair = std::pair<std::shared_ptr<BaseRigidBody>, std::shared_ptr<BaseRigidBody>>;

    class BroadPhase {
    public:
        static std::vector<CollisionPair> FindPotentialCollisions(
            const std::vector<std::shared_ptr<BaseRigidBody>>& bodies
        ) {
            std::vector<CollisionPair> candidatePairs;
            size_t n = bodies.size();

            for (size_t i = 0; i < n; ++i) {
                bodies[i]->UpdateAABB();

                for (size_t j = i + 1; j < n; ++j) {
                    auto bodyA = bodies[i];
                    auto bodyB = bodies[j];

                    // skip if both bodies are immovable
                    if (
                        bodyA->GetBodyType() == BodyType::Static &&
                        bodyB->GetBodyType() == BodyType::Static
                    ) {
                        continue;
                    }

                    if (bodyA->GetAABB().Intersects(bodyB->GetAABB())) {
                        candidatePairs.emplace_back(bodyA, bodyB);
                    }
                }
            }

            return candidatePairs;
        }
    };

}

#endif
