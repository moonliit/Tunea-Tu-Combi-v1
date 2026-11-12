#ifndef MATH_TERRAIN_HPP
#define MATH_TERRAIN_HPP

#include "math/Vector2D.hpp"
#include <vector>
#include <cstdint>

namespace Math {

enum class TerrainSegmentType {
    Line,
    CatmullRom
};

struct TerrainSegment {
    TerrainSegmentType type = TerrainSegmentType::Line;
    std::vector<Math::Vector2D> points;
    int resolution = 8;
};

struct TerrainPolygon {
    std::vector<TerrainSegment> segments;
};

struct TerrainDefinition {
    std::vector<TerrainPolygon> polygons;
};

struct TerrainSample {
    Math::Vector2D position;
    Math::Vector2D tangent;
    Math::Vector2D normal;
    float distance = 0.0f;
};

struct TerrainVisualDefinition {
    float baseY = 700.0f;
    std::uint8_t fillR = 255;
    std::uint8_t fillG = 255;
    std::uint8_t fillB = 255;
    std::uint8_t fillA = 255;
    std::uint8_t outlineR = 70;
    std::uint8_t outlineG = 45;
    std::uint8_t outlineB = 28;
    std::uint8_t outlineA = 255;
};

struct SampledTerrainSection {
    std::vector<TerrainSample> samples;
};

struct TerrainMesh {
    std::vector<Math::Vector2D> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<Math::Vector2D> top;
    std::vector<Math::Vector2D> bottom;
    bool closed = false;
};

std::vector<std::vector<Math::Vector2D>> SampleTerrain(
    const TerrainDefinition& definition
);
std::vector<SampledTerrainSection> SampleTerrainGeometry(
    const TerrainDefinition& definition
);
TerrainMesh BuildTerrainMesh(
    const SampledTerrainSection& section,
    const TerrainVisualDefinition& visual
);
TerrainMesh BuildTerrainPolygonMesh(
    const std::vector<Math::Vector2D>& polygon
);

};

#endif
