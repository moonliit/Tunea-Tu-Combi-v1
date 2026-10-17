#include "math/Spline.hpp"

namespace Math {

Vector2D EvaluateCatmullRom(Vector2D p0, Vector2D p1, Vector2D p2, Vector2D p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    float x = 0.5f * ((2.0f * p1.x) +
                      (-p0.x + p2.x) * t +
                      (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
                      (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);

    float y = 0.5f * ((2.0f * p1.y) +
                      (-p0.y + p2.y) * t +
                      (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
                      (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);

    return {x, y};
}

std::vector<Vector2D> SampleCatmullRom(const std::vector<Vector2D>& controlPoints, int resolution) {
    std::vector<Vector2D> sampledVertices;
    if (controlPoints.size() < 2) return sampledVertices;

    std::vector<Vector2D> extendedPoints = controlPoints;
    extendedPoints.insert(extendedPoints.begin(), controlPoints.front());
    extendedPoints.push_back(controlPoints.back());

    for (size_t i = 1; i < extendedPoints.size() - 2; ++i) {
        Vector2D p0 = extendedPoints[i - 1];
        Vector2D p1 = extendedPoints[i];
        Vector2D p2 = extendedPoints[i + 1];
        Vector2D p3 = extendedPoints[i + 2];

        for (int j = 0; j < resolution; ++j) {
            float t = static_cast<float>(j) / static_cast<float>(resolution);
            Vector2D evaluated = EvaluateCatmullRom(p0, p1, p2, p3, t);
            sampledVertices.push_back({evaluated.x, evaluated.y});
        }
    }

    sampledVertices.push_back({controlPoints.back().x, controlPoints.back().y});
    return sampledVertices;
}

}
