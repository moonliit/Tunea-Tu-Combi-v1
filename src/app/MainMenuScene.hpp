#ifndef MAIN_MENU_SCENE_HPP
#define MAIN_MENU_SCENE_HPP

#include "app/I_Scene.hpp"
#include "app/AppContext.hpp"
#include <raylib.h>

#include <string>
#include <vector>

class MainMenuScene : public I_Scene {
    AppContext* m_context;
    bool m_selectingLevel = false;
    bool m_editingLevels = false;
    int m_selectedLevel = 0;
    std::string m_newLevelName;
    bool m_creatingLevel = false;
    float m_customListScroll = 0.0f;

    void ProcessLevelEditorInput();
public:
    explicit MainMenuScene(AppContext* context);
    void ProcessInput(float) override;
    void Update(float) override {}
    void Render() const override;
};

#endif
