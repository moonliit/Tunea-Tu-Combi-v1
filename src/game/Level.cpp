#include "game/Level.hpp"
#include "game/EditorGrid.hpp"
#include "math/Spline.hpp"

Level::Level(LevelSpecs specs):
    m_editorGrid(specs.gridDims, specs.gridWorldPos, specs.cellSize)
{
    m_catmullRomPoints = Math::SampleCatmullRom(specs.terrainControlPoints, specs.catmullRomResolution);
}
