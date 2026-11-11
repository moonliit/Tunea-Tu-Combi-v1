#include "physics/engine/step-pipeline/04_PositionsCorrection.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/Joint.hpp"
#include <algorithm>
using namespace Math;
using namespace std;

namespace Physics {

void CorrectJointPositions(list<shared_ptr<BaseJoint>>& joints) {
    for (auto& joint : joints) {
        if (!joint) continue;

        switch (joint->GetJointType()) {
            case JointType::Revolute: {
                RevoluteJoint& revolute = *static_pointer_cast<RevoluteJoint>(joint);
                CorrectRevoluteJointPosition(revolute);
                break;
            }
            case JointType::Weld: {
                WeldJoint& weld = *static_pointer_cast<WeldJoint>(joint);
                CorrectWeldJointPosition(weld);
                break;
            }
            default:
                break;
        }
    }
}

void CorrectRevoluteJointPosition(RevoluteJoint& joint) {
    const float k_beta = 0.2f; // Positional correction factor

    auto bodyA = joint.GetBodyA();
    auto bodyB = joint.GetBodyB();
    if (!bodyA || !bodyB) return;

    bool isStaticA = (bodyA->GetBodyType() == BodyType::Static);
    bool isStaticB = (bodyB->GetBodyType() == BodyType::Static);

    // If both bodies are static, skip position correction
    if (isStaticA && isStaticB) return;

    // 1. Compute world anchor points from current transforms
    const Transform& tA = bodyA->GetTransform();
    const Transform& tB = bodyB->GetTransform();

    const Vector2D& localAnchorA = joint.GetLocalAnchorA();
    const Vector2D& localAnchorB = joint.GetLocalAnchorB();

    Vector2D rA = tA.ToWorldVector(localAnchorA);
    Vector2D rB = tB.ToWorldVector(localAnchorB);

    Vector2D pA = tA.position + rA;
    Vector2D pB = tB.position + rB;

    // 2. Position error vector C (the distance between anchors that needs closing)
    // We want pA + deltaA == pB + deltaB, so error vector is (pB - pA)
    Vector2D error = pB - pA;

    // Optional slop threshold to prevent micro-jitter when error is minuscule
    if (error.LengthSquared() < 0.00001f) return;

    // 3. Mass matrix terms (same effective mass matrix K as the velocity solver)
    RealNum invMassA = isStaticA ? RealNum(0) : bodyA->GetMassData().invMass;
    RealNum invMassB = isStaticB ? RealNum(0) : bodyB->GetMassData().invMass;
    RealNum invInertiaA = isStaticA ? RealNum(0) : bodyA->GetMassData().invInertia;
    RealNum invInertiaB = isStaticB ? RealNum(0) : bodyB->GetMassData().invInertia;

    Vector2D uA = {-rA.y, rA.x};
    Vector2D uB = {-rB.y, rB.x};

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

    // 4. Compute positional impulse scalar lambda = -K^(-1) * error * beta
    Vector2D lambda = massMatrix * (error * (-k_beta));

    // 5. Apply direct position and rotation corrections
    if (!isStaticA) {
        Transform& tA = bodyA->GetTransformMut();
        tA.position -= lambda * invMassA;
        tA.rotation -= rA.Cross(lambda) * invInertiaA;
        bodyA->UpdateAABB();
    }
    if (!isStaticB) {
        Transform& tB = bodyB->GetTransformMut();
        tB.position += lambda * invMassB;
        tB.rotation += rB.Cross(lambda) * invInertiaB;
        bodyB->UpdateAABB();
    }
}

void CorrectWeldJointPosition(WeldJoint& joint) {
    // 1. Correct the linear anchor drift (same as revolute)
    CorrectRevoluteJointPosition(joint); // If CorrectRevoluteJointPosition takes BaseJoint& or RevoluteJoint&

    // 2. Correct the angular drift for the weld constraint
    auto bodyA = joint.GetBodyA();
    auto bodyB = joint.GetBodyB();
    if (!bodyA || !bodyB) return;

    bool isStaticA = (bodyA->GetBodyType() == BodyType::Static);
    bool isStaticB = (bodyB->GetBodyType() == BodyType::Static);
    if (isStaticA && isStaticB) return;

    RealNum invInertiaA = isStaticA ? RealNum(0) : bodyA->GetMassData().invInertia;
    RealNum invInertiaB = isStaticB ? RealNum(0) : bodyB->GetMassData().invInertia;
    RealNum angularDenom = invInertiaA + invInertiaB;
    if (angularDenom <= 0.0f) return;

    // Current angle error vs target relative angle
    RealNum currentAngleDiff = bodyB->GetTransform().rotation - bodyA->GetTransform().rotation;
    RealNum angleError = RealNum::WrapAngle(currentAngleDiff - joint.GetRelativeAngle());

    // Positional correction factor (beta)
    float k_beta = 0.2f;
    RealNum lambdaAngle = -(angleError * k_beta) / angularDenom;

    // Apply direct rotation shifts
    if (!isStaticA) {
        Transform& tA = bodyA->GetTransformMut();
        tA.rotation -= lambdaAngle * invInertiaA;
        bodyA->UpdateAABB();
    }
    if (!isStaticB) {
        Transform& tB = bodyB->GetTransformMut();
        tB.rotation += lambdaAngle * invInertiaB;
        bodyB->UpdateAABB();
    }
}

void CorrectManifoldPositions(vector<Manifold>& manifolds) {
    const RealNum k_slop = 0.01f; // allowed penetration threshold (slop)
    const RealNum k_beta = 0.4f;  // positional correction factor (beta)

    for (auto& manifold : manifolds) {
        auto bodyA = manifold.bodyA.lock();
        auto bodyB = manifold.bodyB.lock();
        if (!bodyA || !bodyB) continue;

        bool isStaticA = (bodyA->GetBodyType() == BodyType::Static);
        bool isStaticB = (bodyB->GetBodyType() == BodyType::Static);
        if (isStaticA && isStaticB) continue;

        for (const auto& coll : manifold.collisionPoints) {
            // penetration constant: C
            RealNum C = max(RealNum(0), coll.depth - k_slop);
            if (C <= 0.0f) continue;

            const Vector2D& n = coll.normal;
            const Vector2D& contactPos = coll.position;

            // denominator terms
            Vector2D rA = contactPos - bodyA->GetTransform().position;
            Vector2D rB = contactPos - bodyB->GetTransform().position;

            RealNum rA_x_n = rA.Cross(n);
            RealNum rB_x_n = rB.Cross(n);

            RealNum invMassA =    isStaticA ? RealNum(0) : bodyA->GetMassData().invMass;
            RealNum invMassB =    isStaticB ? RealNum(0) : bodyB->GetMassData().invMass;
            RealNum invInertiaA = isStaticA ? RealNum(0) : bodyA->GetMassData().invInertia;
            RealNum invInertiaB = isStaticB ? RealNum(0) : bodyB->GetMassData().invInertia;

            RealNum angularComponentA = invInertiaA * (rA_x_n * rA_x_n);
            RealNum angularComponentB = invInertiaB * (rB_x_n * rB_x_n);

            RealNum denom = invMassA + invMassB + angularComponentA + angularComponentB;
            if (denom <= 0.0f) continue;

            // correction scalar lambda
            RealNum lambda = (C / denom) * k_beta;

            // direct position updates
            if (!isStaticA) {
                Transform& tA = bodyA->GetTransformMut();
                tA.position = tA.position - n * (invMassA * lambda);
                tA.rotation -= invInertiaA * lambda * rA_x_n;
                bodyA->UpdateAABB();
            }
            if (!isStaticB) {
                Transform& tB = bodyB->GetTransformMut();
                tB.position = tB.position + n * (invMassB * lambda);
                tB.rotation += invInertiaB * lambda * rB_x_n;
                bodyB->UpdateAABB();
            }
        }
    }
}

}
