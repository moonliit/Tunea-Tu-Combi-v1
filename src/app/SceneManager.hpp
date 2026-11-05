#ifndef GAME_SCENE_MANAGER_HPP
#define GAME_SCENE_MANAGER_HPP

#include "app/I_Scene.hpp"
#include <map>
#include <memory>

enum class SceneId {
    MainGame
};

class SceneManager {
private:
    std::map<SceneId, std::unique_ptr<I_Scene>> m_scenes;
    SceneId m_currentScene;

public:
    explicit SceneManager(SceneId initialScene = SceneId::MainGame):
        m_currentScene(initialScene) {}

    void Add(SceneId id, std::unique_ptr<I_Scene> scene);
    void SetCurrent(SceneId id);
    SceneId GetCurrent() const { return m_currentScene; }

    void ProcessInput(float deltaTime);
    void Update(float deltaTime);
    void Render() const;
};

#endif
