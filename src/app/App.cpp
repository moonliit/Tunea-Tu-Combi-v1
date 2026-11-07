#include "app/App.hpp"
#include "app/game/Level.hpp"
#include "app/game/MainGameScene.hpp"
#include "app/editor/EditorScene.hpp"
#include "app/MainMenuScene.hpp"

#include <algorithm>
#include <raylib.h>
#include <utility>

namespace {

constexpr const char* StartupLevelPath = "assets/levels/default.json";
constexpr SceneId StartupScene = SceneId::MainMenu;

}

App::App(int width, int height, const std::string& title):
    m_sceneRequester(&m_sceneManager)
{
    InitWindow(width, height, title.c_str());
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    m_appContext = {
        .windowWidth = width,
        .windowHeight = height,
        .windowTitle = title,
        .sceneRequester = &m_sceneRequester
    };
    m_sceneManager.SetAppContext(&m_appContext);
    LevelDefinition emptyDef = LevelDefinition::FromJsonPath(StartupLevelPath);

    m_sceneManager.AddScene(
        SceneId::Game,
        std::make_unique<MainGameScene>(&m_appContext, std::move(emptyDef))
    );
    m_sceneManager.AddScene(
        SceneId::Editor,
        std::make_unique<EditorScene>(&m_appContext)
    );
    m_sceneManager.AddScene(
        SceneId::MainMenu,
        std::make_unique<MainMenuScene>(&m_appContext)
    );
    m_sceneManager.SetCurrentScene({StartupScene});
}

void App::Run() {
    while (!WindowShouldClose()) {
        float deltaTime = std::min(GetFrameTime(), 0.033f);
        m_sceneManager.ProcessInput(deltaTime);
        m_sceneManager.Update(deltaTime);
        m_sceneManager.Render();
    }
}
