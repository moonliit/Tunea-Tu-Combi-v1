#include "math/Vector2D.hpp"
#include <cmath>
using namespace std;

namespace Math {

Vector2 Vector2D::ToRayVec() const {
    return Vector2 {
        static_cast<float>(x),
        static_cast<float>(y)
    };
}

nlohmann::json Vector2D::ToJson() const {
    return nlohmann::json {
        {"x", static_cast<float>(x)},
        {"y", static_cast<float>(y)}
    };
}

Vector2D Vector2D::Zero() {
    return Vector2D {0.f, 0.f};
}

Vector2D Vector2D::FromAngle(const RealNum& angle) {
    return Vector2D {
        cos(angle),
        sin(angle)
    };
}

Vector2D Vector2D::FromJson(const nlohmann::json& j) {
    return Vector2D {
        j.at("x").get<float>(),
        j.at("y").get<float>()
    };
}

Vector2D Vector2D::operator+(const Vector2D& other) const {
    return Vector2D {
        x + other.x,
        y + other.y
    };
}

Vector2D Vector2D::operator-(const Vector2D& other) const {
    return Vector2D {
        x - other.x,
        y - other.y
    };
}

Vector2D Vector2D::operator*(const RealNum& value) const {
    return Vector2D {
        x * value,
        y * value
    };
}

Vector2D Vector2D::operator/(const RealNum& value) const {
    return Vector2D {
        x / value,
        y / value
    };
}

RealNum Vector2D::Dot(const Vector2D& other) const {
    return (
        this->x * other.x +
        this->y * other.y
    );
}

RealNum Vector2D::Cross(const Vector2D& other) const {
    return (
        this->x * other.y -
        this->y * other.x
    );
}

RealNum Vector2D::TripleCross(
    const Math::Vector2D& p1,
    const Math::Vector2D& p2,
    const Math::Vector2D& p3
) {
    Vector2D u = p2 - p1;
    Vector2D v = p3 - p1;
    return u.Cross(v);
}

Vector2D& Vector2D::operator=(const Vector2D& other) {
    if (this == &other) return *this;
    x = other.x;
    y = other.y;
    return *this;
}

Vector2D& Vector2D::operator+=(const Vector2D& other) {
    x += other.x;
    y += other.y;
    return *this;
}

Vector2D& Vector2D::operator-=(const Vector2D& other) {
    x -= other.x;
    y -= other.y;
    return *this;
}

Vector2D& Vector2D::operator*=(const RealNum& value) {
    x *= value;
    y *= value;
    return *this;
}

Vector2D& Vector2D::operator/=(const RealNum& value) {
    x /= value;
    y /= value;
    return *this;
}

bool Vector2D::operator==(const Vector2D& other) const {
    return (
        fabs(static_cast<float>(x - other.x)) <= RealNum::epsilon &&
        fabs(static_cast<float>(y - other.y)) <= RealNum::epsilon
    );
}

bool Vector2D::operator!=(const Vector2D& other) const {
    return !(*this == other);
}

}
