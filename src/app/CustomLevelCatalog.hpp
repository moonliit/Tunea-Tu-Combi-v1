#ifndef APP_CUSTOM_LEVEL_CATALOG_HPP
#define APP_CUSTOM_LEVEL_CATALOG_HPP

#include "app/CustomLevelData.hpp"

#include <string>
#include <vector>

class CustomLevelCatalog {
    std::string m_path;
    std::vector<CustomLevelData> m_levels;

public:
    explicit CustomLevelCatalog(std::string path = "user_data/custom_levels.json");
    const std::vector<CustomLevelData>& GetLevels() const { return m_levels; }
    std::vector<CustomLevelData>& GetLevels() { return m_levels; }
    void Save() const;
    void Add(std::string name);
    void Delete(std::size_t index);
};

#endif
