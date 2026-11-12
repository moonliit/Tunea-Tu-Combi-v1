#pragma once
#include <cstdint>
#include <optional>
#include <string>

// keeps the identity that started the run
struct IdentityScope {
    std::string combiId;
    std::uint64_t epoch = 0;
};

class IdentitySession {
    bool m_authenticated = false;
    std::string m_id;
    std::uint64_t m_epoch = 0;
public:
    void Poll();
    bool IsAuthenticated() const { return m_authenticated; }
    IdentityScope CaptureResultScope() const { return {m_authenticated ? m_id : "", m_epoch}; }
    std::optional<std::string> ResultOwner(const IdentityScope& scope) const {
        if (!m_authenticated || scope.combiId.empty() || scope.combiId != m_id || scope.epoch != m_epoch)
            return std::nullopt;
        return scope.combiId;
    }
};
