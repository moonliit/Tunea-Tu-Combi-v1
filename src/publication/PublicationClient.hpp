#ifndef PUBLICATION_CLIENT_HPP
#define PUBLICATION_CLIENT_HPP

#include <json.hpp>
#include <cstdint>
#include <optional>
#include <string>

enum class PublicationStatus {
    Idle,
    Publishing,
    Published,
    Error,
    Unavailable
};

struct PublicationSnapshot {
    PublicationStatus status = PublicationStatus::Idle;
    std::string message;
};

class PublicationClient {
    PublicationSnapshot m_snapshot;
    std::optional<nlohmann::json> m_playRequest;
    std::optional<nlohmann::json> m_copyRequest;
    std::uint64_t m_revision = 0;
    double m_pollDelayMs = 0.0;

public:
    void Publish(const nlohmann::json& payload);
    void Poll(double frameMs);
    std::optional<nlohmann::json> TakePlayRequest();
    std::optional<nlohmann::json> TakeCopyRequest();
    const PublicationSnapshot& GetSnapshot() const { return m_snapshot; }
};

#endif
