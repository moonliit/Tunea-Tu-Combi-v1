#ifndef PHYSICS_COMMON_HPP
#define PHYSICS_COMMON_HPP

#include <memory>
#include <vector>
#include "math/Vector2D.hpp"

namespace Physics {
    class BaseRigidBody;

    struct Transform {
        Math::Vector2D position;
        Math::RealNum rotation;

        Math::Vector2D ToWorldPoint(const Math::Vector2D& localPoint) const;
        Math::Vector2D ToLocalPoint(const Math::Vector2D& worldPoint) const;
        Math::Vector2D ToWorldVector(const Math::Vector2D& localVector) const;
        Math::Vector2D ToLocalVector(const Math::Vector2D& worldVector) const;
    };

    struct Shape {
        std::vector<Math::Vector2D> vertices;
    };

    struct Material {
        Math::RealNum friction = 0.4f;
        Math::RealNum restitution = 0.2f;
    };

    struct MassData {
        Math::RealNum mass = 0.0f;
        Math::RealNum invMass = 0.0f; // 1 / mass
        Math::RealNum inertia = 0.0f;
        Math::RealNum invInertia = 0.0f; // 1 / inertia
    };

    struct CollisionPoint {
        Math::Vector2D position;
        Math::Vector2D normal;
        Math::RealNum depth;
        Math::RealNum normalImpulse = 0.0f;
        Math::RealNum tangentImpulse = 0.0f;
    };

    struct Manifold {
        std::weak_ptr<BaseRigidBody> bodyA;
        std::weak_ptr<BaseRigidBody> bodyB;
        std::vector<CollisionPoint> collisionPoints;
    };

    struct JointDef {
        std::weak_ptr<BaseRigidBody> bodyA;
        std::weak_ptr<BaseRigidBody> bodyB;
        Math::Vector2D anchorA;
        Math::Vector2D anchorB;
        bool collideConnected = false;
    };
}

#endif
