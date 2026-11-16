#ifndef MATH_SPLINE_HPP
#define MATH_SPLINE_HPP

#include "math/Vector2D.hpp"
#include <vector>

namespace Math {
    Vector2D EvaluateCatmullRom(Vector2D p0, Vector2D p1, Vector2D p2, Vector2D p3, float t);
    std::vector<Vector2D> SampleCatmullRom(const std::vector<Vector2D>& controlPoints, int resolution);
    std::vector<Vector2D> SampleCatmullRomAdaptive(
        const std::vector<Vector2D>& controlPoints, float tolerance = 1.0f);
}

#endif
