#include "physics/engine/step-pipeline/03_VelocitiesResolution.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/Joint.hpp"
#include <memory>
using namespace Math;
using namespace std;

namespace Physics {

void ResolveJointVelocities(vector<shared_ptr<BaseJoint>>& joints, float dt) {
    for (auto& joint : joints) {
        if (!joint) continue;

        switch (joint->GetJointType()) {
            case JointType::Revolute: {
                RevoluteJoint& revolute = *static_pointer_cast<RevoluteJoint>(joint);
                ResolveRevoluteJointVelocity(revolute, dt);
                break;
            }
            case JointType::Weld: {
                WeldJoint& weld = *static_pointer_cast<WeldJoint>(joint);
                ResolveWeldJointVelocity(weld, dt);
                break;
            }
            default:
                break;
        }
    }
}

void ResolveRevoluteJointVelocity(RevoluteJoint& joint, float dt) {
    auto bodyA = joint.GetBodyA();
    auto bodyB = joint.GetBodyB();
    if (!bodyA || !bodyB) return;

    bool isStaticA = (bodyA->GetBodyType() == BodyType::Static);
    bool isStaticB = (bodyB->GetBodyType() == BodyType::Static);
    if (isStaticA && isStaticB) return;

    // 1. Compute world anchor points from current transforms
    const Transform& tA = bodyA->GetTransform();
    const Transform& tB = bodyB->GetTransform();
    Vector2D rA = tA.ToWorldVector(joint.GetLocalAnchorA());
    Vector2D rB = tB.ToWorldVector(joint.GetLocalAnchorB());
    Vector2D pA = tA.position + rA;
    Vector2D pB = tB.position + rB;

    // 2. Relative velocity at anchor points
    const Vector2D& vA = bodyA->GetLinearVelocity();
    const Vector2D& vB = bodyB->GetLinearVelocity();
    const RealNum& wA = bodyA->GetAngularVelocity();
    const RealNum& wB = bodyB->GetAngularVelocity();

    Vector2D rotVelA = {-wA * rA.y, wA * rA.x};
    Vector2D rotVelB = {-wB * rB.y, wB * rB.x};
    Vector2D vRel = (vB + rotVelB) - (vA + rotVelA);

    // 3. Mass matrix terms (K) for 2D point-to-point constraint
    // Revolute joint restricts motion in both X and Y axes
    RealNum invMassA = isStaticA ? RealNum(0) : bodyA->GetMassData().invMass;
    RealNum invMassB = isStaticB ? RealNum(0) : bodyB->GetMassData().invMass;
    RealNum invInertiaA = isStaticA ? RealNum(0) : bodyA->GetMassData().invInertia;
    RealNum invInertiaB = isStaticB ? RealNum(0) : bodyB->GetMassData().invInertia;

    // 4. Build Effective Mass Matrix (K) for 2D Point-to-Point constraint
    // Skew-symmetric vectors for cross products: u = (-r.y, r.x)
    Vector2D uA = {-rA.y, rA.x};
    Vector2D uB = {-rB.y, rB.x};

    // K = J * M^(-1) * J^T
    // Built column by column or via outer products of the Jacobian rows
    RealNum k_00 = invMassA + invMassB + invInertiaA * (uA.x * uA.x) + invInertiaB * (uB.x * uB.x);
    RealNum k_01 = invInertiaA * (uA.x * uA.y) + invInertiaB * (uB.x * uB.y);
    RealNum k_10 = k_01;
    RealNum k_11 = invMassA + invMassB + invInertiaA * (uA.y * uA.y) + invInertiaB * (uB.y * uB.y);
    Matrix2x2 K = {
        k_00, k_01,
        k_10, k_11
    };
    auto KInverseOpt = K.Inverse();
    if (!KInverseOpt.has_value()) return; // Singular matrix, skip
    Matrix2x2 massMatrix = KInverseOpt.value();

    // 5. Compute bias for positional error correction (Baumgarte)
    Vector2D bias = (pB - pA) * (0.2f / dt);
    Vector2D lambdaNew = massMatrix * ((vRel + bias) * -1.0f);

    Vector2D oldImpulse = joint.GetAccumulatedLinearImpulse();
    Vector2D totalImpulse = oldImpulse + lambdaNew;
    joint.GetAccumulatedLinearImpulseMut() = totalImpulse;
    Vector2D appliedImpulse = lambdaNew;

    // 7. Apply linear and angular impulses to bodies
    if (!isStaticA) {
        Vector2D& vA = bodyA->GetLinearVelocityMut();
        RealNum& wA = bodyA->GetAngularVelocityMut();
        vA -= appliedImpulse * invMassA;
        wA -= rA.Cross(appliedImpulse) * invInertiaA;
    }
    if (!isStaticB) {
        Vector2D& vB = bodyB->GetLinearVelocityMut();
        RealNum& wB = bodyB->GetAngularVelocityMut();
        vB += appliedImpulse * invMassB;
        wB += rB.Cross(appliedImpulse) * invInertiaB;
    }
}

void ResolveWeldJointVelocity(WeldJoint& joint, float dt) {
    // Step A: Solve the 2D linear anchor lock (same as Revolute)
    ResolveRevoluteJointVelocity(joint, dt);

    // Step B: Solve the 1D Angular Constraint (lock relative rotation)
    auto bodyA = joint.GetBodyA();
    auto bodyB = joint.GetBodyB();
    if (!bodyA || !bodyB) return;

    const Transform& tA = bodyA->GetTransform();
    const Transform& tB = bodyB->GetTransform();

    bool isStaticA = (bodyA->GetBodyType() == BodyType::Static);
    bool isStaticB = (bodyB->GetBodyType() == BodyType::Static);
    if (isStaticA && isStaticB) return;

    RealNum invInertiaA = isStaticA ? RealNum(0) : bodyA->GetMassData().invInertia;
    RealNum invInertiaB = isStaticB ? RealNum(0) : bodyB->GetMassData().invInertia;
    RealNum angularDenom = invInertiaA + invInertiaB;
    if (angularDenom <= 0.0f) return;

    // Relative angular velocity error
    RealNum wA = bodyA->GetAngularVelocity();
    RealNum wB = bodyB->GetAngularVelocity();
    RealNum wRel = wB - wA;

    // Positional angular error (thetaB - thetaA - targetAngle)
    RealNum currentAngleDiff = tB.rotation - tA.rotation;
    RealNum angleError = RealNum::WrapAngle((tB.rotation - tA.rotation) - joint.GetRelativeAngle());
    RealNum angularBias = angleError * (0.2f / dt);

    // Scalar angular impulse lambda_angle
    RealNum lambdaNewAngle = -(wRel + angularBias) / angularDenom;

    // Accumulate angular impulse
    RealNum oldAngleImpulse = joint.GetAccumulatedAngularImpulse();
    RealNum totalAngleImpulse = oldAngleImpulse + lambdaNewAngle;
    joint.GetAccumulatedAngularImpulseMut() = totalAngleImpulse;

    RealNum appliedAngleImpulse = lambdaNewAngle;

    // Apply angular impulse
    if (!isStaticA) {
        RealNum& wA = bodyA->GetAngularVelocityMut();
        wA -= appliedAngleImpulse * invInertiaA;
    }
    if (!isStaticB) {
        RealNum& wB = bodyB->GetAngularVelocityMut();
        wB += appliedAngleImpulse * invInertiaB;
    }
}

void ResolveManifoldVelocities(vector<Manifold>& manifolds, float dt) {
    for (auto& manifold : manifolds) {
        auto bodyA = manifold.bodyA.lock();
        auto bodyB = manifold.bodyB.lock();
        if (!bodyA || !bodyB) continue;

        bool isStaticA = (bodyA->GetBodyType() == BodyType::Static);
        bool isStaticB = (bodyB->GetBodyType() == BodyType::Static);
        if (isStaticA && isStaticB) continue;

        RealNum e = manifold.material.restitution;
        RealNum friction = manifold.material.friction;

        for (CollisionPoint& coll : manifold.collisionPoints) {
            const Vector2D& n = coll.normal;
            const Vector2D& contactPos = coll.position;

            Vector2D rA = contactPos - bodyA->GetTransform().position;
            Vector2D rB = contactPos - bodyB->GetTransform().position;

            Vector2D& vA = bodyA->GetLinearVelocityMut();
            Vector2D& vB = bodyB->GetLinearVelocityMut();
            RealNum& wA = bodyA->GetAngularVelocityMut();
            RealNum& wB = bodyB->GetAngularVelocityMut();

            Vector2D rotVelA = {-wA * rA.y, wA * rA.x};
            Vector2D rotVelB = {-wB * rB.y, wB * rB.x};
            Vector2D vRel = (vB + rotVelB) - (vA + rotVelA);

            RealNum invMassA =    isStaticA ? RealNum(0) : bodyA->GetMassData().invMass;
            RealNum invMassB =    isStaticB ? RealNum(0) : bodyB->GetMassData().invMass;
            RealNum invInertiaA = isStaticA ? RealNum(0) : bodyA->GetMassData().invInertia;
            RealNum invInertiaB = isStaticB ? RealNum(0) : bodyB->GetMassData().invInertia;

            RealNum rA_x_n = rA.Cross(n);
            RealNum rB_x_n = rB.Cross(n);
            RealNum angularComponentA = invInertiaA * (rA_x_n * rA_x_n);
            RealNum angularComponentB = invInertiaB * (rB_x_n * rB_x_n);
            RealNum normalDenom = invMassA + invMassB + angularComponentA + angularComponentB;

            if (normalDenom <= 0.0f) continue;

            RealNum vSep = vRel.Dot(n);
            RealNum restitutionBias = 0.0f;
            if (vSep < -1.0f) {
                restitutionBias = -e * vSep;
            }

            // scalar impulse for normal
            RealNum lambdaNormal = (restitutionBias - vSep) / normalDenom;

            // accumulate normal impulse (clamp at 0 so bodies dont pull each other)
            RealNum oldNormalImpulse = coll.normalImpulse;
            coll.normalImpulse = max(oldNormalImpulse + lambdaNormal, RealNum(0));
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
            Vector2D t(-n.y, n.x);
            RealNum vTangent = vRel.Dot(t);

            RealNum rA_x_t = rA.Cross(t);
            RealNum rB_x_t = rB.Cross(t);
            RealNum tangentDenom = (
                invMassA + invMassB +
                (invInertiaA * (rA_x_t * rA_x_t)) +
                (invInertiaB * (rB_x_t * rB_x_t))
            );

            if (tangentDenom > 0.0f) {
                RealNum lambdaTangent = -vTangent / tangentDenom;

                // max friction force is proportional to normal impulse (coloumbs law)
                RealNum maxTangentImpulse = friction * coll.normalImpulse;

                RealNum oldTangentImpulse = coll.tangentImpulse;
                coll.tangentImpulse = clamp(
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
