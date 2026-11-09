#ifndef PHYSICS_POSITIONS_CORRECTION_HPP
#define PHYSICS_POSITIONS_CORRECTION_HPP

#include "physics/engine/BaseRigidBody.hpp"
#include "physics/engine/RigidBodies.hpp"
#include <algorithm>

namespace Physics {
    inline void CorrectPositions(std::vector<Manifold>& manifolds) {
        const Math::RealNum k_slop = 0.000001f; // allowed penetration threshold (slop)
        const Math::RealNum k_beta = 0.4f;  // baumgarte stabilization factor (beta)

        for (auto& manifold : manifolds) {
            auto bodyA = manifold.bodyA.lock();
            auto bodyB = manifold.bodyB.lock();
            if (!bodyA || !bodyB) continue;

            bool isStaticA = (bodyA->GetBodyType() == BodyType::Static);
            bool isStaticB = (bodyB->GetBodyType() == BodyType::Static);
            if (isStaticA && isStaticB) continue;

            for (const auto& coll : manifold.collisionPoints) {
                // Penetration constant: C
                Math::RealNum C = std::max(Math::RealNum(0), coll.depth - k_slop);
                if (C <= 0.0f) continue;

                const Math::Vector2D& n = coll.normal;
                const Math::Vector2D& contactPos = coll.position;

                // denominator terms
                Math::Vector2D rA = contactPos - bodyA->GetTransform().position;
                Math::Vector2D rB = contactPos - bodyB->GetTransform().position;

                Math::RealNum rA_x_n = rA.Cross(n);
                Math::RealNum rB_x_n = rB.Cross(n);

                Math::RealNum invMassA =    isStaticA ? Math::RealNum(0) : bodyA->GetMassData().invMass;
                Math::RealNum invMassB =    isStaticB ? Math::RealNum(0) : bodyB->GetMassData().invMass;
                Math::RealNum invInertiaA = isStaticA ? Math::RealNum(0) : bodyA->GetMassData().invInertia;
                Math::RealNum invInertiaB = isStaticB ? Math::RealNum(0) : bodyB->GetMassData().invInertia;

                Math::RealNum angularComponentA = invInertiaA * (rA_x_n * rA_x_n);
                Math::RealNum angularComponentB = invInertiaB * (rB_x_n * rB_x_n);

                Math::RealNum denom = invMassA + invMassB + angularComponentA + angularComponentB;
                if (denom <= 0.0f) continue;

                // correction scalar lambda
                Math::RealNum lambda = (C / denom) * k_beta;

                // direct position updates
                if (!isStaticA) {
                    Transform& tA = bodyA->GetTransformMut();
                    tA.position = tA.position - n * (invMassA * lambda);
                    bodyA->UpdateAABB();
                }

                if (!isStaticB) {
                    Transform& tB = bodyB->GetTransformMut();
                    tB.position = tB.position + n * (invMassB * lambda);
                    bodyB->UpdateAABB();
                }
            }
        }
    }
}

#endif
