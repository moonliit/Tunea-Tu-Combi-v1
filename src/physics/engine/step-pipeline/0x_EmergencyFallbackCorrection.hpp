#ifndef PHYSICS_EMERGENCY_FALLBACK_CORRECTION_HPP
#define PHYSICS_EMERGENCY_FALLBACK_CORRECTION_HPP

#include "physics/I_Physics.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/BaseRigidBody.hpp"
#include "math/SafeReal.hpp"
#include "math/Vector2D.hpp"

namespace Physics {
    inline void EmergencyFallbackCorrection(
        std::vector<Manifold>& manifolds,
        float deltaTime
    ) {
        for (const auto& manifold: manifolds) {
            for (const auto& coll: manifold.collisionPoints) {
                if (coll.depth <= Math::RealNum::epsilon) continue;

                auto bodyA = manifold.bodyA.lock();
                auto bodyB = manifold.bodyB.lock();
                if (!bodyA || !bodyB) continue;
                if (
                    bodyA->GetBodyType() == BodyType::Static ||
                    bodyB->GetBodyType() == BodyType::Static
                ) {
                    continue;
                }

                // position correction (bruteforce geometric push out)
                Math::RealNum correction = coll.depth;
                bodyA->GetTransformMut().position -= coll.normal * (correction * 0.5f);
                bodyB->GetTransformMut().position += coll.normal * (correction * 0.5f);

                bodyA->UpdateAABB();
                bodyB->UpdateAABB();

                // velocity dampening to prevent energy explosions
                // if penetration is deep enough to trigger this fallback, kill the closing velocity
                // and apply heavy damping so they settle instead of bouncing violently
                if (coll.depth > 0.02f) { // threshold for "stubborn/deep" overlap
                    Math::RealNum decayRate = 15.0f;
                    Math::RealNum dampeningFactor = std::exp(-decayRate * deltaTime);

                    Math::Vector2D& velA = bodyA->GetLinearVelocityMut();

                    // kill velocity component moving into the collision normal
                    Math::RealNum normalVelA = velA.Dot(coll.normal);
                    if (normalVelA < 0.0f) {
                        velA -= coll.normal * normalVelA;
                    }
                    // general dampening to bleed off jitter energy
                    velA *= dampeningFactor;
                    bodyA->GetAngularVelocityMut() *= dampeningFactor;

                    Math::Vector2D& velB = bodyB->GetLinearVelocityMut();
                    Math::RealNum normalVelB = velB.Dot(coll.normal * -1);
                    if (normalVelB < 0.0f) {
                        velB -= (coll.normal * -1) * normalVelB;
                    }
                    velB *= dampeningFactor;
                    bodyB->GetAngularVelocityMut() *= dampeningFactor;
                }
            }
        }
    }
};

#endif
