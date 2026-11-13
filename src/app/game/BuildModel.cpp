#include "app/game/BuildModel.hpp"

#include <utility>

namespace {

const Part* FindPartAt(const EditorGrid& grid, Math::GridCoord coord) {
    for (const Part* part : grid.GetActiveParts()) {
        if (part->GetGridCoord().x == coord.x &&
            part->GetGridCoord().y == coord.y) {
            return part;
        }
    }
    return nullptr;
}

ConnectorDirection OppositeDirection(ConnectorDirection direction) {
    return static_cast<ConnectorDirection>(
        (static_cast<unsigned>(direction) + 2) % 4
    );
}

std::optional<PartOrientation> GetAttachmentOrientation(
    const EditorGrid& grid,
    Math::GridCoord coord,
    PartType type,
    PartOrientation fallback
) {
    if (type != PartType::Fan && type != PartType::WindSail) {
        return fallback;
    }

    struct Candidate {
        Math::GridCoord offset;
        PartOrientation orientation;
    };
    for (const Candidate candidate : {
             Candidate{{1, 0}, PartOrientation::Up},
             Candidate{{0, 1}, PartOrientation::Right},
             Candidate{{-1, 0}, PartOrientation::Down},
             Candidate{{0, -1}, PartOrientation::Left}
         }) {
        const Math::GridCoord neighborCoord{
            coord.x + candidate.offset.x,
            coord.y + candidate.offset.y
        };
        if (!grid.IsWithinBounds(neighborCoord)) continue;
        const auto& neighbor = grid.GetPart(neighborCoord);
        if (neighbor.has_value() &&
            neighbor->GetType() == PartType::SquareBlock) {
            return candidate.orientation;
        }
    }

    return fallback;
}

}

bool BuildModel::PlacePart(
    Math::GridCoord coord,
    PartType type,
    PartOrientation orientation
) {
    if (type == PartType::Driver) {
        if (m_grid.ContainsPartType(PartType::Driver)) return false;
        if (!m_grid.IsWithinBounds(coord)) return false;
        const auto& cell = m_grid.GetPart(coord);
        if (!cell.has_value() ||
            cell->GetType() != PartType::SquareBlock) {
            return false;
        }
    }
    const PartOrientation placementOrientation = GetAttachmentOrientation(
        m_grid,
        coord,
        type,
        orientation
    ).value();
    if (!m_grid.PlacePart(
            coord,
            type,
            placementOrientation,
            m_nextPartId
        )) return false;
    ++m_nextPartId;
    return true;
}

void BuildModel::RemovePart(Math::GridCoord coord) {
    m_grid.RemovePart(coord);
}

void BuildModel::Clear() {
    m_grid.Clear();
}

bool BuildModel::CanShiftContents(Math::GridCoord delta) const {
    return m_grid.CanShiftContents(delta);
}

bool BuildModel::ShiftContents(Math::GridCoord delta) {
    return m_grid.ShiftContents(delta);
}

std::vector<PartPlacement> BuildModel::GetPlacements() const {
    std::vector<PartPlacement> placements;
    placements.reserve(m_grid.GetActiveParts().size());

    for (const Part* part : m_grid.GetActiveParts()) {
        placements.push_back(PartPlacement{
            .id = part->GetId(),
            .type = part->GetType(),
            .coord = part->GetGridCoord(),
            .orientation = part->GetOrientation()
        });
    }

    return placements;
}

std::vector<PartConnection> BuildModel::GetConnections() const {
    std::vector<PartConnection> connections;
    const float halfCell = m_grid.GetCellSize() / 2.0f;

    for (const Part* first : m_grid.GetActiveParts()) {
        const ConnectorMask firstMask = RotateConnectorMask(
            GetPartDefinition(first->GetType()).connectors,
            first->GetOrientation()
        );

        for (const auto [direction, offset] : {
                 std::pair{
                     ConnectorDirection::Right,
                     Math::GridCoord{1, 0}
                 },
                 std::pair{
                     ConnectorDirection::Down,
                     Math::GridCoord{0, 1}
                 }
             }) {
            if ((firstMask & ConnectorBit(direction)) == 0) continue;

            const Math::GridCoord secondCoord{
                first->GetGridCoord().x + offset.x,
                first->GetGridCoord().y + offset.y
            };
            const Part* second = FindPartAt(m_grid, secondCoord);
            if (second == nullptr) continue;
            if (!ArePartsCompatible(first->GetType(), second->GetType())) {
                continue;
            }

            const ConnectorDirection secondDirection =
                OppositeDirection(direction);
            const ConnectorMask secondMask = RotateConnectorMask(
                GetPartDefinition(second->GetType()).connectors,
                second->GetOrientation()
            );
            if ((secondMask & ConnectorBit(secondDirection)) == 0) continue;

            Math::Vector2D anchor = m_grid.CoordToWorld(first->GetGridCoord());
            if (direction == ConnectorDirection::Right) {
                anchor.x += halfCell;
            } else {
                anchor.y += halfCell;
            }

            connections.push_back(PartConnection{
                .first = first->GetId(),
                .second = second->GetId(),
                .firstDirection = direction,
                .secondDirection = secondDirection,
                .anchor = anchor
            });
        }
    }

    return connections;
}
