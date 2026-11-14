#include "publication/PublicationClient.hpp"

#include <cstdlib>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
EM_JS(void, PublishDraftWeb, (const char* json), {
    Module['api'].publishDraft(JSON.parse(UTF8ToString(json)));
});
EM_JS(char*, ReadPublicationWeb, (), {
    return stringToNewUTF8(JSON.stringify(Module['api'].publicationSnapshot()));
});
EM_JS(void, ConsumePublicationActionWeb, (const char* action), {
    Module['api'].consumePublicationAction(UTF8ToString(action));
});
EM_JS_DEPS(publication_deps, "$stringToNewUTF8");
#endif

void PublicationClient::Publish(const nlohmann::json& payload) {
    if (!payload.is_object()) {
        m_snapshot = {
            .status = PublicationStatus::Error,
            .message = "Completa esta versión del nivel antes de publicarla."
        };
        return;
    }
#ifdef __EMSCRIPTEN__
    const std::string serialized = payload.dump();
    PublishDraftWeb(serialized.c_str());
    m_snapshot = {
        .status = PublicationStatus::Publishing,
        .message = "Publicando..."
    };
#else
    m_snapshot = {
        .status = PublicationStatus::Unavailable,
        .message = "La publicación solo está disponible en la versión web."
    };
#endif
}

void PublicationClient::Poll(double frameMs) {
#ifdef __EMSCRIPTEN__
    m_pollDelayMs -= frameMs;
    if (m_pollDelayMs > 0.0) return;
    m_pollDelayMs = 250.0;
    char* raw = ReadPublicationWeb();
    const nlohmann::json state = nlohmann::json::parse(raw, nullptr, false);
    std::free(raw);
    if (!state.is_object()) return;
    const std::string status = state.value("status", "idle");
    m_snapshot.status = status == "publishing"
        ? PublicationStatus::Publishing
        : status == "published"
        ? PublicationStatus::Published
        : status == "error"
        ? PublicationStatus::Error
        : PublicationStatus::Idle;
    m_snapshot.message = state.value("message", "");
    const std::uint64_t revision = state.value("revision", std::uint64_t{0});
    if (revision == m_revision) return;
    m_revision = revision;
    if (state.contains("play") && state["play"].is_object()) {
        m_playRequest = state["play"];
        ConsumePublicationActionWeb("play");
    }
    if (state.contains("copy") && state["copy"].is_object()) {
        m_copyRequest = state["copy"];
        ConsumePublicationActionWeb("copy");
    }
#else
    static_cast<void>(frameMs);
#endif
}

std::optional<nlohmann::json> PublicationClient::TakePlayRequest() {
    std::optional<nlohmann::json> value = std::move(m_playRequest);
    m_playRequest.reset();
    return value;
}

std::optional<nlohmann::json> PublicationClient::TakeCopyRequest() {
    std::optional<nlohmann::json> value = std::move(m_copyRequest);
    m_copyRequest.reset();
    return value;
}
