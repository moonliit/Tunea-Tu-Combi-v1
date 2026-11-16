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
EM_JS(void, OpenCatalogWeb, (), { Module['api'].openCatalog(); });
EM_JS(void, ShowOnlyMineWeb, (bool mine), { Module['api'].showOnlyMine(mine); });
EM_JS(void, SearchCatalogWeb, (const char* query), {
    Module['api'].searchCatalog(UTF8ToString(query));
});
EM_JS(void, NextCatalogPageWeb, (), { Module['api'].nextCatalogPage(); });
EM_JS(void, PreviousCatalogPageWeb, (), { Module['api'].previousCatalogPage(); });
EM_JS(void, PlayPublicLevelWeb, (const char* id), {
    Module['api'].playPublicLevel(UTF8ToString(id));
});
EM_JS(void, CopyPublicLevelWeb, (const char* id), {
    Module['api'].copyPublication(UTF8ToString(id));
});
EM_JS(void, RetirePublicLevelWeb, (const char* id), {
    Module['api'].retirePublication(UTF8ToString(id));
});
EM_JS(char*, ReadCatalogWeb, (), {
    return stringToNewUTF8(JSON.stringify(Module['api'].catalogSnapshot()));
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

void PublicationClient::OpenCatalog() {
#ifdef __EMSCRIPTEN__
    m_catalog.loading = true;
    m_catalog.error.clear();
    OpenCatalogWeb();
#else
    m_catalog = {
        .error = "Los niveles públicos están disponibles en la versión web."
    };
#endif
}

void PublicationClient::ShowOnlyMine(bool mine) {
#ifdef __EMSCRIPTEN__
    m_catalog.onlyMine = mine;
    m_catalog.loading = true;
    m_catalog.error.clear();
    ShowOnlyMineWeb(mine);
#else
    static_cast<void>(mine);
#endif
}

void PublicationClient::SearchCatalog(const std::string& query) {
#ifdef __EMSCRIPTEN__
    m_catalog.loading = true;
    m_catalog.error.clear();
    SearchCatalogWeb(query.c_str());
#else
    static_cast<void>(query);
    m_catalog = {
        .error = "Los niveles públicos están disponibles en la versión web."
    };
#endif
}

void PublicationClient::NextCatalogPage() {
#ifdef __EMSCRIPTEN__
    if (m_catalog.loading || !m_catalog.hasNext) return;
    m_catalog.loading = true;
    NextCatalogPageWeb();
#endif
}

void PublicationClient::PreviousCatalogPage() {
#ifdef __EMSCRIPTEN__
    if (m_catalog.loading || !m_catalog.hasPrevious) return;
    m_catalog.loading = true;
    PreviousCatalogPageWeb();
#endif
}

void PublicationClient::PlayPublicLevel(const std::string& id) {
#ifdef __EMSCRIPTEN__
    PlayPublicLevelWeb(id.c_str());
#else
    static_cast<void>(id);
#endif
}

void PublicationClient::CopyPublicLevel(const std::string& id) {
#ifdef __EMSCRIPTEN__
    CopyPublicLevelWeb(id.c_str());
#else
    static_cast<void>(id);
#endif
}

void PublicationClient::RetirePublicLevel(const std::string& id) {
#ifdef __EMSCRIPTEN__
    RetirePublicLevelWeb(id.c_str());
#else
    static_cast<void>(id);
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
    char* catalogRaw = ReadCatalogWeb();
    const nlohmann::json catalog = nlohmann::json::parse(
        catalogRaw, nullptr, false
    );
    std::free(catalogRaw);
    if (catalog.is_object()) {
        m_catalog.loading = catalog.value("loading", false);
        m_catalog.hasPrevious = catalog.value("hasPrevious", false);
        m_catalog.hasNext = catalog.value("hasNext", false);
        m_catalog.onlyMine = catalog.value("onlyMine", false);
        m_catalog.error = catalog.value("error", "");
        m_catalog.levels.clear();
        if (catalog.contains("items") && catalog["items"].is_array()) {
            for (const auto& item : catalog["items"]) {
                if (!item.is_object()) continue;
                m_catalog.levels.push_back({
                    item.value("id", ""),
                    item.value("name", ""),
                    item.value("author", ""),
                    item.value("mine", false)
                });
            }
        }
    }
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
