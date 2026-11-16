#ifndef RENDERING_BACKGROUND_HPP
#define RENDERING_BACKGROUND_HPP

#include <cstddef>

// one per Lima district
enum class BackgroundId : std::size_t {
    Barranco,
    Barriadas,
    CentroDeLima,
    Chorrillos,
    Miraflores,
    Rimac,
    SanJuanDeLurigancho,
    SanJuanDeMiraflores,
    SantiagoDeSurco,
    Surquillo,
};

inline constexpr std::size_t BackgroundCount = 10;

// how fast the background scrolls compared to the world
inline constexpr float BackgroundParallaxFactor = 0.1f;

const char* GetBackgroundPath(BackgroundId id);

// fixed per official level, custom levels use Miraflores
BackgroundId GetLevelBackground(int levelNumber);

#endif
