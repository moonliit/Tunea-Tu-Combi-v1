#ifndef LEVEL_HPP
#define LEVEL_HPP

#include "math/Vector2D.hpp"
#include "game/EditorGrid.hpp"
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
    EditorGrid m_editorGrid;

public:
    Level(LevelSpecs specs);

    const std::vector<Math::Vector2D>& GetCatmullromPoints() const { return m_catmullRomPoints; }
    EditorGrid& GetGrid() { return m_editorGrid; }
};

#endif
