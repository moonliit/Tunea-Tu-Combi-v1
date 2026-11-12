#ifndef GAME_MAIN_GAME_SCENE_HPP
#define GAME_MAIN_GAME_SCENE_HPP

#include "app/game/GameState.hpp"
#include "app/game/BuildSession.hpp"
#include "app/game/GoalWindowTracker.hpp"
#include "app/game/Level.hpp"
#include "app/game/PlaySession.hpp"
#include "app/game/RunCapture.hpp"
#include "app/I_Scene.hpp"
#include "identity/IdentitySession.hpp"
#include "input/GameInput.hpp"
#include "physics/I_Physics.hpp"
#include "rendering/GameUiRenderer.hpp"
#include "rendering/WorldRenderer.hpp"
#include "scoring/ScoreRules.hpp"

#include <raylib.h>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct AppContext;

struct CarriedPassenger {
    std::size_t id;
    PartId hostId;
    bool hasSkirt;
};

class MainGameScene final : public I_Scene {
private:
    AppContext* m_appContext;
    Camera2D m_camera;
    Vector2 m_editorCameraTarget;
    float m_editorCameraZoom = 1.0f;
    float m_gravity;
    GameStateMachine m_stateMachine;
    bool m_isCustomLevel = false;
    Level m_level;
    BuildSession m_buildSession;
    PlaySession m_playSession;
    GameInput m_input;
    WorldRenderer m_worldRenderer;
    GameUiRenderer m_uiRenderer;
    bool m_levelCompleted = false;
    IdentityScope m_runIdentity;
    double m_elapsedMs = 0.0;
    std::vector<bool> m_passengerCollected;
    std::vector<CarriedPassenger> m_carriedPassengers;
    std::unordered_set<PartId> m_initialPartIds;
    std::unordered_map<PartId, double> m_previousPartX;
    GoalWindowTracker m_goalWindow;
    Scoring::RunStatistics m_statistics;
    Scoring::ScoreBreakdown m_score;
    RunCapture m_capture;

    void ResetToBuildMode();
    void ResetEditorCamera();
    void TransitionToPlayMode();
    void UpdateGoalWindow(double frameMs);
    void CompleteRun();
    void RenderResults() const;

public:
    MainGameScene(
        AppContext* appContext,
        const LevelDefinition& levelDef
    );

    // prevents assigning a run to a new identity
    std::optional<std::string> PersonalRecordOwner() const;
    void ProcessInput(float deltaTime) override;
    void Update(float deltaTime) override;
    void Render() const override;
};

#endif
