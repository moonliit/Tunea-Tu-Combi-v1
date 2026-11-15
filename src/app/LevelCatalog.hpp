#ifndef APP_LEVEL_CATALOG_HPP
#define APP_LEVEL_CATALOG_HPP

#include <array>

struct LevelMenuEntry {
    int levelIndex;
    const char* jsonPath;
    const char* customMusicPath;
};

inline constexpr std::array OfficialLevels{
    LevelMenuEntry{
        .levelIndex = 1,
        .jsonPath = "assets/levels/level_1.json",
        .customMusicPath = "assets/audio/custom/N1 - La Colegiala.ogg",
    },
    LevelMenuEntry{
        .levelIndex = 2,
        .jsonPath = "assets/levels/level_2.json",
        .customMusicPath = "assets/audio/custom/N2 - Soy Provinciano.ogg",
    },
    LevelMenuEntry{
        .levelIndex = 3,
        .jsonPath = "assets/levels/level_3.json",
        .customMusicPath = "assets/audio/custom/N3 - El Aguajal.ogg",
    },
    LevelMenuEntry{
        .levelIndex = 4,
        .jsonPath = "assets/levels/level_4.json",
        .customMusicPath = "assets/audio/custom/N4 - A Patricia.ogg",
    }
};

inline constexpr int OfficialLevelCount =
    static_cast<int>(OfficialLevels.size());

inline constexpr int MaxOfficialLevelCount = 20;

#endif
