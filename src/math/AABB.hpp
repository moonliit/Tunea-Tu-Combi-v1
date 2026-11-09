#ifndef MATH_AABB_HPP
#define MATH_AABB_HPP

#include "math/Vector2D.hpp"

namespace Math {

    struct AABB {
        Vector2D min;
        Vector2D max;

        AABB(): min(Vector2D()), max(Vector2D()) {}
        AABB(const Vector2D& min, const Vector2D& max): min(min), max(max) {}

        constexpr inline bool Contains(const Vector2D& point) const {
            return (
                min.x <= point.x && point.x <= max.x &&
                min.y <= point.y && point.y <= max.y
            );
        }
        constexpr inline bool Overlaps(const AABB& other) const {
            return (
                min.x <= other.max.x && max.x >= other.min.x &&
                min.y <= other.max.y && max.y >= other.min.y
            );
        }
    };

}

#endif
