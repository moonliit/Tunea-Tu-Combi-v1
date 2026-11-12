#ifndef PHYSICS_COMMON_HPP
#define PHYSICS_COMMON_HPP

#include "math/Vector2D.hpp"
#include <memory>
#include <vector>
#include <cmath>
#include <print>

namespace Physics {
    class BaseRigidBody;

    struct Material {
        Math::RealNum friction = 0.4f;
        Math::RealNum restitution = 0.2f;

        static inline Material FromCombined(const Material& matA, const Material& matB) {
            Material combined;
            combined.friction = std::sqrt(matA.friction * matB.friction);
            combined.restitution = std::max(matA.restitution, matB.restitution);
            return combined;
        }
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

        void Print() const {
            std::println("CollisionPoint: position = ({}, {}), normal = ({}, {}), depth = {}, normalImpulse = {}, tangentImpulse = {}",
                float(position.x), float(position.y),
                float(normal.x), float(normal.y),
                float(depth), float(normalImpulse), float(tangentImpulse));
        }
    };

    struct Manifold {
        std::weak_ptr<BaseRigidBody> bodyA;
        std::weak_ptr<BaseRigidBody> bodyB;
        std::vector<CollisionPoint> collisionPoints;
        Material material;
    };

    struct JointDef {
        std::weak_ptr<BaseRigidBody> bodyA;
        std::weak_ptr<BaseRigidBody> bodyB;
        Math::Vector2D anchorA;
        Math::Vector2D anchorB;
        Math::RealNum breakResistance = 1.0f;
        bool collideConnected = false;
    };
}

#endif
