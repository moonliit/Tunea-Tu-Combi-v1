#ifndef GAME_MAIN_GAME_SCENE_HPP
#define GAME_MAIN_GAME_SCENE_HPP

#include "app/game/GameState.hpp"
#include "app/game/BuildSession.hpp"
#include "app/game/Level.hpp"
#include "app/game/PlaySession.hpp"
#include "app/I_Scene.hpp"
#include "identity/IdentitySession.hpp"
#include "input/GameInput.hpp"
#include "rendering/GameUiRenderer.hpp"
#include "rendering/WorldRenderer.hpp"

#include <raylib.h>
#include <vector>

struct AppContext;

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
    float m_driverBeyondGoalSeconds = 0.0f;
    std::vector<bool> m_passengerCollected;
    std::vector<std::pair<Part*, bool>> m_carriedPassengers;

    void ResetToBuildMode();
    void ResetEditorCamera();
    void TransitionToPlayMode();

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
