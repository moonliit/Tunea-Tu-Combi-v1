#ifndef PHYSICS_BASE_HPP
#define PHYSICS_BASE_HPP

#include "physics/I_Physics.hpp"
#include "physics/engine/common/CollisionPolygon.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/common/Transform.hpp"

#include "math/Vector2D.hpp"
#include "math/AABB.hpp"

namespace Physics {

    enum class RbShape: uint8_t {
        Edge = 0,
        TerrainMesh = 1,
        Box = 2,
        Circle = 3,
        Capsule = 4,
        Triangle = 5,
    };

    class BaseRigidBody : public I_RigidBody {
    protected:
        Transform m_transform;
        MassData m_massData;
        Material m_material;

        Math::Vector2D m_linearVelocity;
        Math::RealNum m_angularVelocity;

        Math::Vector2D m_force;

        BodyType m_bodyType;
        Math::AABB m_aabb;

    public:
        BaseRigidBody() = default;
        virtual ~BaseRigidBody() = default;

        Math::Vector2D GetPosition() const override { return m_transform.position; }
        Math::RealNum GetRotation() const override { return m_transform.rotation; }

        inline void ApplyForce(Math::Vector2D force) override {
            if (m_bodyType == BodyType::Static) return;
            m_force += force;
        }
        inline void ApplyLinearImpulse(Math::Vector2D impulse) override {
            if (m_bodyType == BodyType::Static) return;
            m_linearVelocity += impulse * m_massData.invMass;
        }

        virtual void UpdateAABB() = 0;
        virtual RbShape GetRbShape() const = 0;

        const Transform& GetTransform() const { return m_transform; }
        Transform& GetTransformMut() { return m_transform; }

        const Math::Vector2D& GetLinearVelocity() const { return m_linearVelocity; }
        const Math::RealNum& GetAngularVelocity() const { return m_angularVelocity; }
        const Math::Vector2D& GetForce() const { return m_force; }

        Math::Vector2D& GetLinearVelocityMut() { return m_linearVelocity; }
        Math::RealNum& GetAngularVelocityMut() { return m_angularVelocity; }
        Math::Vector2D& GetForceMut() { return m_force; }

        const MassData& GetMassData() const { return m_massData; }
        const Material& GetMaterial() const { return m_material; }

        const BodyType& GetBodyType() const { return m_bodyType; }
        const Math::AABB& GetAABB() const { return m_aabb; }
    };

    class PolygonRigidBody : public BaseRigidBody {
    public:
        virtual ~PolygonRigidBody() = default;
        virtual CollisionPolygon GetCollisionPolygon() const = 0;
    };
}

#endif
