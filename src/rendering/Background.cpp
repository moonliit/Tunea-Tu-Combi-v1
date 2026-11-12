#include "rendering/Background.hpp"

#include <array>

namespace {

// same order as BackgroundId
constexpr std::array<const char*, BackgroundCount> Paths{
    "assets/textures/backgrounds/miraflores.png",
    "assets/textures/backgrounds/barranco.png",
    "assets/textures/backgrounds/chorrillos.png",
    "assets/textures/backgrounds/rimac.png"
};

}

const char* GetBackgroundPath(BackgroundId id) {
    return Paths[static_cast<std::size_t>(id)];
}

BackgroundId GetLevelBackground(int levelNumber) {
    switch (levelNumber) {
        case 2: return BackgroundId::Rimac;
        case 3: return BackgroundId::Chorrillos;
        case 4: return BackgroundId::Barranco;
        default: return BackgroundId::Miraflores;
    }
}
