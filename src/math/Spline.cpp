#include "math/Spline.hpp"
#include <algorithm>
#include <cmath>

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

std::vector<Vector2D> SampleCatmullRomAdaptive(
    const std::vector<Vector2D>& controlPoints, float tolerance) {
    std::vector<Vector2D> sampled;
    if (controlPoints.size() < 2) return sampled;
    const float limit = std::max(tolerance, 0.01f);
    const auto refine = [&](const auto& self, Vector2D p0, Vector2D p1,
                            Vector2D p2, Vector2D p3, float t0, float t1,
                            Vector2D a, Vector2D b, int depth) -> void {
        const float tm = (t0 + t1) * 0.5f;
        const Vector2D mid = EvaluateCatmullRom(p0,p1,p2,p3,tm);
        const Vector2D quarter = EvaluateCatmullRom(p0,p1,p2,p3,(t0+tm)*0.5f);
        const Vector2D threeQuarter = EvaluateCatmullRom(p0,p1,p2,p3,(tm+t1)*0.5f);
        const auto error = [&](const Vector2D& curvePoint, float fraction) {
            return std::hypot(
                float(curvePoint.x) - (float(a.x)+(float(b.x)-float(a.x))*fraction),
                float(curvePoint.y) - (float(a.y)+(float(b.y)-float(a.y))*fraction));
        };
        if (depth < 12 && std::max({error(quarter,0.25f),
                error(mid,0.5f), error(threeQuarter,0.75f)}) > limit) {
            self(self,p0,p1,p2,p3,t0,tm,a,mid,depth+1);
            self(self,p0,p1,p2,p3,tm,t1,mid,b,depth+1);
        } else {
            sampled.push_back(b);
        }
    };
    sampled.push_back(controlPoints.front());
    for (size_t i = 0; i+1 < controlPoints.size(); ++i) {
        const Vector2D p0 = controlPoints[i == 0 ? i : i-1];
        const Vector2D p1 = controlPoints[i];
        const Vector2D p2 = controlPoints[i+1];
        const Vector2D p3 = controlPoints[i+2 < controlPoints.size() ? i+2 : i+1];
        refine(refine,p0,p1,p2,p3,0.0f,1.0f,p1,p2,0);
    }
    return sampled;
}

}
