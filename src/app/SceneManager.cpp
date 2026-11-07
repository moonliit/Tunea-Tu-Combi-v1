#include "app/SceneManager.hpp"
#include "app/game/MainGameScene.hpp"
#include "app/game/Level.hpp"
#include <stdexcept>

SceneRequester::SceneRequester(SceneManager* sceneManager):
    m_sceneManager(sceneManager) {}

void SceneRequester::RequestSceneChange(const SceneRequest& req) {
    m_sceneManager->SetCurrentScene(req);
}

void SceneManager::SetAppContext(AppContext* appContext) {
    m_appContext = appContext;
}

void SceneManager::AddScene(SceneId id, std::unique_ptr<I_Scene> scene) {
    if (!scene) {
        throw std::invalid_argument("A scene manager cannot add a null scene");
    }
    m_scenes.insert_or_assign(id, std::move(scene));
}

void SceneManager::SetCurrentScene(const SceneRequest& req) {
    if (!m_scenes.contains(req.id)) {
        throw std::logic_error("Cannot activate an unregistered scene");
    }
    if (m_currentScene == req.id) {
        return; // no change needed
    }

    if (
        m_currentScene == SceneId::MainMenu &&
        req.id == SceneId::Game &&
        req.data != nullptr
    ) {
        LevelDefinition& levelDef = *(LevelDefinition*)req.data;
        // evict previous game scene and replace it with another one with a new level
        AddScene(
            SceneId::Game,
            std::make_unique<MainGameScene>(m_appContext, std::move(levelDef))
        );
    }
    m_currentScene = req.id;
}

void SceneManager::ProcessInput(float deltaTime) {
    m_scenes.at(m_currentScene)->ProcessInput(deltaTime);
}

void SceneManager::Update(float deltaTime) {
    m_scenes.at(m_currentScene)->Update(deltaTime);
}

void SceneManager::Render() const {
    m_scenes.at(m_currentScene)->Render();
}
