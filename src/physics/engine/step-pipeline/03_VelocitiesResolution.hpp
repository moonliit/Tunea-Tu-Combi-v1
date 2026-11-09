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

            Math::RealNum e = manifold.material.restitution;
            Math::RealNum friction = manifold.material.friction;

            for (CollisionPoint& coll : manifold.collisionPoints) {
                const Math::Vector2D& n = coll.normal;
                const Math::Vector2D& contactPos = coll.position;

                Math::Vector2D rA = contactPos - bodyA->GetTransform().position;
                Math::Vector2D rB = contactPos - bodyB->GetTransform().position;

                Math::Vector2D& vA = bodyA->GetLinearVelocityMut();
                Math::Vector2D& vB = bodyB->GetLinearVelocityMut();
                Math::RealNum& wA = bodyA->GetAngularVelocityMut();
                Math::RealNum& wB = bodyB->GetAngularVelocityMut();

                Math::Vector2D rotVelA = {-wA * rA.y, wA * rA.x};
                Math::Vector2D rotVelB = {-wB * rB.y, wB * rB.x};
                Math::Vector2D vRel = (vB + rotVelB) - (vA + rotVelA);

                Math::RealNum invMassA =    isStaticA ? Math::RealNum(0) : bodyA->GetMassData().invMass;
                Math::RealNum invMassB =    isStaticB ? Math::RealNum(0) : bodyB->GetMassData().invMass;
                Math::RealNum invInertiaA = isStaticA ? Math::RealNum(0) : bodyA->GetMassData().invInertia;
                Math::RealNum invInertiaB = isStaticB ? Math::RealNum(0) : bodyB->GetMassData().invInertia;

                Math::RealNum rA_x_n = rA.Cross(n);
                Math::RealNum rB_x_n = rB.Cross(n);
                Math::RealNum angularComponentA = invInertiaA * (rA_x_n * rA_x_n);
                Math::RealNum angularComponentB = invInertiaB * (rB_x_n * rB_x_n);
                Math::RealNum normalDenom = invMassA + invMassB + angularComponentA + angularComponentB;

                if (normalDenom <= 0.0f) continue;

                Math::RealNum vSep = vRel.Dot(n);
                Math::RealNum restitutionBias = 0.0f;
                if (vSep < -1.0f) {
                    restitutionBias = -e * vSep;
                }

                // scalar impulse for normal
                Math::RealNum lambdaNormal = (restitutionBias - vSep) / normalDenom;

                // accumulate normal impulse (clamp at 0 so bodies dont pull each other)
                Math::RealNum oldNormalImpulse = coll.normalImpulse;
                coll.normalImpulse = std::max(oldNormalImpulse + lambdaNormal, Math::RealNum(0));
                lambdaNormal = coll.normalImpulse - oldNormalImpulse;

                // apply normal impulses to velocities
                if (!isStaticA) {
                    vA -= n * (lambdaNormal * invMassA);
                    wA -= rA_x_n * lambdaNormal * invInertiaA;
                }
                if (!isStaticB) {
                    vB += n * (lambdaNormal * invMassB);
                    wB += rB_x_n * lambdaNormal * invInertiaB;
                }

                rotVelA = {-wA * rA.y, wA * rA.x};
                rotVelB = {-wB * rB.y, wB * rB.x};
                vRel = (vB + rotVelB) - (vA + rotVelA);

                // define tangent vector perpendicular to normal
                Math::Vector2D t(-n.y, n.x);
                Math::RealNum vTangent = vRel.Dot(t);

                Math::RealNum rA_x_t = rA.Cross(t);
                Math::RealNum rB_x_t = rB.Cross(t);
                Math::RealNum tangentDenom = (
                    invMassA + invMassB +
                    (invInertiaA * (rA_x_t * rA_x_t)) +
                    (invInertiaB * (rB_x_t * rB_x_t))
                );

                if (tangentDenom > 0.0f) {
                    Math::RealNum lambdaTangent = -vTangent / tangentDenom;

                    // max friction force is proportional to normal impulse (coloumbs law)
                    Math::RealNum maxTangentImpulse = friction * coll.normalImpulse;

                    Math::RealNum oldTangentImpulse = coll.tangentImpulse;
                    coll.tangentImpulse = std::clamp(
                        oldTangentImpulse + lambdaTangent,
                        -maxTangentImpulse,
                        maxTangentImpulse
                    );
                    lambdaTangent = coll.tangentImpulse - oldTangentImpulse;

                    // apply friction impulse to velocities
                    if (!isStaticA) {
                        vA -= t * (lambdaTangent * invMassA);
                        wA -= rA_x_t * lambdaTangent * invInertiaA;
                    }
                    if (!isStaticB) {
                        vB += t * (lambdaTangent * invMassB);
                        wB += rB_x_t * lambdaTangent * invInertiaB;
                    }
                }
            }
        }
    }
}

#endif
