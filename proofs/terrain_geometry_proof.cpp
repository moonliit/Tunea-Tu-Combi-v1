#include "math/Terrain.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

const char* IssueName(Math::PolygonIssue issue) {
    switch (issue) {
        case Math::PolygonIssue::None: return "valido";
        case Math::PolygonIssue::TooFewVertices: return "pocos_vertices";
        case Math::PolygonIssue::ShortEdge: return "arista_corta";
        case Math::PolygonIssue::SmallArea: return "area_pequena";
        case Math::PolygonIssue::SelfIntersection: return "autointerseccion";
        case Math::PolygonIssue::TriangulationFailed: return "triangulacion_fallida";
    }
    return "desconocido";
}

double Area(const std::vector<Math::Vector2D>& polygon) {
    double twiceArea = 0.0;
    for (size_t i = 0; i < polygon.size(); ++i) {
        const auto& a = polygon[i];
        const auto& b = polygon[(i+1)%polygon.size()];
        twiceArea += double(a.x)*double(b.y)-double(b.x)*double(a.y);
    }
    return std::abs(twiceArea)*0.5;
}

void EmitCase(const std::string& name,
              const std::vector<Math::Vector2D>& polygon,
              Math::PolygonIssue expected) {
    const auto result = Math::TriangulateTerrainPolygon(polygon);
    if (result.validation.issue != expected) {
        throw std::runtime_error(name + ": diagnóstico inesperado");
    }
    const size_t expectedTriangles = polygon.size() >= 3
        ? polygon.size()-2 : 0;
    const size_t triangles = result.indices.size()/3;
    if (result.validation.valid() && triangles != expectedTriangles) {
        throw std::runtime_error(name + ": faltan triángulos");
    }
    if (!result.validation.valid() && triangles != 0) {
        throw std::runtime_error(name + ": se aceptó un contorno inválido");
    }
    std::set<size_t> badEdges(result.validation.badEdges.begin(),
                              result.validation.badEdges.end());
    std::set<size_t> badVertices(result.validation.badVertices.begin(),
                                 result.validation.badVertices.end());
    std::cout << name << ',' << polygon.size() << ',' << Area(polygon)
        << ',' << IssueName(result.validation.issue)
        << ',' << badEdges.size() << ',' << badVertices.size()
        << ',' << triangles << ',' << expectedTriangles << '\n';
}

Math::TerrainDefinition ReadTerrain(const std::string& name) {
    const std::string path = std::string(TERRAIN_PROOF_SOURCE_DIR) +
        "/assets/levels/" + name + ".json";
    std::ifstream file(path);
    if (!file) throw std::runtime_error("No se pudo leer " + path);
    const auto data = nlohmann::json::parse(file);
    Math::TerrainDefinition terrain;
    for (const auto& polygonData : data.at("terrain").at("polygons")) {
        Math::TerrainPolygon polygon;
        for (const auto& segmentData : polygonData.at("segments")) {
            Math::TerrainSegment segment;
            segment.type = segmentData.at("type") == "CatmullRom"
                ? Math::TerrainSegmentType::CatmullRom
                : Math::TerrainSegmentType::Line;
            segment.resolution = segmentData.value("resolution", 8);
            for (const auto& point : segmentData.at("points")) {
                segment.points.push_back({
                    point.at("x").get<float>(), point.at("y").get<float>()});
            }
            polygon.segments.push_back(std::move(segment));
        }
        terrain.polygons.push_back(std::move(polygon));
    }
    return terrain;
}

}

int main() {
    try {
        using Math::PolygonIssue;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "caso,vertices,area_u2,diagnostico,aristas_marcadas,"
            "vertices_marcados,triangulos,triangulos_esperados\n";
        EmitCase("cuadrado", {{0,0}, {100,0}, {100,100}, {0,100}},
            PolygonIssue::None);
        EmitCase("concavo", {{0,0}, {100,0}, {100,100}, {50,50}, {0,100}},
            PolygonIssue::None);
        EmitCase("cruce", {{0,0}, {100,100}, {0,100}, {100,0}},
            PolygonIssue::SelfIntersection);
        EmitCase("toque", {{0,0}, {100,0}, {50,0}, {100,100}, {0,100}},
            PolygonIssue::SelfIntersection);
        EmitCase("repetido", {{0,0}, {100,0}, {100,0}, {0,100}},
            PolygonIssue::ShortEdge);
        EmitCase("collineal", {{0,0}, {100,0}, {200,0}},
            PolygonIssue::SmallArea);
        EmitCase("dos_puntos", {{0,0}, {100,0}},
            PolygonIssue::TooFewVertices);

        for (const char* level : {"default", "level_01", "level_02",
                                  "level_03", "level_04", "level_05",
                                  "level_06", "level_07", "level_08",
                                  "level_09", "level_10", "level_11",
                                  "level_12", "level_13"}) {
            const auto terrain = ReadTerrain(level);
            const std::string levelName = level;
            if (levelName == "default" || levelName == "level_02" ||
                levelName == "level_03" || levelName == "level_05" ||
                levelName == "level_07") {
                const bool hasCurve = std::any_of(
                    terrain.polygons.begin(), terrain.polygons.end(),
                    [](const Math::TerrainPolygon& polygon) {
                        return std::any_of(polygon.segments.begin(),
                            polygon.segments.end(),
                            [](const Math::TerrainSegment& segment) {
                                return segment.type ==
                                    Math::TerrainSegmentType::CatmullRom &&
                                    segment.points.size() >= 3;
                            });
                    });
                if (!hasCurve) {
                    throw std::runtime_error(levelName + ": falta la curva esperada");
                }
            }
            const auto sections = Math::SampleTerrain(terrain);
            if (sections.empty()) {
                throw std::runtime_error(std::string(level) + ": sin contorno");
            }
            for (size_t i = 0; i < sections.size(); ++i) {
                auto polygon = sections[i];
                if (polygon.front() != polygon.back()) {
                    throw std::runtime_error(std::string(level) + ": contorno abierto");
                }
                polygon.pop_back();
                EmitCase(std::string(level) + "_" + std::to_string(i+1),
                    polygon, PolygonIssue::None);
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
