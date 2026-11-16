#include "rendering/Background.hpp"

#include <array>

namespace {

// same order as BackgroundId
constexpr std::array<const char*, BackgroundCount> Paths{
    "assets/textures/backgrounds/barranco.png",
    "assets/textures/backgrounds/barriadas.png",
    "assets/textures/backgrounds/centro_de_lima.png",
    "assets/textures/backgrounds/chorrillos.png",
    "assets/textures/backgrounds/miraflores.png",
    "assets/textures/backgrounds/rimac.png",
    "assets/textures/backgrounds/san_juan_de_lurigancho.png",
    "assets/textures/backgrounds/san_juan_de_miraflores.png",
    "assets/textures/backgrounds/santiago_de_surco.png",
    "assets/textures/backgrounds/surquillo.png",
};

}

const char* GetBackgroundPath(BackgroundId id) {
    return Paths[static_cast<std::size_t>(id)];
}

BackgroundId GetLevelBackground(int levelNumber) {
    switch (levelNumber) {
        case 1: return BackgroundId::Miraflores;
        case 2: return BackgroundId::Rimac;
        case 3: return BackgroundId::Chorrillos;
        case 4: return BackgroundId::Barranco;
        case 5: return BackgroundId::SanJuanDeMiraflores;
        case 6: return BackgroundId::Surquillo;
        case 7: return BackgroundId::Barriadas;
        case 8: return BackgroundId::CentroDeLima;
        case 9: return BackgroundId::SanJuanDeLurigancho;
        case 10: return BackgroundId::SantiagoDeSurco;
        case 11: return BackgroundId::Miraflores;
        case 12: return BackgroundId::Rimac;
        case 13: return BackgroundId::Chorrillos;
        default: return BackgroundId::Miraflores;
    }
}
