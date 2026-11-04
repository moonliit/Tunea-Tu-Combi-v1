#include "game/Level.hpp"
#include <utility>

Level::Level(LevelSpecs specs):
    m_definition(std::move(specs)),
    m_buildModel(
        m_definition.grid.dims,
        m_definition.grid.worldPos,
        m_definition.grid.cellSize
    )
{
    if (m_definition.terrain.polygons.empty() &&
        m_definition.terrainControlPoints.size() >= 2) {
        m_definition.terrain.polygons.push_back(TerrainPolygon{
            .segments = {
                TerrainSegment{
                    .type = TerrainSegmentType::CatmullRom,
                    .points = std::move(m_definition.terrainControlPoints),
                    .resolution = m_definition.catmullRomResolution
                }
            }
        });
    }
    const auto sections = SampleTerrain(m_definition.terrain);
    if (!sections.empty()) {
        m_catmullRomPoints = sections.front();
    }
}
