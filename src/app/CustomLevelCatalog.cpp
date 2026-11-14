#include "app/CustomLevelCatalog.hpp"

#include <filesystem>
#include <fstream>
#include <array>
#include <random>
#include <utility>

namespace {

std::string CreateUuid() {
    std::array<unsigned char, 16> bytes;
    std::random_device random;
    for (unsigned char& byte : bytes) {
        byte = static_cast<unsigned char>(random());
    }
    bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0f) | 0x40);
    bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3f) | 0x80);
    constexpr char Hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(36);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        if (index == 4 || index == 6 || index == 8 || index == 10) {
            result.push_back('-');
        }
        result.push_back(Hex[bytes[index] >> 4]);
        result.push_back(Hex[bytes[index] & 0x0f]);
    }
    return result;
}

nlohmann::json DraftSnapshot(const CustomLevelData& level) {
    return {
        {"name", level.name},
        {"definition", level.definition.ToJson()},
        {"music", {
            {"path", level.musicPath},
            {"title", level.musicTitle},
            {"artist", level.musicArtist}
        }}
    };
}

bool ApprovalMatches(const CustomLevelData& level) {
    return level.approval.has_value() &&
        level.approval->is_object() &&
        level.approval->contains("snapshot") &&
        level.approval->at("snapshot") == DraftSnapshot(level);
}

}

CustomLevelCatalog::CustomLevelCatalog(std::string path): m_path(std::move(path)) {
    std::ifstream file(m_path);
    if (!file) return;
    try {
        const nlohmann::json data = nlohmann::json::parse(file);
        if (!data.is_array()) return;
        for (const auto& item : data) {
            CustomLevelData level;
            level.draftId = CreateUuid();
            if (item.is_string()) {
                level.name = item.get<std::string>();
            } else if (item.is_object() && item.contains("name") && item["name"].is_string()) {
                level.draftId = item.value("draftId", level.draftId);
                level.name = item["name"].get<std::string>();
                if (item.contains("definition") && item["definition"].is_object())
                    level.definition = LevelDefinition::FromJson(item["definition"]);
                level.musicPath = item.value("musicPath", level.musicPath);
                level.musicTitle = item.value("musicTitle", level.musicTitle);
                level.musicArtist = item.value("musicArtist", level.musicArtist);
                level.hasTerrain = item.value("hasTerrain", false);
                level.hasGrid = item.value("hasGrid", false);
                level.hasGoal = item.value("hasGoal", false);
                if (item.contains("sourcePublicationId") && item["sourcePublicationId"].is_string())
                    level.sourcePublicationId = item["sourcePublicationId"].get<std::string>();
                if (item.contains("approval") && item["approval"].is_object())
                    level.approval = item["approval"];
            }
            if (!ApprovalMatches(level)) level.approval.reset();
            if (!level.name.empty()) m_levels.push_back(std::move(level));
        }
    } catch (const nlohmann::json::exception&) {}
}

void CustomLevelCatalog::Save() {
    for (CustomLevelData& level : m_levels) {
        if (!ApprovalMatches(level)) level.approval.reset();
    }
    std::filesystem::create_directories(std::filesystem::path(m_path).parent_path());
    std::ofstream file(m_path);
    if (!file) return;
    nlohmann::json data = nlohmann::json::array();
    for (const auto& level : m_levels) {
        nlohmann::json item = {
            {"draftId", level.draftId},
            {"name", level.name}, {"definition", level.definition.ToJson()},
            {"musicPath", level.musicPath}, {"musicTitle", level.musicTitle},
            {"musicArtist", level.musicArtist},
            {"hasTerrain", level.hasTerrain}, {"hasGrid", level.hasGrid},
            {"hasGoal", level.hasGoal}
        };
        if (level.sourcePublicationId.has_value())
            item["sourcePublicationId"] = level.sourcePublicationId.value();
        if (level.approval.has_value()) item["approval"] = level.approval.value();
        data.push_back(std::move(item));
    }
    file << data.dump(2);
}

void CustomLevelCatalog::Add(std::string name) {
    m_levels.push_back(CustomLevelData{
        .draftId = CreateUuid(),
        .name = std::move(name)
    });
    Save();
}

bool CustomLevelCatalog::AddCopy(const nlohmann::json& value) {
    try {
        if (!value.is_object() || !value.contains("definition") ||
            !value.contains("music")) return false;
        CustomLevelData level;
        level.draftId = CreateUuid();
        level.name = value.at("name").get<std::string>();
        level.definition = LevelDefinition::FromJson(value.at("definition"));
        level.definition.publicId.reset();
        level.musicPath = value.at("music").at("path").get<std::string>();
        level.musicTitle = value.at("music").at("title").get<std::string>();
        level.musicArtist = value.at("music").at("artist").get<std::string>();
        level.hasTerrain = !level.definition.terrain.polygons.empty();
        level.hasGrid = true;
        level.hasGoal = true;
        level.sourcePublicationId = value.at("sourcePublicationId").get<std::string>();
        m_levels.push_back(std::move(level));
        Save();
        return true;
    } catch (const nlohmann::json::exception&) {
        return false;
    }
}

void CustomLevelCatalog::Delete(std::size_t index) {
    if (index >= m_levels.size()) return;
    m_levels.erase(m_levels.begin() + static_cast<std::ptrdiff_t>(index));
    Save();
}

void CustomLevelCatalog::MarkCompleted(
    std::size_t index,
    const nlohmann::json& capture
) {
    if (index >= m_levels.size()) return;
    CustomLevelData& level = m_levels[index];
    level.approval = {
        {"publicationRequestId", CreateUuid()},
        {"snapshot", DraftSnapshot(level)},
        {"capture", capture}
    };
    Save();
}

bool CustomLevelCatalog::HasValidApproval(std::size_t index) const {
    return index < m_levels.size() && ApprovalMatches(m_levels[index]);
}

nlohmann::json CustomLevelCatalog::GetDraftSnapshot(std::size_t index) const {
    if (index >= m_levels.size()) return nullptr;
    return DraftSnapshot(m_levels[index]);
}

nlohmann::json CustomLevelCatalog::GetPublicationPayload(std::size_t index) const {
    if (!HasValidApproval(index)) return nullptr;
    const CustomLevelData& level = m_levels[index];
    const nlohmann::json snapshot = DraftSnapshot(level);
    return {
        {"publicationRequestId", level.approval->at("publicationRequestId")},
        {"draftId", level.draftId},
        {"name", snapshot.at("name")},
        {"definition", snapshot.at("definition")},
        {"music", snapshot.at("music")},
        {"capture", level.approval->at("capture")}
    };
}
