#ifndef APP_CUSTOM_LEVEL_DATA_HPP
#define APP_CUSTOM_LEVEL_DATA_HPP

#include "app/game/Level.hpp"

#include <string>

struct CustomLevelData {
    std::string name;
    LevelDefinition definition = LevelDefinition::Default();
    bool hasTerrain = false;
    bool hasGrid = false;
    bool hasGoal = false;
};

#endif
