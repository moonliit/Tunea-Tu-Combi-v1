#ifndef APP_CUSTOM_LEVEL_EDITOR_SCENE_HPP
#define APP_CUSTOM_LEVEL_EDITOR_SCENE_HPP
#include "app/I_Scene.hpp"
#include "math/Vector2D.hpp"
#include <raylib.h>
#include <vector>
struct AppContext;
class CustomLevelEditorScene final : public I_Scene {
    AppContext* m_context;
    Camera2D m_designCamera{};
    enum class DesignTool { Terrain, Grid, Goal, Eraser };
    DesignTool m_designTool = DesignTool::Terrain;
    std::vector<Math::Vector2D> m_terrainWorkPoints;
    bool m_musicPickerOpen = false;
    int m_musicCategory = 0;
    int m_musicTrack = 0;
    float m_musicListScroll = 0.0f;
public:
    explicit CustomLevelEditorScene(AppContext* context);
    void ProcessInput(float) override;
    void Update(float) override {}
    void Render() const override;
};
#endif
