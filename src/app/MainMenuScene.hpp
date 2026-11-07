#ifndef MAIN_MENU_SCENE_HPP
#define MAIN_MENU_SCENE_HPP

#include "app/I_Scene.hpp"
#include "app/AppContext.hpp"

enum class SceneId;

class MainMenuScene : public I_Scene {
    AppContext* m_context;
    bool m_selectingLevel = false;
    int m_selectedLevel = 0;
public:
    explicit MainMenuScene(AppContext* context): m_context(context) {}
    void ProcessInput(float) override;
    void Update(float) override {}
    void Render() const override;
};

#endif
