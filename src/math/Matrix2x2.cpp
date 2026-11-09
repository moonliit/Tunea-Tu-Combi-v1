#include "math/Matrix2x2.hpp"
#include <cmath>
using namespace Math;
using namespace std;

Matrix2x2 Matrix2x2::Zero() {
    return Matrix2x2 {
        0.0f, 0.0f,
        0.0f, 0.0f
    };
}

Matrix2x2 Matrix2x2::Identity() {
    return Matrix2x2 {
        1.0f, 0.0f,
        0.0f, 1.0f
    };
}

Matrix2x2 Matrix2x2::Rotation(RealNum angle) {
    RealNum cosTheta = cos(angle);
    RealNum sinTheta = sin(angle);
    return Matrix2x2 {
        cosTheta, -sinTheta,
        sinTheta, cosTheta
    };
}

Matrix2x2 Matrix2x2::FromRows(const Vector2D& row1, const Vector2D& row2) {
    return Matrix2x2 {
        row1.x, row1.y,
        row2.x, row2.y
    };
}

Matrix2x2 Matrix2x2::FromCols(const Vector2D& col1, const Vector2D& col2) {
    return Matrix2x2 {
        col1.x, col2.x,
        col1.y, col2.y
    };
}

Matrix2x2 Matrix2x2::operator+(const Matrix2x2& other) const {
    return Matrix2x2 {
        (a + other.a), (b + other.b),
        (c + other.c), (d + other.d)
    };
}
Matrix2x2 Matrix2x2::operator-(const Matrix2x2& other) const {
    return Matrix2x2 {
        (a - other.a), (b - other.b),
        (c - other.c), (d - other.d)
    };
}

Matrix2x2 Matrix2x2::operator*(const RealNum& value) const {
    return Matrix2x2 {
        (a * value), (b * value),
        (c * value), (d * value)
    };
}

Matrix2x2 Matrix2x2::operator/(const RealNum& value) const {
    return Matrix2x2 {
        (a / value), (b / value),
        (c / value), (d / value)
    };
}

RealNum Matrix2x2::Determinant() const {
    return a * d - b * c;
}

optional<Matrix2x2> Matrix2x2::Inverse() const {
    RealNum det = Determinant();
    if (det == 0.0f) return nullopt;

    RealNum invDet = 1.0f / det;
    return Matrix2x2 {
        d , -b,
        -c,  a
    } * invDet;
}

Matrix2x2 Matrix2x2::Transpose() const {
    return Matrix2x2 {
        a, c,
        b, d
    };
}

Matrix2x2 Matrix2x2::MultiplyMatrixXMatrix(const Matrix2x2& m1, const Matrix2x2& m2) {
    return Matrix2x2 {
        (m1.a * m2.a + m1.b * m2.c), (m1.a * m2.b + m1.b * m2.d),
        (m1.c * m2.a + m1.d * m2.c), (m1.c * m2.b + m1.d * m2.d)
    };
}
Vector2D Matrix2x2::MultiplyMatrixXVector(const Matrix2x2& m, const Vector2D& v) {
    return Vector2D {
        m.a * v.x + m.b * v.y,
        m.c * v.x + m.d * v.y
    };
}

Matrix2x2 Matrix2x2::operator*(const Matrix2x2& other) const {
    return Matrix2x2::MultiplyMatrixXMatrix(*this, other);
}

Vector2D Matrix2x2::operator*(const Vector2D& v) const {
    return Matrix2x2::MultiplyMatrixXVector(*this, v);
}

Matrix2x2& Matrix2x2::operator=(const Matrix2x2& other) {
    if (this == &other) return *this;
    a = other.a;
    b = other.b;
    c = other.c;
    d = other.d;
    return *this;
}

Matrix2x2& Matrix2x2::operator+=(const Matrix2x2& other) {
    a += other.a;
    b += other.b;
    c += other.c;
    d += other.d;
    return *this;
}

Matrix2x2& Matrix2x2::operator-=(const Matrix2x2& other) {
    a -= other.a;
    b -= other.b;
    c -= other.c;
    d -= other.d;
    return *this;
}

Matrix2x2& Matrix2x2::operator*=(const RealNum& value) {
    a *= value;
    b *= value;
    c *= value;
    d *= value;
    return *this;
}

Matrix2x2& Matrix2x2::operator/=(const RealNum& value) {
    a /= value;
    b /= value;
    c /= value;
    d /= value;
    return *this;
}

bool Matrix2x2::operator==(const Matrix2x2& other) const {
    if (this == &other) return true;
    return (
        (a == other.a) && (b == other.b) &&
        (c == other.c) && (d == other.d)
    );
}

bool Matrix2x2::operator!=(const Matrix2x2& other) const {
    return !(*this == other);
}
