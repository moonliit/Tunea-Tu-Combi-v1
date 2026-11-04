#ifndef GAME_HPP
#define GAME_HPP

#include "game/AppState.hpp"
#include "game/BuildSession.hpp"
#include "game/Level.hpp"
#include "game/Part.hpp"
#include "game/PlaySession.hpp"
#include "input/GameInput.hpp"
#include "rendering/GameUiRenderer.hpp"
#include "rendering/WorldRenderer.hpp"

#include <raylib.h>
#include <optional>
#include <memory>
#include <vector>

class Game {
private:
    int m_screenWidth;
    int m_screenHeight;
    Camera2D m_camera;

    float m_gravity;
    AppStateMachine m_stateMachine;
    Level m_level;
    BuildSession m_buildSession;
    PlaySession m_playSession;
    GameInput m_input;
    WorldRenderer m_worldRenderer;
    GameUiRenderer m_uiRenderer;

    void ProcessInput(float dt);
    void Update(float dt);
    void Render();
    void ResetToBuildMode();
    void TransitionToPlayMode();

public:
    Game(int width, int height, const std::string& title, LevelSpecs specs);
    ~Game() = default;

    void Run();
};

#endif
