#include "app/game/GhostPlayback.hpp"

#include <algorithm>
#include <cmath>

#ifdef __EMSCRIPTEN__
#include <cstdlib>
#include <emscripten.h>
EM_JS(void, PrepareChampionGhostWeb, (const char* level), {
    Module['api'].prepareChampionGhost(UTF8ToString(level));
});
EM_JS(void, LoadPreparedChampionGhostWeb, (), {
    Module['api'].loadPreparedChampionGhost();
});
EM_JS(char*, ReadChampionGhostWeb, (), {
    return stringToNewUTF8(JSON.stringify(Module['api'].ghostSnapshot()));
});
EM_JS_DEPS(ghost_playback_deps, "$stringToNewUTF8");
#endif

namespace {
constexpr double Pi = 3.14159265358979323846;

double InterpolateAngle(double first, double second, double amount) {
    double difference = std::remainder(second - first, 2.0 * Pi);
    return first + difference * amount;
}
}

void GhostPlayback::Request(const std::string& levelId, float cellSize) {
    Reset();
    m_levelId = levelId;
    m_cellSize = cellSize;
    if (levelId.empty()) {
        m_status = GhostLoadStatus::Unavailable;
        return;
    }
    m_requested = true;
    m_status = GhostLoadStatus::Loading;
#ifdef __EMSCRIPTEN__
    PrepareChampionGhostWeb(levelId.c_str());
#else
    m_requested = false;
    m_status = GhostLoadStatus::Unavailable;
#endif
}

bool GhostPlayback::Load(const nlohmann::json& capture) {
    if (
        !capture.is_object() || capture.value("levelId", "") != m_levelId ||
        capture.value("formatVersion", "") != "run-v2" ||
        capture.value("physicsVersion", "") != "custom-physics-v1" ||
        capture.value("rulesVersion", "") != "score-v2"
    ) return false;
    try {
        for (const auto& part : capture.at("construction")) {
            m_definitions.emplace(part.at("id").get<PartId>(), Definition{
                .type = static_cast<PartType>(part.at("type").get<int>()),
                .orientation = static_cast<PartOrientation>(part.at("orientation").get<int>())
            });
        }
        for (const auto& connection : capture.at("connections")) {
            const PartId first = connection.at("first").get<PartId>();
            const PartId second = connection.at("second").get<PartId>();
            if (m_definitions.at(first).type == PartType::CircleWheel) {
                m_legacyWheelHosts.try_emplace(first, second);
            } else if (m_definitions.at(second).type == PartType::CircleWheel) {
                m_legacyWheelHosts.try_emplace(second, first);
            }
        }
        for (const auto& passenger : capture.at("levelDefinition").at("passengers")) {
            m_passengerSkirts.push_back(passenger.value("hasSkirt", false));
        }
        for (const auto& event : capture.at("events")) {
            if (event.value("type", "") == "passengerPickup" &&
                event.contains("hostId")) {
                m_passengerPickups.push_back({
                    .id = event.at("id").get<std::size_t>(),
                    .hostId = event.at("hostId").get<PartId>(),
                    .timeMs = event.at("timeMs").get<std::int64_t>()
                });
            }
        }
        for (const auto& source : capture.at("frames")) {
            Frame frame{.timeMs = source.at("timeMs").get<std::int64_t>()};
            for (const auto& state : source.at("parts")) {
                frame.parts.emplace(state.at("id").get<PartId>(), Transform{
                    .x = state.at("xMilli").get<double>() / 1000.0,
                    .y = state.at("yMilli").get<double>() / 1000.0,
                    .rotation = state.at("rotationMilli").get<double>() / 1000.0,
                    .attachmentRotation = state.value(
                        "attachmentRotationMilli", state.at("rotationMilli").get<double>()
                    ) / 1000.0,
                    .hasAttachmentRotation = state.contains("attachmentRotationMilli")
                });
            }
            m_frames.push_back(std::move(frame));
        }
    } catch (const nlohmann::json::exception&) {
        m_definitions.clear();
        m_legacyWheelHosts.clear();
        m_frames.clear();
        m_passengerSkirts.clear();
        m_passengerPickups.clear();
        return false;
    }
    if (m_frames.size() < 2 || m_frames.front().timeMs != 0) {
        m_definitions.clear();
        m_legacyWheelHosts.clear();
        m_frames.clear();
        m_passengerSkirts.clear();
        m_passengerPickups.clear();
        return false;
    }
    m_loaded = true;
    m_status = GhostLoadStatus::Ready;
    m_renderStates.reserve(m_definitions.size());
    m_passengerStates.reserve(m_passengerPickups.size());
    Sample();
    return true;
}

void GhostPlayback::PollRemote() {
#ifdef __EMSCRIPTEN__
    if (!m_requested || m_loaded) return;
    char* raw = ReadChampionGhostWeb();
    const nlohmann::json state = nlohmann::json::parse(raw, nullptr, false);
    std::free(raw);
    if (!state.is_object()) return;
    const std::string status = state.value("status", "idle");
    if (status == "ready") {
        if (!Load(state.value("capture", nlohmann::json::object()))) {
            m_status = GhostLoadStatus::Incompatible;
            m_requested = false;
            m_visible = false;
            m_playing = false;
        }
    } else if (status == "available") {
        m_status = GhostLoadStatus::Available;
    } else if (status == "loading-recording") {
        m_status = GhostLoadStatus::LoadingRecording;
    } else if (status == "unavailable") {
        m_status = GhostLoadStatus::Unavailable;
        m_requested = false;
        m_visible = false;
        m_playing = false;
    } else if (status == "incompatible") {
        m_status = GhostLoadStatus::Incompatible;
        m_requested = false;
        m_visible = false;
        m_playing = false;
    } else if (status == "error") {
        m_status = GhostLoadStatus::Error;
        m_requested = false;
        m_visible = false;
        m_playing = false;
    } else {
        m_status = GhostLoadStatus::Loading;
    }
#endif
}

void GhostPlayback::EnsureLoaded() {
#ifdef __EMSCRIPTEN__
    if (m_status == GhostLoadStatus::Available) {
        m_status = GhostLoadStatus::LoadingRecording;
        LoadPreparedChampionGhostWeb();
    }
#endif
}

void GhostPlayback::Play() {
    m_visible = true;
    m_advanceWhileLoading = false;
    if (GetDurationMs() > 0.0 && m_timeMs >= GetDurationMs()) m_timeMs = 0.0;
    m_playing = true;
    EnsureLoaded();
    if (m_loaded) Sample();
}

void GhostPlayback::Pause() {
    m_playing = false;
}

void GhostPlayback::Restart() {
    m_timeMs = 0.0;
    m_visible = true;
    m_playing = true;
    m_advanceWhileLoading = false;
    EnsureLoaded();
    if (m_loaded) Sample();
}

void GhostPlayback::PrepareCompetition() {
    m_timeMs = 0.0;
    m_visible = true;
    m_playing = false;
    m_advanceWhileLoading = false;
    EnsureLoaded();
    if (m_loaded) Sample();
}

void GhostPlayback::StartCompetition() {
    m_timeMs = 0.0;
    m_visible = true;
    m_playing = true;
    m_advanceWhileLoading = true;
    EnsureLoaded();
    if (m_loaded) Sample();
}

void GhostPlayback::StopAndRewind() {
    m_timeMs = 0.0;
    m_visible = false;
    m_playing = false;
    m_advanceWhileLoading = false;
    m_renderStates.clear();
    m_passengerStates.clear();
}

void GhostPlayback::Update(double frameMs) {
    PollRemote();
    if (m_visible && !m_loaded && m_status == GhostLoadStatus::Available) {
        EnsureLoaded();
    }
    if (m_playing && (m_loaded || m_advanceWhileLoading)) m_timeMs += frameMs;
    if (!m_loaded) return;
    m_timeMs = std::min(
        m_timeMs,
        static_cast<double>(m_frames.back().timeMs)
    );
    Sample();
    if (m_timeMs >= static_cast<double>(m_frames.back().timeMs)) {
        m_playing = false;
    }
}

void GhostPlayback::Sample() {
    if (m_frames.empty()) return;
    const auto upper = std::upper_bound(
        m_frames.begin(), m_frames.end(), m_timeMs,
        [](double time, const Frame& frame) { return time < frame.timeMs; }
    );
    const Frame& second = upper == m_frames.end() ? m_frames.back() : *upper;
    const Frame& first = upper == m_frames.begin() ? m_frames.front() : *(upper - 1);
    const double span = static_cast<double>(second.timeMs - first.timeMs);
    const double amount = span > 0.0
        ? std::clamp((m_timeMs - first.timeMs) / span, 0.0, 1.0)
        : 0.0;
    m_renderStates.clear();
    m_passengerStates.clear();
    for (const auto& [id, firstState] : first.parts) {
        const auto definition = m_definitions.find(id);
        if (definition == m_definitions.end()) continue;
        const auto next = second.parts.find(id);
        // DON'T INTERPOLATE through an appearance/disappearance boundary
        const Transform state = next == second.parts.end() ? firstState : Transform{
            .x = firstState.x + (next->second.x - firstState.x) * amount,
            .y = firstState.y + (next->second.y - firstState.y) * amount,
            .rotation = InterpolateAngle(firstState.rotation, next->second.rotation, amount),
            .attachmentRotation = InterpolateAngle(
                firstState.attachmentRotation, next->second.attachmentRotation, amount
            ),
            .hasAttachmentRotation = firstState.hasAttachmentRotation
        };
        double attachmentRotation = state.attachmentRotation;
        if (definition->second.type == PartType::CircleWheel &&
            !state.hasAttachmentRotation) {
            const auto host = m_legacyWheelHosts.find(id);
            if (host != m_legacyWheelHosts.end()) {
                const auto initialWheel = m_frames.front().parts.find(id);
                const auto initialHost = m_frames.front().parts.find(host->second);
                const auto hostFirst = first.parts.find(host->second);
                const auto hostSecond = second.parts.find(host->second);
                if (initialWheel != m_frames.front().parts.end() &&
                    initialHost != m_frames.front().parts.end() &&
                    hostFirst != first.parts.end()) {
                    const double hostRotation = hostSecond == second.parts.end()
                        ? hostFirst->second.rotation
                        : InterpolateAngle(
                            hostFirst->second.rotation,
                            hostSecond->second.rotation,
                            amount
                        );
                    attachmentRotation = initialWheel->second.rotation +
                        (hostRotation - initialHost->second.rotation);
                }
            }
        }
        m_renderStates.push_back({
            .id = id,
            .type = definition->second.type,
            .position = {state.x, state.y},
            .rotation = state.rotation,
            .attachmentRotation = attachmentRotation,
            .cellSize = m_cellSize
        });
    }
    for (const PassengerPickup& pickup : m_passengerPickups) {
        if (m_timeMs < pickup.timeMs || pickup.id >= m_passengerSkirts.size()) continue;
        const auto host = std::find_if(
            m_renderStates.begin(),
            m_renderStates.end(),
            [&pickup](const GhostRenderState& state) {
                return state.id == pickup.hostId;
            }
        );
        if (host == m_renderStates.end()) continue;
        m_passengerStates.push_back({
            .position = {host->position.x, host->position.y + 7.0},
            .hasSkirt = m_passengerSkirts[pickup.id]
        });
    }
}

void GhostPlayback::Reset() {
    m_levelId.clear();
    m_timeMs = 0.0;
    m_requested = false;
    m_loaded = false;
    m_visible = false;
    m_playing = false;
    m_advanceWhileLoading = false;
    m_status = GhostLoadStatus::Idle;
    m_definitions.clear();
    m_legacyWheelHosts.clear();
    m_frames.clear();
    m_renderStates.clear();
    m_passengerSkirts.clear();
    m_passengerPickups.clear();
    m_passengerStates.clear();
}
