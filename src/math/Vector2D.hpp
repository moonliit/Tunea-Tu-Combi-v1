#ifndef MATH_VECTOR2D_HPP
#define MATH_VECTOR2D_HPP

#include "math/SafeReal.hpp"
#include <raylib.h>

class EditorGrid;

namespace Math {

    struct Vector2D {
        RealNum x;
        RealNum y;

        inline Vector2 ToRayVec() const { return Vector2(static_cast<float>(x), static_cast<float>(y)); }
        static Vector2D Zero() { return {0, 0}; }
    };
}

#endif
