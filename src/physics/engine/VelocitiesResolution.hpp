#ifndef PHYSICS_VELOCITIES_RESOLUTION_HPP
#define PHYSICS_VELOCITIES_RESOLUTION_HPP

#include "physics/engine/BaseRigidBody.hpp"
#include "physics/engine/World.hpp"
#include <algorithm>

namespace Physics {
    inline void ResolveVelocities(std::vector<Manifold>& manifolds, float dt) {
        for (auto& manifold : manifolds) {
            auto bodyA = manifold.bodyA.lock();
            auto bodyB = manifold.bodyB.lock();
            if (!bodyA || !bodyB) continue;

            bool isStaticA = (bodyA->GetBodyType() == BodyType::Static);
            bool isStaticB = (bodyB->GetBodyType() == BodyType::Static);
            if (isStaticA && isStaticB) continue;

            // restitution coefficient (e)
            float e = std::min(bodyA->GetMaterial().restitution, bodyB->GetMaterial().restitution);

            for (const auto& cp : manifold.collisionPoints) {
                // 1. Math formulation mapping
                const Math::Vector2D& n = cp.normal;
                const Math::Vector2D& contactPos = cp.position;

                Math::Vector2D rA = contactPos - bodyA->GetTransform().position;
                Math::Vector2D rB = contactPos - bodyB->GetTransform().position;

                // 2D cross product terms for rotational lever arms (r x n)
                Math::Vector2D& vA = bodyA->GetLinearVelocityMut();
                Math::Vector2D& vB = bodyB->GetLinearVelocityMut();
                Math::RealNum& wA = bodyA->GetAngularVelocityMut();
                Math::RealNum& wB = bodyB->GetAngularVelocityMut();

                Math::Vector2D rotVelA = {-wA * rA.y, wA * rA.x};
                Math::Vector2D rotVelB = {-wB * rB.y, wB * rB.x};
                Math::Vector2D vRel = (vB + rotVelB) - (vA + rotVelA);

                // Math::Vector2D vRel = (vB + rB * wB) - (vA + rA * wA);
                Math::RealNum vSep = vRel.Dot(n);

                // Skip if objects are already separating
                if (vSep >= 0.0f) continue;

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

                // 2. Compute impulse scalar j
                Math::RealNum j = (-(1.0f + e) * vSep) / denom;

                // 3. Apply linear and angular velocity impulses
                if (!isStaticA) {
                    vA -= n * (j * invMassA);
                    wA -= rA_x_n * j * invInertiaA;
                }
                if (!isStaticB) {
                    vB += n * (j * invMassB);
                    wB += rB_x_n * j * invInertiaB;
                }
            }
        }
    }
}

#endif
