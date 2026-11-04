#include "game/BuildModel.hpp"

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

}

bool BuildModel::PlacePart(
    Math::GridCoord coord,
    PartType type,
    PartOrientation orientation
) {
    if (!m_grid.PlacePart(coord, type, orientation, m_nextPartId)) return false;
    ++m_nextPartId;
    return true;
}

void BuildModel::RemovePart(Math::GridCoord coord) {
    m_grid.RemovePart(coord);
}

void BuildModel::Clear() {
    m_grid.Clear();
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
