#include "app/game/EditorGrid.hpp"
#include "app/game/PartDefinition.hpp"
#include "math/GridCoord.hpp"
#include "math/Vector2D.hpp"
#include "raylib.h"
#include <cmath>

using namespace Math;

namespace {

std::size_t CountPartTree(const Part& part) {
    return 1 + (
        part.HasContainedPart()
            ? CountPartTree(*part.GetContainedPart())
            : 0
    );
}

}

EditorGrid::EditorGrid(GridCoord gridDims, Vector2D initialPos, float cellSize):
    m_gridDims(gridDims), m_initialPos(initialPos), m_cellSize(cellSize)
{
    const int mult = gridDims.x * gridDims.y;
    m_cells.resize(mult);
}

EditorGrid::EditorGrid(const EditorGrid& other):
    m_gridDims(other.m_gridDims),
    m_initialPos(other.m_initialPos),
    m_cellSize(other.m_cellSize),
    m_cells(other.m_cells)
{
}

EditorGrid& EditorGrid::operator=(const EditorGrid& other) {
    if (this == &other) return *this;
    m_gridDims = other.m_gridDims;
    m_initialPos = other.m_initialPos;
    m_cellSize = other.m_cellSize;
    m_cells = other.m_cells;
    return *this;
}

std::size_t EditorGrid::GetPartCount() const {
    std::size_t count = 0;
    for (const std::optional<Part>& cell : m_cells) {
        if (!cell.has_value()) continue;
        count += CountPartTree(cell.value());
    }
    return count;
}

bool EditorGrid::ContainsPartType(PartType type) const {
    for (const std::optional<Part>& cell : m_cells) {
        if (!cell.has_value()) continue;
        const Part& part = cell.value();
        if (part.GetType() == type ||
            (part.HasContainedPart() &&
             part.GetContainedPart()->GetType() == type)) {
            return true;
        }
    }
    return false;
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

bool EditorGrid::IsWithinBounds(Math::GridCoord coord) const {
    return IsGridCoordWithinSelfBounds(coord);
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

std::vector<Part*> EditorGrid::CollectActiveParts() {
    std::vector<Part*> parts;
    for (std::optional<Part>& cell : m_cells) {
        if (!cell.has_value()) continue;
        parts.push_back(&cell.value());
    }
    return parts;
}

std::vector<const Part*> EditorGrid::CollectActiveParts() const {
    std::vector<const Part*> parts;
    for (const std::optional<Part>& cell : m_cells) {
        if (!cell.has_value()) continue;
        parts.push_back(&cell.value());
    }
    return parts;
}

std::optional<Part>& EditorGrid::GetPart(GridCoord coord) {
    return m_cells[GetIndex(coord)];
}

const std::optional<Part>& EditorGrid::GetPart(GridCoord coord) const {
    size_t idx = GetIndex(coord);
    return m_cells[idx];
}

bool EditorGrid::PlacePart(
    GridCoord coord,
    PartType type,
    PartOrientation orientation,
    PartId id
) {
    if (!IsGridCoordWithinSelfBounds(coord)) return false;
    size_t idx = GetIndex(coord);

    if (!m_cells[idx].has_value()) {
        m_cells[idx] = Part(coord, type, orientation, id);
        return true;
    }

    if (GetPartDefinition(type).role == PartRole::Containable) {
        Part candidate(coord, type, orientation, id);
        if (m_cells[idx]->CanContain(candidate)) {
            return m_cells[idx]->ContainPart(
                std::make_unique<Part>(std::move(candidate))
            );
        }
        if (GetPartDefinition(m_cells[idx]->m_type).role ==
                PartRole::Container &&
            m_cells[idx]->CanContain(candidate)) {
            m_cells[idx]->RemoveContainedPart();
            return m_cells[idx]->ContainPart(
                std::make_unique<Part>(std::move(candidate))
            );
        }
    } else if (GetPartDefinition(type).role == PartRole::Container) {
        Part candidate(coord, type, orientation, id);
        Part& existingPart = m_cells[idx].value();
        if (GetPartDefinition(existingPart.m_type).role == PartRole::Containable) {
            candidate.ContainPart(
                std::make_unique<Part>(std::move(existingPart))
            );
            m_cells[idx] = std::move(candidate);
            return true;
        }
        if (GetPartDefinition(existingPart.m_type).role == PartRole::Container) {
            candidate.ContainPart(
                std::make_unique<Part>(std::move(existingPart))
            );
            m_cells[idx] = std::move(candidate);
            return true;
        }
    }

    RemovePart(coord);

    m_cells[idx] = Part(coord, type, orientation, id);
    return true;
}

void EditorGrid::RemovePart(GridCoord coord) {
    if (!IsGridCoordWithinSelfBounds(coord)) return;
    size_t idx = GetIndex(coord);

    if (!m_cells[idx].has_value()) return;

    m_cells[idx].reset();
}

void EditorGrid::RotatePart(GridCoord coord) {
    if (!IsGridCoordWithinSelfBounds(coord)) return;
    std::optional<Part>& part = m_cells[GetIndex(coord)];
    if (part.has_value()) {
        // rotate the drivers box + the contained driver
        if (part->GetType() == PartType::DriverBox) {
            part->m_orientation = RotatePartOrientation(part->m_orientation);
            if (part->HasContainedPart()) {
                Part* child = part->GetContainedPart();
                child->m_orientation = RotatePartOrientation(child->m_orientation);
            }
        } else if (part->HasContainedPart()) {
            Part* child = part->GetContainedPart();
            child->m_orientation = RotatePartOrientation(child->m_orientation);
        } else {
            part->m_orientation = RotatePartOrientation(part->m_orientation);
        }
    }
}

void EditorGrid::Clear() {
    for (std::optional<Part>& cell : m_cells) {
        cell.reset();
    }
}

bool EditorGrid::CanShiftContents(Math::GridCoord delta) const {
    for (const std::optional<Part>& cell : m_cells) {
        if (!cell.has_value()) continue;
        const Part* part = &cell.value();
        const GridCoord shifted{
            part->GetGridCoord().x + delta.x,
            part->GetGridCoord().y + delta.y
        };
        if (!IsGridCoordWithinSelfBounds(shifted)) return false;
    }
    return true;
}

bool EditorGrid::ShiftContents(Math::GridCoord delta) {
    if (!CanShiftContents(delta)) return false;
    if (delta.x == 0 && delta.y == 0) return true;

    std::vector<std::optional<Part>> shiftedCells;
    shiftedCells.resize(m_cells.size());
    for (std::optional<Part>& cell : m_cells) {
        if (!cell.has_value()) continue;
        const GridCoord oldCoord = cell->GetGridCoord();
        const GridCoord newCoord{
            oldCoord.x + delta.x,
            oldCoord.y + delta.y
        };
        const size_t newIndex = GetIndex(newCoord);
        shiftedCells[newIndex] = std::move(cell);
        shiftedCells[newIndex]->m_gridCoord = newCoord;
        if (shiftedCells[newIndex]->m_containedPart) {
            shiftedCells[newIndex]->m_containedPart->m_gridCoord = newCoord;
        }
    }

    m_cells = std::move(shiftedCells);
    return true;
}

void EditorGrid::DrawSelf() const {
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
                Fade(LIGHTGRAY, 0.5f)
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
