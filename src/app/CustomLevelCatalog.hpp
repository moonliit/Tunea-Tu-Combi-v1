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
    void Save();
    void Add(std::string name);
    bool AddCopy(const nlohmann::json& value);
    void Delete(std::size_t index);
    void MarkCompleted(std::size_t index, const nlohmann::json& capture);
    bool HasValidApproval(std::size_t index) const;
    nlohmann::json GetDraftSnapshot(std::size_t index) const;
    nlohmann::json GetPublicationPayload(std::size_t index) const;
};

#endif
