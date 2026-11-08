#ifndef MATH_MATRIX_2X2_HPP
#define MATH_MATRIX_2X2_HPP

#include <raylib.h>
#include <optional>
#include "math/SafeReal.hpp"
#include "math/Vector2D.hpp"

class EditorGrid;

namespace Math {

    struct Matrix2x2 {
        RealNum a, b;
        RealNum c, d;

        static Matrix2x2 Identity();
        static Matrix2x2 Rotation(RealNum angle);

        Matrix2x2 operator+(const Matrix2x2& other) const;
        Matrix2x2 operator-(const Matrix2x2& other) const;
        Matrix2x2 operator*(const RealNum& value) const;
        Matrix2x2 operator/(const RealNum& value) const;

        RealNum Determinant() const;
        std::optional<Matrix2x2> Inverse() const;
        Matrix2x2 Transpose() const;

        Matrix2x2 operator*(const Matrix2x2& other) const;
        Vector2D operator*(const Vector2D& v) const;
        static Matrix2x2 MultiplyMatrixXMatrix(const Matrix2x2& m1, const Matrix2x2& m2);
        static Vector2D MultiplyMatrixXVector(const Matrix2x2& m, const Vector2D& v);

        Matrix2x2& operator=(const Matrix2x2& other);
        Matrix2x2& operator+=(const Matrix2x2& other);
        Matrix2x2& operator-=(const Matrix2x2& other);
        Matrix2x2& operator*=(const RealNum& value);
        Matrix2x2& operator/=(const RealNum& value);

        bool operator==(const Matrix2x2& other) const;
        bool operator!=(const Matrix2x2& other) const;
    };
}

#endif
