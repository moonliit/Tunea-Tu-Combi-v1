#include "math/Terrain.hpp"
#include "math/Spline.hpp"

#include <algorithm>
#include <cmath>
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
    return Math::SampleCatmullRom(
        segment.points,
        std::max(1, segment.resolution)
    );
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
    mesh.indices = TriangulatePolygon(mesh.vertices);
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
    mesh.indices = TriangulatePolygon(mesh.vertices);
    if (mesh.indices.empty()) {
        mesh.vertices.clear();
        return mesh;
    }

    mesh.top = mesh.vertices;
    mesh.closed = true;
    return mesh;
}

}
