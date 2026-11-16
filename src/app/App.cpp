#include "app/App.hpp"
#include "app/game/Level.hpp"
#include "app/game/MainGameScene.hpp"
#include "app/MainMenuScene.hpp"
#include "app/CustomLevelCatalog.hpp"
#include "app/CustomLevelEditorScene.hpp"
#include "app/OfficialProgressStore.hpp"

#include <algorithm>
#include <raylib.h>
#include <utility>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace {

constexpr const char* StartupLevelPath = "assets/levels/default.json";
constexpr SceneId StartupScene = SceneId::MainMenu;

void ShowLoadingScreen(int width, int height) {
    const Texture2D loadingText = LoadTexture(
        "assets/textures/ui/loading/cargando.png"
    );
    BeginDrawing();
    ClearBackground(BLACK);
    if (loadingText.id != 0) {
        const float drawWidth = std::min(
            static_cast<float>(loadingText.width), width * 0.65f
        );
        const float drawHeight = drawWidth * loadingText.height / loadingText.width;
        DrawTexturePro(loadingText,
            Rectangle{0.0f, 0.0f,
                static_cast<float>(loadingText.width),
                static_cast<float>(loadingText.height)},
            Rectangle{(width - drawWidth) / 2.0f,
                (height - drawHeight) / 2.0f, drawWidth, drawHeight},
            Vector2{}, 0.0f, WHITE);
    } else {
        constexpr const char* fallback = "Cargando...";
        constexpr int fontSize = 40;
        DrawText(fallback,
            (width - MeasureText(fallback, fontSize)) / 2,
            (height - fontSize) / 2, fontSize, RAYWHITE);
    }
    EndDrawing();
    if (loadingText.id != 0) UnloadTexture(loadingText);
#ifdef __EMSCRIPTEN__
    emscripten_sleep(0);  // lets the browser show it before the loading starts
#endif
}

}

App::App(int width, int height, const std::string& title):
    m_sceneRequester(&m_sceneManager)
{
    InitWindow(width, height, title.c_str());
    ShowLoadingScreen(width, height);
    m_audioController.Initialize();
    m_textureLibrary.Initialize();
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    m_appContext = {
        .windowWidth = width,
        .windowHeight = height,
        .windowTitle = title,
        .sceneRequester = &m_sceneRequester,
        .audioController = &m_audioController,
        .textureLibrary = &m_textureLibrary,
        .customLevels = std::make_shared<CustomLevelCatalog>()
    };
#ifndef __EMSCRIPTEN__
    m_appContext.unlockedLevel = OfficialProgressStore::LoadUnlockedLevel();
#endif
    m_sceneManager.SetAppContext(&m_appContext);
    LevelDefinition emptyDef = LevelDefinition::FromJsonPath(StartupLevelPath);

    m_sceneManager.AddScene(
        SceneId::Game,
        std::make_unique<MainGameScene>(&m_appContext, std::move(emptyDef))
    );
    m_sceneManager.AddScene(
        SceneId::CustomLevelEditor,
        std::make_unique<CustomLevelEditorScene>(&m_appContext)
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
        m_appContext.identity.Poll();
        m_appContext.publications.Poll(static_cast<double>(deltaTime) * 1000.0);
        m_sceneManager.ProcessInput(deltaTime);
        m_sceneManager.Update(deltaTime);
        m_audioController.Update(deltaTime);
        m_sceneManager.Render();
    }
}

App::~App() {
    m_textureLibrary.Shutdown();
    m_audioController.Shutdown();
    CloseWindow();
}
