#include "app/App.hpp"
#include "app/game/MainGameScene.hpp"

#include <algorithm>
#include <raylib.h>
#include <utility>

App::App(int width, int height, const std::string& title, LevelSpecs specs) {
    InitWindow(width, height, title.c_str());
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    m_sceneManager.Add(
        SceneId::MainGame,
        std::make_unique<MainGameScene>(width, height, std::move(specs))
    );
    m_sceneManager.SetCurrent(SceneId::MainGame);
}

void App::Run() {
    while (!WindowShouldClose()) {
        float deltaTime = std::min(GetFrameTime(), 0.033f);
        m_sceneManager.ProcessInput(deltaTime);
        m_sceneManager.Update(deltaTime);
        m_sceneManager.Render();
    }
}
