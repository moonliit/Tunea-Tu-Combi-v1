#ifndef PHYSICS_JOINT_HPP
#define PHYSICS_JOINT_HPP

#include "physics/I_Physics.hpp"
#include "physics/engine/BaseRigidBody.hpp"
#include "physics/engine/common/Common.hpp"
#include <memory>

namespace Physics {

enum class JointType {
    Revolute,
    Weld
};

class BaseJoint : public I_Joint {
public:
    static constexpr Math::RealNum breakThreshold = 500.0f;
protected:
    std::weak_ptr<BaseRigidBody> m_bodyA;
    std::weak_ptr<BaseRigidBody> m_bodyB;
    Math::Vector2D m_localAnchorA;
    Math::Vector2D m_localAnchorB;
    Math::Vector2D m_accumulatedLinearImpulse;
    Math::RealNum m_smoothedStress = 0.0f;
    Math::RealNum m_baselineStress = 0.0f;
    bool m_isBroken = false;

public:
    BaseJoint() = default;
    virtual ~BaseJoint() = default;

    virtual JointType GetJointType() const = 0;

    std::shared_ptr<BaseRigidBody> GetBodyA() { return m_bodyA.lock(); }
    std::shared_ptr<BaseRigidBody> GetBodyB() { return m_bodyB.lock(); }
    const Math::Vector2D& GetLocalAnchorA() const { return m_localAnchorA; }
    const Math::Vector2D& GetLocalAnchorB() const { return m_localAnchorB; }
    const Math::Vector2D& GetAccumulatedLinearImpulse() const { return m_accumulatedLinearImpulse; }
    Math::Vector2D& GetAccumulatedLinearImpulseMut() { return m_accumulatedLinearImpulse; }

    const Math::RealNum& GetCurrentStress() const { return m_smoothedStress; }
    void SetCurrentStress(const Math::RealNum& stress) { m_smoothedStress = stress; }

    // void AddStress(const Math::RealNum& stress) {
    //     m_smoothedStress += stress;
    // }
    void AccumulateAndFilterStress(Math::RealNum instantStress) {
        // 1. Slowly learn the resting baseline (gravitational load / static weight)
        // 0.99f means it adapts very slowly to steady state
        m_baselineStress = (m_baselineStress * 0.99f) + (instantStress * 0.01f);

        // 2. Isolate dynamic stress (only spikes *above* the resting baseline count)
        Math::RealNum dynamicStress = std::max(Math::RealNum(0), instantStress - m_baselineStress);

        // 3. Low-pass filter (EMA) to kill solver jitter and high-frequency spikes
        // 0.85f inertia, 0.15f new input (adjust to taste)
        m_smoothedStress = (m_smoothedStress * 0.85f) + (dynamicStress * 0.15f);
        std::println("Joint stress: {}", float(m_smoothedStress));
    }
    void ResetStress() { m_smoothedStress = 0.0f; }

    bool IsBroken() const { return m_isBroken; }
    void SetIsBroken(bool isBroken) { m_isBroken = isBroken; }
    bool ShouldBreak() const { return m_smoothedStress > breakThreshold; }
};

class RevoluteJoint : public BaseJoint {
public:
    explicit RevoluteJoint(const JointDef& def) {
        m_bodyA = def.bodyA;
        m_bodyB = def.bodyB;
        m_localAnchorA = def.anchorA;
        m_localAnchorB = def.anchorB;
        m_accumulatedLinearImpulse = Math::Vector2D::Zero();
    }
    virtual ~RevoluteJoint() = default;
    JointType GetJointType() const override { return JointType::Revolute; }
};

class WeldJoint : public RevoluteJoint {
private:
    Math::RealNum m_relativeAngle;
    Math::RealNum m_accumulatedAngularImpulse;

public:
    explicit WeldJoint(const JointDef& def) : RevoluteJoint(def) {
        auto bodyA = m_bodyA.lock();
        auto bodyB = m_bodyB.lock();
        if (bodyA && bodyB) {
            m_relativeAngle = bodyB->GetTransform().rotation - bodyA->GetTransform().rotation;
        } else {
            m_relativeAngle = 0.0f;
        }
        m_accumulatedAngularImpulse = 0.0f;
    }
    virtual ~WeldJoint() = default;
    JointType GetJointType() const override { return JointType::Weld; }

    const Math::RealNum& GetRelativeAngle() const { return m_relativeAngle; }
    const Math::RealNum& GetAccumulatedAngularImpulse() const { return m_accumulatedAngularImpulse; }
    Math::RealNum& GetRelativeAngleMut() { return m_relativeAngle; }
    Math::RealNum& GetAccumulatedAngularImpulseMut() { return m_accumulatedAngularImpulse; }
};

}

#endif
