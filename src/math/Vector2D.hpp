#ifndef MATH_VECTOR2D_HPP
#define MATH_VECTOR2D_HPP

#include "math/SafeReal.hpp"
#include <raylib.h>

class EditorGrid;

namespace Math {

    struct Vector2D {
        RealNum x;
        RealNum y;

        Vector2 ToRayVec() const;
        static Vector2D Zero();
        static Vector2D FromAngle(RealNum angle);

        Vector2D operator+(const Vector2D& other) const;
        Vector2D operator-(const Vector2D& other) const;
        Vector2D operator*(const RealNum& value) const;
        Vector2D operator/(const RealNum& value) const;

        RealNum Cross(const Vector2D& other) const;
        static RealNum TripleCross(
            const Vector2D& a,
            const Vector2D& b,
            const Vector2D& c
        );

        bool operator==(const Vector2D& other) const;
        bool operator!=(const Vector2D& other) const;
    };

}

#endif
