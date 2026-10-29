#ifndef EDITOR_GRID_HPP
#define EDITOR_GRID_HPP

#include "game/Part.hpp"
#include "math/GridCoord.hpp"
#include "math/Vector2D.hpp"
#include <optional>
#include <vector>

class EditorGrid {
private:
    Math::GridCoord m_gridDims;
    Math::Vector2D m_initialPos;
    float m_cellSize;

    std::vector<std::optional<Part>> m_cells;
    std::vector<Part*> m_activeParts;

    size_t GetIndex(Math::GridCoord coord) const;
    bool IsGridCoordWithinSelfBounds(Math::GridCoord coord) const;
    bool IsWorldPosWithinSelfBounds(Math::Vector2D pos) const;

public:
    EditorGrid(Math::GridCoord gridDims, Math::Vector2D initialPos, float cellSize);

    const std::optional<Part>& GetPart(Math::GridCoord coord) const;
    bool PlacePart(Math::GridCoord coord, PartType type);
    void RemovePart(Math::GridCoord coord);
    void Clear();

    const std::vector<Part*>& GetActiveParts() { return m_activeParts; }
    float GetCellSize() const { return m_cellSize; }

    void DrawSelf();
    std::optional<Math::GridCoord> WorldToCoord(Math::Vector2D v) const;
    Math::Vector2D CoordToWorld(Math::GridCoord coord) const;
};

#endif
