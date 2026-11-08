#ifndef PHYSICS_COMMON_HPP
#define PHYSICS_COMMON_HPP

#include <memory>
#include <vector>
#include "physics/I_Physics.hpp"
#include "math/Vector2D.hpp"

namespace Physics {
    struct Transform {
        Math::Vector2D position;
        Math::RealNum rotation;
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
        Math::RealNum penetrationDepth;
        Math::RealNum normalImpulse = 0.0f;
        Math::RealNum tangentImpulse = 0.0f;
    };

    struct Manifold {
        std::shared_ptr<I_RigidBody> bodyA;
        std::shared_ptr<I_RigidBody> bodyB;
        std::vector<CollisionPoint> collisionPoints;
    };

    struct JointDef {
        std::shared_ptr<I_RigidBody> bodyA;
        std::shared_ptr<I_RigidBody> bodyB;
        Math::Vector2D anchorA;
        Math::Vector2D anchorB;
        bool collideConnected = false;
    };
}

#endif
