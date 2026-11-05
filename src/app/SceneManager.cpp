#include "app/SceneManager.hpp"

#include <stdexcept>

void SceneManager::Add(SceneId id, std::unique_ptr<I_Scene> scene) {
    if (!scene) {
        throw std::invalid_argument("A scene manager cannot add a null scene");
    }
    m_scenes.insert_or_assign(id, std::move(scene));
}

void SceneManager::SetCurrent(SceneId id) {
    if (!m_scenes.contains(id)) {
        throw std::logic_error("Cannot activate an unregistered scene");
    }
    m_currentScene = id;
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
