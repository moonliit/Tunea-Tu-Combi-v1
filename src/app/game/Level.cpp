#include "app/game/Level.hpp"
#include "math/Vector2D.hpp"
#include <algorithm>
#include <fstream>

Level::Level(const LevelDefinition& specs):
    m_definition(std::move(specs)),
    m_buildModel(
        m_definition.grid.dims,
        m_definition.grid.worldPos,
        m_definition.grid.cellSize
    )
{
    if (m_definition.terrain.polygons.empty() &&
        m_definition.terrainControlPoints.size() >= 2) {
        m_definition.terrain.polygons.push_back(Math::TerrainPolygon{
            .segments = {
            Math::TerrainSegment{
                    .type = Math::TerrainSegmentType::CatmullRom,
                    .points = std::move(m_definition.terrainControlPoints),
                    .resolution = m_definition.catmullRomResolution
                }
            }
        });
    }
    const std::vector<std::vector<Math::Vector2D>> sections =
        Math::SampleTerrain(m_definition.terrain);
    if (!sections.empty()) {
        m_catmullRomPoints = sections.front();
    }
}

Math::AABB Level::GetContentBounds(float margin) const {
    const EditorGrid& grid = m_buildModel.GetGrid();
    const Math::Vector2D firstCell = grid.CoordToWorld({0, 0});
    const Math::Vector2D lastCell = grid.CoordToWorld({
        m_definition.grid.dims.x - 1,
        m_definition.grid.dims.y - 1
    });
    const float halfCell = grid.GetCellSize() * 0.5f;
    float minX = static_cast<float>(firstCell.x) - halfCell;
    float minY = static_cast<float>(firstCell.y) - halfCell;
    float maxX = static_cast<float>(lastCell.x) + halfCell;
    float maxY = static_cast<float>(lastCell.y) + halfCell;

    const auto include = [&](float x, float y) {
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
    };

    for (const auto& section : Math::SampleTerrain(m_definition.terrain)) {
        for (const Math::Vector2D& point : section) {
            include(static_cast<float>(point.x), static_cast<float>(point.y));
        }
    }
    const float goalX = static_cast<float>(m_definition.goal.position.x);
    const float goalY = static_cast<float>(m_definition.goal.position.y);
    const float halfGoalX = static_cast<float>(GoalSize.x) * 0.5f;
    const float halfGoalY = static_cast<float>(GoalSize.y) * 0.5f;
    include(goalX - halfGoalX, goalY - halfGoalY);
    include(goalX + halfGoalX, goalY + halfGoalY);
    for (const PassengerDefinition& passenger : m_definition.passengers) {
        include(
            static_cast<float>(passenger.position.x),
            static_cast<float>(passenger.position.y)
        );
    }
    return Math::AABB(
        Math::Vector2D{minX - margin, minY - margin},
        Math::Vector2D{maxX + margin, maxY + margin}
    );
}

LevelDefinition LevelDefinition::Default() {
    return LevelDefinition {
        .terrain = Math::TerrainDefinition{},
        .grid = GridDefinition{
            .dims = {6, 4},
            .worldPos = {-50.0f, 480.0f},
            .cellSize = 40.0f
        },
        .initialCamera = CameraDefinition{
            .target = {300.0f, 360.0f},
            .zoom = 0.72f
        },
        .goal = GoalDefinition{
            .position = {900.0f, 430.0f}
        }
    };
}

nlohmann::json LevelDefinition::ToJson() const {
    nlohmann::json j;

    // build terrain polygons explicitly
    nlohmann::json polygonsJson = nlohmann::json::array();
    for (const auto& poly : terrain.polygons) {
        nlohmann::json segmentsJson = nlohmann::json::array();
        for (const auto& seg : poly.segments) {
            nlohmann::json pointsJson = nlohmann::json::array();
            for (const auto& pt : seg.points) {
                pointsJson.push_back(pt.ToJson());
            }

            segmentsJson.push_back({
                {"type", (seg.type == Math::TerrainSegmentType::CatmullRom) ? "CatmullRom" : "Line"},
                {"points", pointsJson},
                {"resolution", seg.resolution}
            });
        }
        polygonsJson.push_back({{"segments", segmentsJson}});
    }
    j["terrain"]["polygons"] = polygonsJson;

    j["grid"] = {
        {"dims", {grid.dims.x, grid.dims.y}},
        {"worldPos", grid.worldPos.ToJson()},
        {"cellSize", grid.cellSize}
    };

    if (initialCamera.has_value()) {
        j["initialCamera"] = {
            {"target", initialCamera.value().target.ToJson()},
            {"zoom", initialCamera.value().zoom}
        };
    }

    j["goal"] = {
        {"position", goal.position.ToJson()}
    };

    j["passengers"] = nlohmann::json::array();
    for (const auto& passenger : passengers) {
        j["passengers"].push_back({
            {"position", passenger.position.ToJson()},
            {"hasSkirt", passenger.hasSkirt}
        });
    }

    return j;
}

LevelDefinition LevelDefinition::FromJson(const nlohmann::json& j) {
    LevelDefinition level;

    if (j.contains("terrain") && j["terrain"].contains("polygons")) {
        for (const auto& polyJson : j["terrain"]["polygons"]) {
            Math::TerrainPolygon poly;
            for (const auto& segJson : polyJson["segments"]) {
                Math::TerrainSegment seg;
                std::string typeStr = segJson.at("type");
                seg.type = (typeStr == "CatmullRom") ? Math::TerrainSegmentType::CatmullRom : Math::TerrainSegmentType::Line;

                if (segJson.contains("resolution")) {
                    seg.resolution = segJson.at("resolution");
                }

                for (const auto& ptJson : segJson.at("points")) {
                    seg.points.push_back(Math::Vector2D::FromJson(ptJson));
                }
                poly.segments.push_back(seg);
            }
            level.terrain.polygons.push_back(poly);
        }
    }

    if (j.contains("grid")) {
        auto dimsArr = j["grid"].at("dims");
        level.grid.dims = {dimsArr[0].get<int>(), dimsArr[1].get<int>()};
        level.grid.worldPos = Math::Vector2D::FromJson(j["grid"].at("worldPos"));
        level.grid.cellSize = j["grid"].at("cellSize");
    }

    if (j.contains("initialCamera")) {
        CameraDefinition cam;
        cam.target = Math::Vector2D::FromJson(j["initialCamera"].at("target"));
        cam.zoom = j["initialCamera"].at("zoom");
        level.initialCamera = cam;
    }

    if (j.contains("goal")) {
        level.goal.position = Math::Vector2D::FromJson(j["goal"].at("position"));
    }

    if (j.contains("passengers") && j["passengers"].is_array()) {
        for (const auto& passengerJson : j["passengers"]) {
            level.passengers.push_back({
                .position = Math::Vector2D::FromJson(passengerJson.at("position")),
                .hasSkirt = passengerJson.value("hasSkirt", false)
            });
        }
    }

    return level;
}

LevelDefinition LevelDefinition::FromJsonPath(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + path);
    }
    return LevelDefinition::FromJson(nlohmann::json::parse(file));
}
