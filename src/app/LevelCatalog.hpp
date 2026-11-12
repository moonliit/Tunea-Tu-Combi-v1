#ifndef APP_LEVEL_CATALOG_HPP
#define APP_LEVEL_CATALOG_HPP

#include <array>

struct LevelMenuEntry {
    const char* path;
    const char* label;
    const char* customMusicPath;
};

inline constexpr std::array OfficialLevels{
    LevelMenuEntry{"assets/levels/level_1.json", "Nivel 1", "assets/audio/custom/N1 - La Colegiala.ogg"},
    LevelMenuEntry{"assets/levels/level_2.json", "Nivel 2", "assets/audio/custom/N2 - Soy Provinciano.ogg"},
    LevelMenuEntry{"assets/levels/level_3.json", "Nivel 3", "assets/audio/custom/N3 - El Aguajal.ogg"},
    LevelMenuEntry{"assets/levels/level_4.json", "Nivel 4", "assets/audio/custom/N4 - A Patricia.ogg"}
};

inline constexpr int OfficialLevelCount =
    static_cast<int>(OfficialLevels.size());

#endif
