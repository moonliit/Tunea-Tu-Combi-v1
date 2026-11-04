#ifndef LEVEL_HPP
#define LEVEL_HPP

#include "math/Vector2D.hpp"
#include "game/BuildModel.hpp"
#include <vector>

struct LevelSpecs {
    Math::Vector2D initialCameraPos;
    std::vector<Math::Vector2D> terrainControlPoints;
    int catmullRomResolution;
    Math::GridCoord gridDims;
    Math::Vector2D gridWorldPos;
    float cellSize;
};

class Level {
private:
    std::vector<Math::Vector2D> m_catmullRomPoints;
    BuildModel m_buildModel;

public:
    Level(LevelSpecs specs);

    const std::vector<Math::Vector2D>& GetCatmullromPoints() const { return m_catmullRomPoints; }
    BuildModel& GetBuildModel() { return m_buildModel; }
    const BuildModel& GetBuildModel() const { return m_buildModel; }
    EditorGrid& GetGrid() { return m_buildModel.GetGrid(); }
    const EditorGrid& GetGrid() const { return m_buildModel.GetGrid(); }
};

#endif
