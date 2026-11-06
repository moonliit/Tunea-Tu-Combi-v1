#ifndef EDITOR_SCENE_HPP
#define EDITOR_SCENE_HPP

#include "app/I_Scene.hpp"
#include "math/Vector2D.hpp"
#include "math/Terrain.hpp"
#include <raylib.h>
#include <vector>

enum class EditorTool {
    PlacePoint,
    SelectSegmentType
};

struct WorkingSegmentPoint {
    Math::Vector2D point;
    Math::TerrainSegmentType type;
};

class EditorScene final : public I_Scene {
private:
    int m_screenWidth;
    int m_screenHeight;
    Camera2D m_camera;

    EditorTool m_currentTool;
    mutable Math::TerrainSegmentType m_activeSegmentType;
    mutable std::vector<WorkingSegmentPoint> m_currentPolygonWorkPoints;
    std::vector<Math::TerrainPolygon> m_completedPolygons;

    static constexpr size_t MAX_FILENAME_LENGTH = 128;
    mutable char m_filenameBuffer[MAX_FILENAME_LENGTH];

    void HandleCameraInput(float deltaTime);
    void HandleMouseInput();
    void DrawUI();

public:
    EditorScene(int width, int height);
    ~EditorScene();

    void ProcessInput(float deltaTime) override;
    void Update(float deltaTime) override;
    void Render() const override;
};

#endif
