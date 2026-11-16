#ifndef APP_CUSTOM_LEVEL_EDITOR_SCENE_HPP
#define APP_CUSTOM_LEVEL_EDITOR_SCENE_HPP
#include "app/I_Scene.hpp"
#include "math/Vector2D.hpp"
#include <raylib.h>
#include <optional>
#include <string>
#include <vector>
struct AppContext;
class CustomLevelEditorScene final : public I_Scene {
    AppContext* m_context;
    Camera2D m_designCamera{};
    enum class DesignTool { Terrain, Grid, Goal, Eraser, Passenger, TntObstacle };
    DesignTool m_designTool = DesignTool::Terrain;
    std::optional<Math::Vector2D> m_gridAnchor;
    Math::Vector2D m_gridPreviewCursor{};
    int m_gridAnchorLevel = -1;
    std::vector<Math::Vector2D> m_terrainWorkPoints;
    std::optional<Vector2> m_lastEraserMouseScreen;
    std::optional<Vector2> m_rightDragStart;
    bool m_rightDragged = false;
    bool m_rightPressOnCanvas = false;
    std::vector<Math::Vector2D> m_erasedPassengerPositions;
    bool m_musicPickerOpen = false;
    std::string m_publishFeedbackDraftId;
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
