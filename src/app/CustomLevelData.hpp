#ifndef APP_CUSTOM_LEVEL_DATA_HPP
#define APP_CUSTOM_LEVEL_DATA_HPP

#include "app/game/Level.hpp"

#include <json.hpp>
#include <optional>
#include <string>

struct CustomLevelData {
    std::string draftId;
    std::string name;
    LevelDefinition definition = LevelDefinition::Default();
    std::string musicPath = "assets/audio/custom/N2 - Soy Provinciano.ogg";
    std::string musicTitle = "Soy Provinciano";
    std::string musicArtist = "Chacalón";
    bool hasTerrain = false;
    bool hasGrid = false;
    bool hasGoal = false;
    std::optional<std::string> sourcePublicationId;
    std::optional<nlohmann::json> approval;
};

#endif
