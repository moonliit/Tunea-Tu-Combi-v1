#ifndef MAIN_MENU_SCENE_HPP
#define MAIN_MENU_SCENE_HPP

#include "app/I_Scene.hpp"
#include "app/AppContext.hpp"
#include "app/game/BuildModel.hpp"
#include "app/game/PlaySession.hpp"
#include "rendering/WorldRenderer.hpp"
#include <raylib.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

class MainMenuScene : public I_Scene {
    AppContext* m_context;
    bool m_selectingLevel = false;
    bool m_editingLevels = false;
    bool m_browsingCommunity = false;
    bool m_communitySearchOpen = false;
    std::string m_communityQuery;
    int m_communityScroll = 0;
    int m_selectedLevel = 0;
    std::string m_newLevelName;
    bool m_creatingLevel = false;
    float m_customListScroll = 0.0f;

    std::unique_ptr<BuildModel> m_titleModel;
    std::unique_ptr<PlaySession> m_titleSession;
    std::unique_ptr<WorldRenderer> m_titleRenderer;
    Camera2D m_titleCamera{};
    const Part* m_titleDriver = nullptr;
    double m_titleElapsedMs = 0.0;
    float m_titleTerrainLength = 0.0f;

    void ProcessLevelEditorInput();
    void ProcessCommunityInput();
    void InitTitleWorld();
    void ResetTitleWorld();
    void UpdateTitleWorld(float deltaTime);
    void RenderTitleWorld() const;
    std::optional<Vector2> GetTitleDriverPosition() const;

public:
    explicit MainMenuScene(AppContext* context);
    void ProcessInput(float) override;
    void Update(float) override;
    void Render() const override;
};

#endif
