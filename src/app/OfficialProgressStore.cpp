#include "app/OfficialProgressStore.hpp"

#include "app/LevelCatalog.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <json.hpp>

int OfficialProgressStore::LoadUnlockedLevel(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) return 1;

    const auto data = nlohmann::json::parse(file, nullptr, false);
    if (!data.is_object()) return 1;
    const auto level = data.find("unlockedLevel");
    if (level == data.end() || !level->is_number_integer()) return 1;

    const auto value = level->get<std::int64_t>();
    return static_cast<int>(std::clamp<std::int64_t>(value, 1, OfficialLevelCount));
}

bool OfficialProgressStore::SaveUnlockedLevel(
    int unlockedLevel,
    const std::filesystem::path& path
) {
    const int level = std::max(
        std::clamp(unlockedLevel, 1, OfficialLevelCount),
        LoadUnlockedLevel(path)
    );

    std::error_code error;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) return false;
    }

    std::filesystem::path temporary = path;
    temporary += ".tmp";
    {
        std::ofstream file(temporary, std::ios::trunc);
        if (!file) return false;
        file << nlohmann::json{{"unlockedLevel", level}}.dump(2) << '\n';
        file.flush();
        if (!file) return false;
    }

    std::filesystem::rename(temporary, path, error);
    if (!error) return true;

    // SOME native filesystems cannot rename over an existing file
    error.clear();
    const bool copied = std::filesystem::copy_file(
        temporary,
        path,
        std::filesystem::copy_options::overwrite_existing,
        error
    );
    if (copied) std::filesystem::remove(temporary, error);
    return copied;
}
