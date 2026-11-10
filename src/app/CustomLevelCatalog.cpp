#include "app/CustomLevelCatalog.hpp"

#include <filesystem>
#include <fstream>
#include <utility>

CustomLevelCatalog::CustomLevelCatalog(std::string path): m_path(std::move(path)) {
    std::ifstream file(m_path);
    if (!file) return;
    try {
        const nlohmann::json data = nlohmann::json::parse(file);
        if (!data.is_array()) return;
        for (const auto& item : data) {
            CustomLevelData level;
            if (item.is_string()) {
                level.name = item.get<std::string>();
            } else if (item.is_object() && item.contains("name") && item["name"].is_string()) {
                level.name = item["name"].get<std::string>();
                if (item.contains("definition") && item["definition"].is_object())
                    level.definition = LevelDefinition::FromJson(item["definition"]);
                level.musicPath = item.value("musicPath", level.musicPath);
                level.musicTitle = item.value("musicTitle", level.musicTitle);
                level.musicArtist = item.value("musicArtist", level.musicArtist);
                level.hasTerrain = item.value("hasTerrain", false);
                level.hasGrid = item.value("hasGrid", false);
                level.hasGoal = item.value("hasGoal", false);
            }
            if (!level.name.empty()) m_levels.push_back(std::move(level));
        }
    } catch (const nlohmann::json::exception&) {}
}

void CustomLevelCatalog::Save() const {
    std::filesystem::create_directories(std::filesystem::path(m_path).parent_path());
    std::ofstream file(m_path);
    if (!file) return;
    nlohmann::json data = nlohmann::json::array();
    for (const auto& level : m_levels) data.push_back({
        {"name", level.name}, {"definition", level.definition.ToJson()},
        {"musicPath", level.musicPath}, {"musicTitle", level.musicTitle},
        {"musicArtist", level.musicArtist},
        {"hasTerrain", level.hasTerrain}, {"hasGrid", level.hasGrid},
        {"hasGoal", level.hasGoal}
    });
    file << data.dump(2);
}

void CustomLevelCatalog::Add(std::string name) {
    m_levels.push_back(CustomLevelData{.name = std::move(name)});
    Save();
}

void CustomLevelCatalog::Delete(std::size_t index) {
    if (index >= m_levels.size()) return;
    m_levels.erase(m_levels.begin() + static_cast<std::ptrdiff_t>(index));
    Save();
}
