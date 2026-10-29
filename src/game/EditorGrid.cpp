#include "game/EditorGrid.hpp"
#include "math/GridCoord.hpp"
#include "math/Vector2D.hpp"
#include "raylib.h"
#include <cmath>

using namespace Math;

EditorGrid::EditorGrid(GridCoord gridDims, Vector2D initialPos, float cellSize):
    m_gridDims(gridDims), m_initialPos(initialPos), m_cellSize(cellSize)
{
    const int mult = gridDims.x * gridDims.y;
    m_cells.resize(mult, std::nullopt);
}

size_t EditorGrid::GetIndex(GridCoord coord) const {
    return coord.y * m_gridDims.x + coord.x;
}

bool EditorGrid::IsGridCoordWithinSelfBounds(Math::GridCoord coord) const {
    return (
        coord.x >= 0 && coord.x < m_gridDims.x &&
        coord.y >= 0 && coord.y < m_gridDims.y
    );
}

bool EditorGrid::IsWorldPosWithinSelfBounds(Math::Vector2D pos) const {
    const float width = m_cellSize * m_gridDims.x;
    const float height = m_cellSize * m_gridDims.y;

    float left = m_initialPos.x - (width / 2.0f);
    float right = m_initialPos.x + (width / 2.0f);
    float top = m_initialPos.y - height;
    float bottom = m_initialPos.y;

    return (
        pos.x >= left && pos.x < right &&
        pos.y >= top && pos.y < bottom
    );
}

const std::optional<Part>& EditorGrid::GetPart(GridCoord coord) const {
    size_t idx = GetIndex(coord);
    return m_cells[idx];
}

bool EditorGrid::PlacePart(GridCoord coord, PartType type) {
    if (!IsGridCoordWithinSelfBounds(coord)) return false;
    size_t idx = GetIndex(coord);

    if (m_cells[idx].has_value()) {
        RemovePart(coord);
    }

    m_cells[idx] = Part(coord, type);
    m_activeParts.push_back(&m_cells[idx].value());
    m_cells[idx].value().m_activeCacheIndex = m_activeParts.size() - 1;
    return true;
}

void EditorGrid::RemovePart(GridCoord coord) {
    if (!IsGridCoordWithinSelfBounds(coord)) return;
    size_t idx = GetIndex(coord);

    if (!m_cells[idx].has_value()) return;

    Part& partToRemove = m_cells[idx].value();
    size_t cacheIdx = partToRemove.m_activeCacheIndex;

    if (cacheIdx < m_activeParts.size() - 1) {
        m_activeParts[cacheIdx] = m_activeParts.back();
        m_activeParts[cacheIdx]->m_activeCacheIndex = cacheIdx;
    }
    m_activeParts.pop_back();
    m_cells[idx].reset();
}

void EditorGrid::Clear() {
    const int mult = m_gridDims.x * m_gridDims.y;
    m_cells.assign(mult, std::nullopt);
    m_activeParts.clear();
}

void EditorGrid::DrawSelf() {
    const float sizeMultiplier = 0.9f;
    const float drawnSize = m_cellSize * sizeMultiplier;
    const float offsetPos = drawnSize / 2.0f;

    for (int x = 0; x < m_gridDims.x; x++) {
        for (int y = 0; y < m_gridDims.y; y++) {
            Math::GridCoord coord {x, y};
            Math::Vector2D pos = CoordToWorld(coord);
            DrawRectangleRec(
                Rectangle{
                    pos.x - offsetPos,
                    pos.y - offsetPos,
                    drawnSize, drawnSize
                },
                Fade(LIGHTGRAY, 100)
            );
        }
    }
}

std::optional<GridCoord> EditorGrid::WorldToCoord(Vector2D v) const {
    if (!IsWorldPosWithinSelfBounds(v)) {
        return std::nullopt;
    }

    const float width = m_cellSize * m_gridDims.x;
    const float height = m_cellSize * m_gridDims.y;

    // Offset relative to the top-left corner of the grid bounding box
    float relX = v.x - (m_initialPos.x - (width / 2.0f));
    float relY = v.y - (m_initialPos.y - height);

    int x = static_cast<int>(std::floor(relX / m_cellSize));
    int y = static_cast<int>(std::floor(relY / m_cellSize));

    Math::GridCoord coord{x, y};
    if (IsGridCoordWithinSelfBounds(coord)) {
        return coord;
    }

    return std::nullopt;
}

Vector2D EditorGrid::CoordToWorld(GridCoord coord) const {
    const float width = m_cellSize * m_gridDims.x;
    const float height = m_cellSize * m_gridDims.y;

    RealNum x = coord.x * m_cellSize + m_cellSize / 2.0f;
    RealNum y = coord.y * m_cellSize + m_cellSize / 2.0f;

    return Vector2D {
        x + m_initialPos.x - width / 2,
        y + m_initialPos.y - height,
    };
}
