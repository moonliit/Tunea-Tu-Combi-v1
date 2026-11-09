#ifndef APP_LEVEL_CATALOG_HPP
#define APP_LEVEL_CATALOG_HPP

#include <array>

struct LevelMenuEntry {
    const char* path;
    const char* label;
    const char* musicPath;
};

inline constexpr std::array OfficialLevels{
    LevelMenuEntry{"assets/levels/level_1.json", "Nivel 1", "assets/audio/level_1.wav"},
    LevelMenuEntry{"assets/levels/level_2.json", "Nivel 2", "assets/audio/level_2.wav"},
    LevelMenuEntry{"assets/levels/level_3.json", "Nivel 3", "assets/audio/level_3.wav"}
};

inline constexpr int OfficialLevelCount =
    static_cast<int>(OfficialLevels.size());

#endif
