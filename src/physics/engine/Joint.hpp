#ifndef PHYSICS_JOINT_HPP
#define PHYSICS_JOINT_HPP

#include "physics/engine/StructuralAnalysis.hpp"
#include "physics/engine/BaseRigidBody.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/I_Physics.hpp"
#include <memory>
#include <algorithm>

namespace Physics {

enum class JointType {
    Revolute,
    Weld
};

class BaseJoint : public I_Joint {
public:
    static constexpr Math::RealNum breakThreshold = 30.0f;
    static constexpr Math::RealNum revoluteExtraDamping = 2.0f;
protected:
    std::weak_ptr<BaseRigidBody> m_bodyA;
    std::weak_ptr<BaseRigidBody> m_bodyB;
    Math::Vector2D m_localAnchorA;
    Math::Vector2D m_localAnchorB;
    Math::Vector2D m_accumulatedLinearImpulse;
    Math::RealNum m_peakSolverStress = 0.0f;
    Math::RealNum m_localStress = 0.0f;
    Math::RealNum m_breakResistance = 1.0f;
    bool m_isBroken = false;

public:
    BaseJoint() = default;
    virtual ~BaseJoint() = default;

    virtual JointType GetJointType() const = 0;

    std::shared_ptr<BaseRigidBody> GetBodyA() { return m_bodyA.lock(); }
    std::shared_ptr<BaseRigidBody> GetBodyB() { return m_bodyB.lock(); }
    std::shared_ptr<BaseRigidBody> GetBodyA() const { return m_bodyA.lock(); }
    std::shared_ptr<BaseRigidBody> GetBodyB() const { return m_bodyB.lock(); }
    const Math::Vector2D& GetLocalAnchorA() const { return m_localAnchorA; }
    const Math::Vector2D& GetLocalAnchorB() const { return m_localAnchorB; }
    const Math::Vector2D& GetAccumulatedLinearImpulse() const { return m_accumulatedLinearImpulse; }
    Math::Vector2D& GetAccumulatedLinearImpulseMut() { return m_accumulatedLinearImpulse; }

    Math::RealNum GetPeakSolverStress() const { return m_peakSolverStress; }
    Math::RealNum GetLocalStress() const { return m_localStress; }
    Math::RealNum GetBreakResistance() const { return m_breakResistance; }
    void SetBreakResistance(Math::RealNum resistance) {
        m_breakResistance = std::max(Math::RealNum(0.01f), resistance);
    }

    void RecordSolverStress(Math::RealNum instantStress) {
        m_peakSolverStress = std::max(m_peakSolverStress, instantStress);
    }
    void ResetStress() { m_peakSolverStress = 0.0f; m_localStress = 0.0f; }
    void UpdateLocalStress(const Structural::LoadData& load) {
        Math::RealNum damping = load.massDamping;
        damping *= load.leverArmDamping;
        if (GetJointType() == JointType::Revolute) {
            damping *= revoluteExtraDamping;
        }
        damping *= 1 + 0.25f * load.redundancy;
        damping = std::clamp(damping, Math::RealNum(1), Math::RealNum(8));

        m_localStress = m_peakSolverStress / damping;
    }

    bool IsBroken() const { return m_isBroken; }
    void SetIsBroken(bool isBroken) { m_isBroken = isBroken; }
    bool ShouldBreak() const {
        return m_localStress >= breakThreshold * m_breakResistance;
    }
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
        m_breakResistance = def.breakResistance;
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
