#ifndef GAME_BUILD_MODEL_HPP
#define GAME_BUILD_MODEL_HPP

#include "game/EditorGrid.hpp"
#include "game/PartDefinition.hpp"

#include <vector>

struct PartPlacement {
    PartId id;
    PartType type;
    Math::GridCoord coord;
    PartOrientation orientation;
};

struct PartConnection {
    PartId first;
    PartId second;
    ConnectorDirection firstDirection;
    ConnectorDirection secondDirection;
    Math::Vector2D anchor;
};

class BuildModel {
private:
    EditorGrid m_grid;
    PartId m_nextPartId = 1;

public:
    BuildModel(Math::GridCoord gridDims, Math::Vector2D initialPos, float cellSize):
        m_grid(gridDims, initialPos, cellSize) {}

    EditorGrid& GetGrid() { return m_grid; }
    const EditorGrid& GetGrid() const { return m_grid; }

    bool PlacePart(
        Math::GridCoord coord,
        PartType type,
        PartOrientation orientation = PartOrientation::Up
    );
    void RemovePart(Math::GridCoord coord);
    void Clear();
    bool CanShiftContents(Math::GridCoord delta) const;
    bool ShiftContents(Math::GridCoord delta);
    std::vector<PartPlacement> GetPlacements() const;
    std::vector<PartConnection> GetConnections() const;

    const PartDefinition& GetDefinition(PartType type) const {
        return GetPartDefinition(type);
    }
};

#endif
