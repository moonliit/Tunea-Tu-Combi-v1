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
        .jsonPath = "assets/levels/level_01.json",
        .customMusicPath = "assets/audio/custom/Elsa.ogg",
    },
    LevelMenuEntry{
        .levelIndex = 2,
        .jsonPath = "assets/levels/level_02.json",
        .customMusicPath = "assets/audio/custom/Nunca Pense Llorar.ogg",
    },
    LevelMenuEntry{
        .levelIndex = 3,
        .jsonPath = "assets/levels/level_03.json",
        .customMusicPath = "assets/audio/custom/La Colegiala.ogg",
    },
    LevelMenuEntry{
        .levelIndex = 4,
        .jsonPath = "assets/levels/level_04.json",
        .customMusicPath = "assets/audio/custom/El Embrujo.ogg",
    },
    LevelMenuEntry{
        .levelIndex = 5,
        .jsonPath = "assets/levels/level_05.json",
        .customMusicPath = "assets/audio/custom/Soy Provinciano.ogg",
    },
    LevelMenuEntry{
        .levelIndex = 6,
        .jsonPath = "assets/levels/level_06.json",
        .customMusicPath = "assets/audio/custom/Lo Que Pasó, Pasó.ogg",
    },
    LevelMenuEntry{
        .levelIndex = 7,
        .jsonPath = "assets/levels/level_07.json",
        .customMusicPath = "assets/audio/custom/El Aguajal.ogg",
    },
    LevelMenuEntry{
        .levelIndex = 8,
        .jsonPath = "assets/levels/level_08.json",
        .customMusicPath = "assets/audio/custom/Kulikitaka.ogg",
    },
    LevelMenuEntry{
        .levelIndex = 9,
        .jsonPath = "assets/levels/level_09.json",
        .customMusicPath = "assets/audio/custom/El Aguajal.ogg",
    },
    LevelMenuEntry{
        .levelIndex = 10,
        .jsonPath = "assets/levels/level_10.json",
        .customMusicPath = "assets/audio/custom/Gasolina.ogg",
    },
    LevelMenuEntry{
        .levelIndex = 11,
        .jsonPath = "assets/levels/level_11.json",
        .customMusicPath = "assets/audio/custom/La Dueña del Swing.ogg",
    },
    LevelMenuEntry{
        .levelIndex = 12,
        .jsonPath = "assets/levels/level_12.json",
        .customMusicPath = "assets/audio/custom/CHOP SUEY.ogg",
    },
    LevelMenuEntry{
        .levelIndex = 13,
        .jsonPath = "assets/levels/level_13.json",
        .customMusicPath = "assets/audio/custom/DONT USE THIS SONG I BEG YOU.ogg",
    },
};

inline constexpr int OfficialLevelCount =
    static_cast<int>(OfficialLevels.size());

inline constexpr int MaxOfficialLevelCount = 20;

#endif
