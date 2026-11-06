#include "app/App.hpp"
#include "app/game/Level.hpp"
#include "app/game/MainGameScene.hpp"
#include "app/editor/EditorScene.hpp"

#include <algorithm>
#include <raylib.h>
#include <utility>
#include <print>

App::App(int width, int height, const std::string& title) {
    InitWindow(width, height, title.c_str());
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    SceneRequester requester(&m_sceneManager);
    m_appContext = {
        .windowWidth = width,
        .windowHeight = height,
        .windowTitle = title,
        .sceneRequester = &requester
    };
    m_sceneManager.SetAppContext(&m_appContext);
    // LevelDefinition emptyDef = LevelDefinition::Default();
    LevelDefinition emptyDef = LevelDefinition::FromJsonPath("assets/levels/default.json");
    // LevelDefinition emptyDef = LevelDefinition::FromJsonPath("assets/levels/level_1.json");

    std::println("grid dims: {}x{}", emptyDef.grid.dims.x, emptyDef.grid.dims.y);

    m_sceneManager.AddScene(
        SceneId::Game,
        std::make_unique<MainGameScene>(&m_appContext, std::move(emptyDef))
    );
    m_sceneManager.AddScene(
        SceneId::Editor,
        std::make_unique<EditorScene>(&m_appContext)
    );
    m_sceneManager.SetCurrentScene({SceneId::Game});
    // m_sceneManager.SetCurrentScene({SceneId::Editor});
    // m_sceneManager.SetCurrentScene({SceneId::MainMenu});
}

void App::Run() {
    while (!WindowShouldClose()) {
        float deltaTime = std::min(GetFrameTime(), 0.033f);
        m_sceneManager.ProcessInput(deltaTime);
        m_sceneManager.Update(deltaTime);
        m_sceneManager.Render();
    }
}
