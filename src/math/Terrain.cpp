#include "math/Terrain.hpp"
#include "math/Spline.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
using namespace Math;

namespace Math {

float SignedArea(const std::vector<Math::Vector2D>& polygon) {
    float area = 0.0f;
    for (size_t index = 0; index < polygon.size(); ++index) {
        const auto& current = polygon[index];
        const auto& next = polygon[(index + 1) % polygon.size()];
        area += static_cast<float>(current.x * next.y - next.x * current.y);
    }
    return area * 0.5f;
}

namespace {
constexpr double MinimumEdge = 0.5;
constexpr double MinimumArea = 1.0;

double Cross(const Vector2D& a, const Vector2D& b, const Vector2D& c) {
    return (double(b.x) - double(a.x)) * (double(c.y) - double(a.y)) -
        (double(b.y) - double(a.y)) * (double(c.x) - double(a.x));
}

bool OnSegment(const Vector2D& a, const Vector2D& b,
               const Vector2D& p, double tolerance) {
    return std::abs(Cross(a, b, p)) <= tolerance &&
        double(p.x) >= std::min(double(a.x), double(b.x)) - 1e-6 &&
        double(p.x) <= std::max(double(a.x), double(b.x)) + 1e-6 &&
        double(p.y) >= std::min(double(a.y), double(b.y)) - 1e-6 &&
        double(p.y) <= std::max(double(a.y), double(b.y)) + 1e-6;
}

bool EdgesIntersect(const Vector2D& a, const Vector2D& b,
                    const Vector2D& c, const Vector2D& d) {
    const double scale = std::max({1.0, std::hypot(double(b.x)-double(a.x), double(b.y)-double(a.y)),
        std::hypot(double(d.x)-double(c.x), double(d.y)-double(c.y))});
    const double tolerance = 1e-8 * scale * scale;
    const double o1 = Cross(a,b,c), o2 = Cross(a,b,d);
    const double o3 = Cross(c,d,a), o4 = Cross(c,d,b);
    if ((o1 > tolerance && o2 < -tolerance || o1 < -tolerance && o2 > tolerance) &&
        (o3 > tolerance && o4 < -tolerance || o3 < -tolerance && o4 > tolerance)) return true;
    return OnSegment(a,b,c,tolerance) || OnSegment(a,b,d,tolerance) ||
        OnSegment(c,d,a,tolerance) || OnSegment(c,d,b,tolerance);
}
}

PolygonValidation ValidateTerrainPolygon(const std::vector<Vector2D>& polygon) {
    PolygonValidation result;
    const size_t n = polygon.size();
    if (n < 3) { result.issue = PolygonIssue::TooFewVertices; return result; }
    for (size_t i = 0; i < n; ++i) {
        const auto& a = polygon[i];
        const auto& b = polygon[(i+1)%n];
        if (std::hypot(double(b.x)-double(a.x), double(b.y)-double(a.y)) < MinimumEdge) {
            result.badEdges.push_back(i);
            result.badVertices.push_back(i);
            result.badVertices.push_back((i+1)%n);
        }
    }
    if (!result.badEdges.empty()) { result.issue = PolygonIssue::ShortEdge; return result; }
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i+1; j < n; ++j) {
            if (j == i+1 || (i == 0 && j == n-1)) continue;
            if (EdgesIntersect(polygon[i], polygon[(i+1)%n],
                               polygon[j], polygon[(j+1)%n])) {
                result.badEdges.push_back(i);
                result.badEdges.push_back(j);
                result.badVertices.insert(result.badVertices.end(),
                    {i, (i+1)%n, j, (j+1)%n});
            }
        }
    }
    if (!result.badEdges.empty()) { result.issue = PolygonIssue::SelfIntersection; return result; }
    double twiceArea = 0;
    for (size_t i = 0; i < n; ++i) {
        const auto& a = polygon[i]; const auto& b = polygon[(i+1)%n];
        twiceArea += double(a.x)*double(b.y)-double(b.x)*double(a.y);
    }
    if (std::abs(twiceArea) < 2*MinimumArea) {
        result.issue = PolygonIssue::SmallArea;
        result.badVertices.resize(n);
        std::iota(result.badVertices.begin(), result.badVertices.end(), 0);
    }
    return result;
}

const char* PolygonIssueMessage(PolygonIssue issue) {
    switch (issue) {
        case PolygonIssue::None: return "Contorno válido";
        case PolygonIssue::TooFewVertices: return "Se necesitan tres vértices";
        case PolygonIssue::ShortEdge: return "Vértices repetidos o arista muy corta";
        case PolygonIssue::SmallArea: return "Área demasiado pequeña";
        case PolygonIssue::SelfIntersection: return "Las aristas se cruzan o se tocan";
        case PolygonIssue::TriangulationFailed: return "No se pudo triangular el contorno";
    }
    return "Error geométrico";
}

bool IsInsideTriangle(
    const Math::Vector2D& point,
    const Math::Vector2D& a,
    const Math::Vector2D& b,
    const Math::Vector2D& c
) {
    const float first = Vector2D::TripleCross(a, b, point);
    const float second = Vector2D::TripleCross(b, c, point);
    const float third = Vector2D::TripleCross(c, a, point);
    return (
        first >= -RealNum::epsilon &&
        second >= -RealNum::epsilon &&
        third >= -RealNum::epsilon
    );
}

std::vector<std::uint32_t> TriangulatePolygon(
    const std::vector<Math::Vector2D>& polygon
) {
    std::vector<std::uint32_t> triangles;
    if (polygon.size() < 3) return triangles;

    std::vector<std::uint32_t> remaining;
    remaining.reserve(polygon.size());
    if (SignedArea(polygon) > 0.0f) {
        for (std::uint32_t index = 0; index < polygon.size(); ++index) {
            remaining.push_back(index);
        }
    } else {
        for (std::uint32_t index = polygon.size(); index > 0; --index) {
            remaining.push_back(index - 1);
        }
    }

    while (remaining.size() > 3) {
        bool clipped = false;
        for (size_t index = 0; index < remaining.size(); ++index) {
            const std::uint32_t previous =
                remaining[(index + remaining.size() - 1) % remaining.size()];
            const std::uint32_t current = remaining[index];
            const std::uint32_t next =
                remaining[(index + 1) % remaining.size()];
            if (Vector2D::TripleCross(
                    polygon[previous],
                    polygon[current],
                    polygon[next]
                ) <= 0.0001f) {
                continue;
            }

            bool containsVertex = false;
            for (const std::uint32_t candidate : remaining) {
                if (candidate == previous ||
                    candidate == current ||
                    candidate == next) {
                    continue;
                }
                if (IsInsideTriangle(
                        polygon[candidate],
                        polygon[previous],
                        polygon[current],
                        polygon[next]
                    )) {
                    containsVertex = true;
                    break;
                }
            }
            if (containsVertex) continue;

            triangles.insert(
                triangles.end(),
                {previous, current, next}
            );
            remaining.erase(remaining.begin() + index);
            clipped = true;
            break;
        }
        if (!clipped) {
            triangles.clear();
            return triangles;
        }
    }

    triangles.insert(
        triangles.end(),
        {remaining[0], remaining[1], remaining[2]}
    );
    return triangles;
}

PolygonTriangulation TriangulateTerrainPolygon(const std::vector<Vector2D>& polygon) {
    PolygonTriangulation result;
    result.validation = ValidateTerrainPolygon(polygon);
    if (!result.validation.valid()) return result;
    result.indices = TriangulatePolygon(polygon);
    bool failed = result.indices.size() != (polygon.size()-2)*3;
    double triangleArea = 0.0;
    for (size_t i = 0; !failed && i+2 < result.indices.size(); i += 3) {
        const double twiceArea = Cross(polygon[result.indices[i]],
            polygon[result.indices[i+1]], polygon[result.indices[i+2]]);
        if (twiceArea <= 1e-6) failed = true;
        triangleArea += twiceArea * 0.5;
    }
    if (!failed && std::abs(triangleArea - std::abs(double(SignedArea(polygon)))) >
            std::max(0.01, triangleArea * 1e-5)) failed = true;
    if (failed) {
        result.indices.clear();
        result.validation.issue = PolygonIssue::TriangulationFailed;
    }
    return result;
}

std::vector<Math::Vector2D> SampleLine(
    const std::vector<Math::Vector2D>& points
) {
    return points.size() >= 2
        ? points
        : std::vector<Math::Vector2D>{};
}

std::vector<Math::Vector2D> SampleSegment(const TerrainSegment& segment) {
    if (segment.type == TerrainSegmentType::Line) {
        return SampleLine(segment.points);
    }

    if (segment.points.size() < 2) return {};
    return Math::SampleCatmullRomAdaptive(segment.points, TerrainCurveTolerance);
}

SampledTerrainSection EnrichSection(
    const std::vector<Math::Vector2D>& points
) {
    SampledTerrainSection section;
    float distance = 0.0f;
    for (size_t index = 0; index < points.size(); ++index) {
        const auto previous = points[index == 0 ? 0 : index - 1];
        const auto next = points[index + 1 < points.size() ? index + 1 : index];
        Math::Vector2D tangent{next.x - previous.x, next.y - previous.y};
        const float length = std::sqrt(static_cast<float>(
            tangent.x * tangent.x + tangent.y * tangent.y
        ));
        if (length > 0.0f) {
            tangent.x /= length;
            tangent.y /= length;
        }
        if (index > 0) {
            const auto prior = points[index - 1];
            const auto current = points[index];
            const float dx = static_cast<float>(current.x - prior.x);
            const float dy = static_cast<float>(current.y - prior.y);
            distance += std::sqrt(dx * dx + dy * dy);
        }
        section.samples.push_back({
            .position = points[index],
            .tangent = tangent,
            .normal = {-tangent.y, tangent.x},
            .distance = distance
        });
    }
    return section;
}


std::vector<std::vector<Math::Vector2D>> SampleTerrain(
    const TerrainDefinition& definition
) {
    std::vector<std::vector<Math::Vector2D>> sections;
    sections.reserve(definition.polygons.size());
    for (const TerrainPolygon& polygon : definition.polygons) {
        std::vector<Math::Vector2D> boundary;
        for (const TerrainSegment& segment : polygon.segments) {
            std::vector<Math::Vector2D> sampled = SampleSegment(segment);
            if (sampled.empty()) continue;
            if (!boundary.empty() &&
                boundary.back() == sampled.front()) {
                sampled.erase(sampled.begin());
            }
            boundary.insert(
                boundary.end(),
                sampled.begin(),
                sampled.end()
            );
        }
        if (boundary.size() >= 3) {
            sections.push_back(std::move(boundary));
        }
    }
    return sections;
}

std::vector<SampledTerrainSection> SampleTerrainGeometry(
    const TerrainDefinition& definition
) {
    std::vector<SampledTerrainSection> sections;
    for (const auto& points : SampleTerrain(definition)) {
        sections.push_back(EnrichSection(points));
    }
    return sections;
}

TerrainMesh BuildTerrainMesh(
    const SampledTerrainSection& section,
    const TerrainVisualDefinition& visual
) {
    TerrainMesh mesh;
    for (const TerrainSample& sample : section.samples) {
        mesh.top.push_back(sample.position);
    }
    if (mesh.top.size() < 2) return mesh;

    mesh.bottom = {
        {mesh.top.front().x, visual.baseY},
        {mesh.top.back().x, visual.baseY}
    };
    mesh.vertices = mesh.top;
    mesh.vertices.insert(
        mesh.vertices.end(),
        mesh.bottom.rbegin(),
        mesh.bottom.rend()
    );
    mesh.indices = TriangulateTerrainPolygon(mesh.vertices).indices;
    if (mesh.indices.empty()) {
        mesh.vertices.clear();
    }
    return mesh;
}

TerrainMesh BuildTerrainPolygonMesh(
    const std::vector<Math::Vector2D>& polygon
) {
    TerrainMesh mesh;
    if (polygon.size() < 3) return mesh;

    if (polygon.front() != polygon.back()) {
        return mesh;
    }

    mesh.vertices.assign(polygon.begin(), polygon.end() - 1);
    if (mesh.vertices.size() < 3) {
        mesh.vertices.clear();
        return mesh;
    }
    mesh.indices = TriangulateTerrainPolygon(mesh.vertices).indices;
    if (mesh.indices.empty()) {
        mesh.vertices.clear();
        return mesh;
    }

    mesh.top = mesh.vertices;
    mesh.closed = true;
    return mesh;
}

}
