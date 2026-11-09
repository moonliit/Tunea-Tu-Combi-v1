#include "physics/engine/Common.hpp"
#include "math/Matrix2x2.hpp"
using namespace Math;

namespace Physics {

Vector2D Transform::ToWorldPoint(const Vector2D& localPoint) const {
    Matrix2x2 rot = Matrix2x2::Rotation(rotation);
    return (rot * localPoint) + position;
}

Vector2D Transform::ToLocalPoint(const Vector2D& worldPoint) const {
    Matrix2x2 invRot = Matrix2x2::Rotation(-rotation);
    return invRot * (worldPoint - position);
}

Vector2D Transform::ToWorldVector(const Vector2D& localVector) const {
    Matrix2x2 rot = Matrix2x2::Rotation(rotation);
    return rot * localVector;
}

Vector2D Transform::ToLocalVector(const Vector2D& worldVector) const {
    Matrix2x2 invRot = Matrix2x2::Rotation(-rotation);
    return invRot * worldVector;
}

}
