#include "app/game/GhostPlayback.hpp"

#include <algorithm>
#include <cmath>

#ifdef __EMSCRIPTEN__
#include <cstdlib>
#include <emscripten.h>
EM_JS(void, LoadChampionGhostWeb, (const char* level), {
    Module['api'].loadChampionRecording(UTF8ToString(level));
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

void GhostPlayback::Start(const std::string& levelId, float cellSize) {
    Reset();
    m_levelId = levelId;
    m_cellSize = cellSize;
    if (levelId.empty()) return;
    m_requested = true;
#ifdef __EMSCRIPTEN__
    LoadChampionGhostWeb(levelId.c_str());
#endif
}

void GhostPlayback::Load(const nlohmann::json& capture) {
    if (
        !capture.is_object() || capture.value("levelId", "") != m_levelId ||
        capture.value("formatVersion", "") != "run-v2" ||
        capture.value("physicsVersion", "") != "custom-physics-v1" ||
        capture.value("rulesVersion", "") != "score-v2"
    ) return;
    try {
        for (const auto& part : capture.at("construction")) {
            m_definitions.emplace(part.at("id").get<PartId>(), Definition{
                .type = static_cast<PartType>(part.at("type").get<int>()),
                .orientation = static_cast<PartOrientation>(part.at("orientation").get<int>())
            });
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
                    .rotation = state.at("rotationMilli").get<double>() / 1000.0
                });
            }
            m_frames.push_back(std::move(frame));
        }
    } catch (const nlohmann::json::exception&) {
        Reset();
        return;
    }
    if (m_frames.size() < 2 || m_frames.front().timeMs != 0) {
        Reset();
        return;
    }
    m_loaded = true;
    Sample();
}

void GhostPlayback::Update(double frameMs) {
    if (!m_requested) return;
    // Keep the ghost on the run timeline even while its file is downloading!
    m_timeMs += frameMs;
#ifdef __EMSCRIPTEN__
    if (!m_loaded) {
        char* raw = ReadChampionGhostWeb();
        const nlohmann::json state = nlohmann::json::parse(raw, nullptr, false);
        std::free(raw);
        if (state.is_object() && state.value("status", "") == "ready") {
            Load(state.value("capture", nlohmann::json::object()));
        } else if (state.is_object() && state.value("status", "") == "unavailable") {
            m_requested = false;
        }
    }
#endif
    if (!m_loaded) return;
    m_timeMs = std::min(
        m_timeMs,
        static_cast<double>(m_frames.back().timeMs)
    );
    Sample();
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
    std::unordered_map<PartId, Transform> sampled;
    for (const auto& [id, firstState] : first.parts) {
        const auto definition = m_definitions.find(id);
        if (definition == m_definitions.end()) continue;
        const auto next = second.parts.find(id);
        // DON'T INTERPOLATE through an appearance/disappearance boundary
        const Transform state = next == second.parts.end() ? firstState : Transform{
            .x = firstState.x + (next->second.x - firstState.x) * amount,
            .y = firstState.y + (next->second.y - firstState.y) * amount,
            .rotation = InterpolateAngle(firstState.rotation, next->second.rotation, amount)
        };
        sampled.emplace(id, state);
        m_renderStates.push_back({
            .type = definition->second.type,
            .position = {state.x, state.y},
            .rotation = state.rotation,
            .attachmentRotation = state.rotation,
            .cellSize = m_cellSize
        });
    }
    for (const PassengerPickup& pickup : m_passengerPickups) {
        if (m_timeMs < pickup.timeMs || pickup.id >= m_passengerSkirts.size()) continue;
        const auto host = sampled.find(pickup.hostId);
        if (host == sampled.end()) continue;
        m_passengerStates.push_back({
            .position = {host->second.x, host->second.y + 7.0},
            .hasSkirt = m_passengerSkirts[pickup.id]
        });
    }
}

void GhostPlayback::Reset() {
    m_levelId.clear();
    m_timeMs = 0.0;
    m_requested = false;
    m_loaded = false;
    m_definitions.clear();
    m_frames.clear();
    m_renderStates.clear();
    m_passengerSkirts.clear();
    m_passengerPickups.clear();
    m_passengerStates.clear();
}
