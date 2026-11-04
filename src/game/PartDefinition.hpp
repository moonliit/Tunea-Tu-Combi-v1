#ifndef GAME_PART_DEFINITION_HPP
#define GAME_PART_DEFINITION_HPP

#include "game/Part.hpp"

#include <string_view>
#include <cstdint>

enum class PartBehavior {
    Passive,
    Explosive
};

enum class ConnectorDirection : std::uint8_t {
    Up,
    Right,
    Down,
    Left
};

using ConnectorMask = std::uint8_t;

constexpr ConnectorMask ConnectorBit(ConnectorDirection direction) {
    return static_cast<ConnectorMask>(1u << static_cast<unsigned>(direction));
}

constexpr ConnectorMask RotateConnectorMask(
    ConnectorMask mask,
    PartOrientation orientation
) {
    ConnectorMask rotated = 0;
    for (unsigned index = 0; index < 4; ++index) {
        const auto direction = static_cast<ConnectorDirection>(index);
        if ((mask & ConnectorBit(direction)) == 0) continue;

        const auto rotatedIndex =
            (index + static_cast<unsigned>(orientation)) % 4;
        rotated |= ConnectorBit(
            static_cast<ConnectorDirection>(rotatedIndex)
        );
    }
    return rotated;
}

struct PartDefinition {
    PartType type;
    std::string_view name;
    float mass;
    PartShape shape;
    PartBehavior behavior = PartBehavior::Passive;
    float radiusScale = 0.5f;
    float attachmentOffsetScale = 0.0f;
    ConnectorMask connectors = 0;
};

const PartDefinition& GetPartDefinition(PartType type);
bool ArePartsCompatible(PartType first, PartType second);

#endif
