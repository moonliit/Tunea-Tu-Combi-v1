#ifndef PHYSICS_TRANSFORM_HPP
#define PHYSICS_TRANSFORM_HPP

#include "math/SafeReal.hpp"
#include "math/Vector2D.hpp"
#include "math/Matrix2x2.hpp"

namespace Physics {
    struct Transform {
        Math::Vector2D position;
        Math::RealNum rotation;

        constexpr inline Math::Vector2D ToWorldPoint(const Math::Vector2D& localPoint) const {
            Math::Matrix2x2 rot = Math::Matrix2x2::Rotation(rotation);
            return (rot * localPoint) + position;
        }
        constexpr inline Math::Vector2D ToLocalPoint(const Math::Vector2D& worldPoint) const {
            Math::Matrix2x2 invRot = Math::Matrix2x2::Rotation(-rotation);
            return invRot * (worldPoint - position);
        }
        constexpr inline Math::Vector2D ToWorldVector(const Math::Vector2D& localVector) const {
            Math::Matrix2x2 rot = Math::Matrix2x2::Rotation(rotation);
            return rot * localVector;
        }
        constexpr inline Math::Vector2D ToLocalVector(const Math::Vector2D& worldVector) const {
            Math::Matrix2x2 invRot = Math::Matrix2x2::Rotation(-rotation);
            return invRot * worldVector;
        }
    };
}

#endif
