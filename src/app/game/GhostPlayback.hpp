#ifndef GAME_GHOST_PLAYBACK_HPP
#define GAME_GHOST_PLAYBACK_HPP

#include "app/game/Part.hpp"

#include <cstdint>
#include <json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

struct GhostRenderState {
    PartId id;
    PartType type;
    Math::Vector2D position;
    double rotation;
    double attachmentRotation;
    float cellSize;
};

struct GhostPassengerState {
    Math::Vector2D position;
    bool hasSkirt;
    std::string face;
};

enum class GhostLoadStatus {
    Idle,
    Loading,
    Available,
    LoadingRecording,
    Ready,
    Unavailable,
    Incompatible,
    Error
};

class GhostPlayback {
    struct Definition {
        PartType type;
        PartOrientation orientation;
    };
    struct Transform {
        double x;
        double y;
        double rotation;
        double attachmentRotation;
        bool hasAttachmentRotation;
    };
    struct Frame {
        std::int64_t timeMs;
        std::unordered_map<PartId, Transform> parts;
    };

    std::string m_levelId;
    float m_cellSize = 1.0f;
    double m_timeMs = 0.0;
    bool m_requested = false;
    bool m_loaded = false;
    bool m_visible = false;
    bool m_playing = false;
    bool m_advanceWhileLoading = false;
    GhostLoadStatus m_status = GhostLoadStatus::Idle;
    std::unordered_map<PartId, Definition> m_definitions;
    std::unordered_map<PartId, PartId> m_legacyWheelHosts;
    std::vector<Frame> m_frames;
    std::vector<GhostRenderState> m_renderStates;
    struct PassengerPickup {
        std::size_t id;
        PartId hostId;
        std::int64_t timeMs;
    };
    std::vector<bool> m_passengerSkirts;
    std::vector<std::string> m_passengerFaces;
    std::vector<PassengerPickup> m_passengerPickups;
    std::vector<GhostPassengerState> m_passengerStates;

    bool Load(const nlohmann::json& capture);
    void PollRemote();
    void Sample();

public:
    void Request(const std::string& levelId, float cellSize);
    void EnsureLoaded();
    void Play();
    void Pause();
    void Restart();
    void PrepareCompetition();
    void StartCompetition();
    void StopAndRewind();
    void Update(double frameMs);
    void Reset();
    bool IsLoaded() const { return m_loaded; }
    bool IsVisible() const { return m_visible && m_loaded; }
    bool IsPlaying() const { return m_playing; }
    GhostLoadStatus GetStatus() const { return m_status; }
    double GetTimeMs() const { return m_timeMs; }
    double GetDurationMs() const {
        return m_frames.empty() ? 0.0 : static_cast<double>(m_frames.back().timeMs);
    }
    const std::vector<GhostRenderState>& GetRenderStates() const {
        return m_renderStates;
    }
    const std::vector<GhostPassengerState>& GetPassengerStates() const {
        return m_passengerStates;
    }
};

#endif
