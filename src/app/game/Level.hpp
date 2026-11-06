#ifndef GAME_LEVEL_HPP
#define GAME_LEVEL_HPP

#include "app/game/BuildModel.hpp"
#include "math/Vector2D.hpp"
#include "math/Terrain.hpp"
#include "json.hpp"
#include <optional>
#include <vector>

struct CameraDefinition {
    Math::Vector2D target;
    float zoom = 1.0f;
};

struct GoalDefinition {
    Math::Vector2D position;
};

struct GridDefinition {
    Math::GridCoord dims;
    Math::Vector2D worldPos;
    float cellSize;
};

inline constexpr Math::Vector2D GoalSize{80.0f, 80.0f};

struct LevelDefinition {
    Math::TerrainDefinition terrain;
    GridDefinition grid;
    std::optional<CameraDefinition> initialCamera;
    GoalDefinition goal;
    std::vector<Math::Vector2D> terrainControlPoints;
    int catmullRomResolution = 8;

    static LevelDefinition Default();
    static LevelDefinition FromJson(const nlohmann::json& j);
    static LevelDefinition FromJsonPath(const std::string& path);
    nlohmann::json ToJson() const;
};

class Level {
private:
    LevelDefinition m_definition;
    std::vector<Math::Vector2D> m_catmullRomPoints;
    BuildModel m_buildModel;

public:
    Level(const LevelDefinition& specs);

    const LevelDefinition& GetDefinition() const { return m_definition; }
    const std::vector<Math::Vector2D>& GetCatmullromPoints() const { return m_catmullRomPoints; }
    const Math::TerrainDefinition& GetTerrain() const {
        return m_definition.terrain;
    }
    BuildModel& GetBuildModel() { return m_buildModel; }
    const BuildModel& GetBuildModel() const { return m_buildModel; }
    EditorGrid& GetGrid() { return m_buildModel.GetGrid(); }
    const EditorGrid& GetGrid() const { return m_buildModel.GetGrid(); }
};

#endif
