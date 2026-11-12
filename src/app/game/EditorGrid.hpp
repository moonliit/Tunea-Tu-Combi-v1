#ifndef EDITOR_GRID_HPP
#define EDITOR_GRID_HPP

#include "app/game/Part.hpp"
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

    friend class Part;

public:
    EditorGrid(Math::GridCoord gridDims, Math::Vector2D initialPos, float cellSize);
    EditorGrid(const EditorGrid& other);
    EditorGrid& operator=(const EditorGrid& other);

    std::optional<Part>& GetPart(Math::GridCoord coord);
    const std::optional<Part>& GetPart(Math::GridCoord coord) const;
    bool PlacePart(
        Math::GridCoord coord,
        PartType type,
        PartOrientation orientation = PartOrientation::Up,
        PartId id = 0
    );
    void RemovePart(Math::GridCoord coord);
    void RotatePart(Math::GridCoord coord);
    void Clear();
    bool CanShiftContents(Math::GridCoord delta) const;
    bool ShiftContents(Math::GridCoord delta);

    std::vector<Part*>& GetActiveParts() { return m_activeParts; }
    const std::vector<Part*>& GetActiveParts() const { return m_activeParts; }
    std::size_t GetPartCount() const;
    bool ContainsPartType(PartType type) const;
    float GetCellSize() const { return m_cellSize; }

    void DrawSelf() const;
    std::optional<Math::GridCoord> WorldToCoord(Math::Vector2D v) const;
    Math::Vector2D CoordToWorld(Math::GridCoord coord) const;
    bool IsWithinBounds(Math::GridCoord coord) const;
};

#endif
