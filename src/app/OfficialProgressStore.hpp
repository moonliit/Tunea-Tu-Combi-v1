#ifndef APP_OFFICIAL_PROGRESS_STORE_HPP
#define APP_OFFICIAL_PROGRESS_STORE_HPP

#include <filesystem>

namespace OfficialProgressStore {

inline const std::filesystem::path DefaultPath = "user_data/official_progress.json";

int LoadUnlockedLevel(const std::filesystem::path& path = DefaultPath);
bool SaveUnlockedLevel(int unlockedLevel, const std::filesystem::path& path = DefaultPath);

}

#endif
