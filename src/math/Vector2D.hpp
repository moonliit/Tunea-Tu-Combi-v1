#ifndef MATH_VECTOR2D_HPP
#define MATH_VECTOR2D_HPP

#include "math/SafeReal.hpp"
#include "json.hpp"
#include <raylib.h>

class EditorGrid;

namespace Math {

    struct Vector2D {
        RealNum x;
        RealNum y;

        Vector2 ToRayVec() const;
        nlohmann::json ToJson() const;

        static Vector2D FromAngle(RealNum angle);
        static Vector2D FromJson(const nlohmann::json& j);
        static Vector2D Zero();

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
