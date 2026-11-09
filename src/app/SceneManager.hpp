#ifndef SCENE_MANAGER_HPP
#define SCENE_MANAGER_HPP

#include "app/I_Scene.hpp"
#include <map>
#include <memory>

struct AppContext;

enum class SceneId {
    MainMenu,
    Editor,
    CustomLevelEditor,
    Game,
};

struct SceneRequest {
    SceneId id;
    void* data = nullptr;
};

class SceneManager {
private:
    std::map<SceneId, std::unique_ptr<I_Scene>> m_scenes;
    AppContext* m_appContext;
    SceneId m_currentScene;

public:
    explicit SceneManager(SceneId initialScene = SceneId::Game):
        m_currentScene(initialScene) {}

    void AddScene(SceneId id, std::unique_ptr<I_Scene> scene);
    void SetCurrentScene(const SceneRequest& req);
    SceneId GetCurrentScene() const { return m_currentScene; }

    void SetAppContext(AppContext* appContext);
    void ProcessInput(float deltaTime);
    void Update(float deltaTime);
    void Render() const;
};

class SceneRequester {
private:
    SceneManager* m_sceneManager;

public:
    explicit SceneRequester(SceneManager* sceneManager);
    void RequestSceneChange(const SceneRequest& req);
};

#endif
