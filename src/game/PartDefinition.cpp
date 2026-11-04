#include "game/PartDefinition.hpp"

#include <stdexcept>

namespace {

const PartDefinition SQUARE_BLOCK{
    PartType::SquareBlock,
    "Square block",
    1.0f,
    PartShape::Box,
    PartBehavior::Passive,
    0.5f,
    0.0f,
    ConnectorBit(ConnectorDirection::Up) |
        ConnectorBit(ConnectorDirection::Right) |
        ConnectorBit(ConnectorDirection::Down) |
        ConnectorBit(ConnectorDirection::Left)
};
const PartDefinition CIRCLE_WHEEL{
    PartType::CircleWheel,
    "Circle wheel",
    1.0f,
    PartShape::Circle,
    PartBehavior::Passive,
    0.32f,
    0.18f,
    ConnectorBit(ConnectorDirection::Up)
};
const PartDefinition TNT_BARREL{
    PartType::TNTBarrel,
    "TNT barrel",
    1.0f,
    PartShape::Box,
    PartBehavior::Explosive,
    0.5f,
    0.0f,
    ConnectorBit(ConnectorDirection::Up) |
        ConnectorBit(ConnectorDirection::Right) |
        ConnectorBit(ConnectorDirection::Down) |
        ConnectorBit(ConnectorDirection::Left)
};
const PartDefinition FAN{
    PartType::Fan,
    "Fan",
    1.0f,
    PartShape::Capsule,
    PartBehavior::Passive,
    0.5f,
    0.0f,
    ConnectorBit(ConnectorDirection::Right)
};
const PartDefinition WIND_SAIL{
    PartType::WindSail,
    "Wind sail",
    1.0f,
    PartShape::Triangle,
    PartBehavior::Passive,
    0.5f,
    0.0f,
    ConnectorBit(ConnectorDirection::Right)
};

}

const PartDefinition& GetPartDefinition(PartType type) {
    switch (type) {
        case PartType::SquareBlock: return SQUARE_BLOCK;
        case PartType::CircleWheel: return CIRCLE_WHEEL;
        case PartType::TNTBarrel: return TNT_BARREL;
        case PartType::Fan: return FAN;
        case PartType::WindSail: return WIND_SAIL;
    }

    throw std::invalid_argument("Unknown part type");
}

bool ArePartsCompatible(PartType first, PartType second) {
    if (first == PartType::CircleWheel &&
        second == PartType::CircleWheel) {
        return false;
    }

    return true;
}
