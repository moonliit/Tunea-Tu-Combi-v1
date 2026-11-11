#ifndef RENDERING_BACKGROUND_HPP
#define RENDERING_BACKGROUND_HPP

#include <cstddef>

// one per Lima district
enum class BackgroundId : std::size_t {
    Miraflores,
    Barranco,
    Chorrillos,
    Rimac
};

inline constexpr std::size_t BackgroundCount = 4;

const char* GetBackgroundPath(BackgroundId id);

// fixed per official level, custom levels use Miraflores
BackgroundId GetLevelBackground(int levelNumber);

#endif
