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
protected:
    std::weak_ptr<BaseRigidBody> m_bodyA;
    std::weak_ptr<BaseRigidBody> m_bodyB;
    Math::Vector2D m_localAnchorA;
    Math::Vector2D m_localAnchorB;
    Math::Vector2D m_accumulatedLinearImpulse;

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
