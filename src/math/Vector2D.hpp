#ifndef MATH_VECTOR2D_HPP
#define MATH_VECTOR2D_HPP

#include "math/SafeReal.hpp"
#include "Vector2.hpp"

namespace Math {
    struct Vector2D {
        RealNum x;
        RealNum y;

        inline raylib::Vector2 ToRaylib() const { return raylib::Vector2(static_cast<float>(x), static_cast<float>(y)); }
        static Vector2D Zero() { return {0, 0}; }
    };
}

#endif
