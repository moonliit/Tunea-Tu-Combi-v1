#ifndef GAME_MAIN_GAME_SCENE_HPP
#define GAME_MAIN_GAME_SCENE_HPP

#include "app/game/GameState.hpp"
#include "app/game/BuildSession.hpp"
#include "app/game/Level.hpp"
#include "app/game/PlaySession.hpp"
#include "app/I_Scene.hpp"
#include "input/GameInput.hpp"
#include "rendering/GameUiRenderer.hpp"
#include "rendering/WorldRenderer.hpp"

#include <raylib.h>

class MainGameScene final : public I_Scene {
private:
    int m_screenWidth;
    int m_screenHeight;
    Camera2D m_camera;
    Vector2 m_editorCameraTarget;
    float m_editorCameraZoom = 1.0f;
    float m_gravity;
    GameStateMachine m_stateMachine;
    Level m_level;
    BuildSession m_buildSession;
    PlaySession m_playSession;
    GameInput m_input;
    WorldRenderer m_worldRenderer;
    GameUiRenderer m_uiRenderer;

    void ResetToBuildMode();
    void ResetEditorCamera();
    void TransitionToPlayMode();

public:
    MainGameScene(int width, int height, LevelSpecs specs);
    void ProcessInput(float deltaTime) override;
    void Update(float deltaTime) override;
    void Render() const override;
};

#endif
