#ifndef GAME_GHOST_PLAYBACK_HPP
#define GAME_GHOST_PLAYBACK_HPP

#include "app/game/Part.hpp"

#include <cstdint>
#include <json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

struct GhostRenderState {
    PartType type;
    Math::Vector2D position;
    double rotation;
    double attachmentRotation;
    float cellSize;
};

struct GhostPassengerState {
    Math::Vector2D position;
    bool hasSkirt;
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
    std::unordered_map<PartId, Definition> m_definitions;
    std::vector<Frame> m_frames;
    std::vector<GhostRenderState> m_renderStates;
    struct PassengerPickup {
        std::size_t id;
        PartId hostId;
        std::int64_t timeMs;
    };
    std::vector<bool> m_passengerSkirts;
    std::vector<PassengerPickup> m_passengerPickups;
    std::vector<GhostPassengerState> m_passengerStates;

    void Load(const nlohmann::json& capture);
    void Sample();

public:
    void Start(const std::string& levelId, float cellSize);
    void Update(double frameMs);
    void Reset();
    bool IsLoaded() const { return m_loaded; }
    const std::vector<GhostRenderState>& GetRenderStates() const {
        return m_renderStates;
    }
    const std::vector<GhostPassengerState>& GetPassengerStates() const {
        return m_passengerStates;
    }
};

#endif
