#include "app/CustomLevelEditorScene.hpp"
#include "app/AppContext.hpp"
#include "app/CustomLevelCatalog.hpp"
#include "app/CustomLevelData.hpp"
#include "app/SceneManager.hpp"
#include "app/AudioController.hpp"
#include "app/game/Level.hpp"
#include "app/SceneManager.hpp"
#include "ui/UiButton.hpp"
#include <raylib.h>
#include <algorithm>
#include <utility>

CustomLevelEditorScene::CustomLevelEditorScene(AppContext* context): m_context(context) {
    m_designCamera.offset = {context->windowWidth / 2.0f, context->windowHeight / 2.0f};
    m_designCamera.target = m_designCamera.offset;
    m_designCamera.zoom = 1.0f;
}

void CustomLevelEditorScene::ProcessInput(float) {
    if (m_context->activeCustomLevel < 0 ||
        m_context->activeCustomLevel >= static_cast<int>(m_context->customLevels->GetLevels().size())) {
        m_context->sceneRequester->RequestSceneChange({SceneId::MainMenu});
        return;
    }
    CustomLevelData& custom = m_context->customLevels->GetLevels()[m_context->activeCustomLevel];

    const Rectangle terrainButton{20.0f, 16.0f, 120.0f, 36.0f};
    const Rectangle gridButton{146.0f, 16.0f, 120.0f, 36.0f};
    const Rectangle goalButton{272.0f, 16.0f, 100.0f, 36.0f};
    const Rectangle editButton{
        static_cast<float>(m_context->windowWidth - 132), 16.0f, 112.0f, 36.0f
    };
    const Rectangle menuButton = UiButton::CenteredAtBottom(
        m_context->windowWidth,
        m_context->windowHeight,
        110.0f,
        38.0f,
        18.0f
    );

    if (UiButton::WasClicked(terrainButton)) {
        m_designTool = DesignTool::Terrain;
        return;
    }
    if (UiButton::WasClicked(gridButton)) {
        m_designTool = DesignTool::Grid;
        return;
    }
    if (UiButton::WasClicked(goalButton)) {
        m_designTool = DesignTool::Goal;
        return;
    }
    if (UiButton::WasClicked(menuButton)) {
        m_context->audioController->PlayUiClick();
        m_context->sceneRequester->RequestSceneChange({SceneId::MainMenu});
        m_terrainWorkPoints.clear();
        return;
    }
    if (UiButton::WasClicked(editButton)) {
        if (!custom.hasTerrain || !custom.hasGrid || !custom.hasGoal) return;
        m_context->audioController->PlayUiClick();
        m_context->customLevels->Save();
        LevelDefinition definition = custom.definition;
        definition.levelNumber = -1;
        definition.initialCamera.reset();
        m_context->audioController->StartLevel(1);
        m_context->sceneRequester->RequestSceneChange({SceneId::Game, &definition});
        return;
    }

    if (IsKeyDown(KEY_W)) m_designCamera.target.y -= 8.0f / m_designCamera.zoom;
    if (IsKeyDown(KEY_S)) m_designCamera.target.y += 8.0f / m_designCamera.zoom;
    if (IsKeyDown(KEY_A)) m_designCamera.target.x -= 8.0f / m_designCamera.zoom;
    if (IsKeyDown(KEY_D)) m_designCamera.target.x += 8.0f / m_designCamera.zoom;
    m_designCamera.zoom = std::clamp(
        m_designCamera.zoom + GetMouseWheelMove() * 0.08f,
        0.25f,
        2.0f
    );

    const Vector2 screenMouse = GetMousePosition();
    if (screenMouse.y < 65.0f || screenMouse.y > m_context->windowHeight - 58.0f) {
        return;
    }
    const Vector2 worldMouse = GetScreenToWorld2D(screenMouse, m_designCamera);
    const Math::Vector2D point{worldMouse.x, worldMouse.y};

    if (m_designTool == DesignTool::Terrain &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_terrainWorkPoints.push_back(point);
    } else if (m_designTool == DesignTool::Terrain &&
               IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) &&
               m_terrainWorkPoints.size() >= 3) {
        Math::TerrainPolygon polygon;
        std::vector<Math::Vector2D> points = m_terrainWorkPoints;
        points.push_back(points.front());
        polygon.segments.push_back(Math::TerrainSegment{
            .type = Math::TerrainSegmentType::Line,
            .points = std::move(points),
            .resolution = 1
        });
        custom.definition.terrain.polygons.push_back(std::move(polygon));
        custom.hasTerrain = true;
        m_terrainWorkPoints.clear();
        m_context->customLevels->Save();
    } else if (m_designTool == DesignTool::Grid &&
               IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const float gridWidth = custom.definition.grid.dims.x *
            custom.definition.grid.cellSize;
        const float gridHeight = custom.definition.grid.dims.y *
            custom.definition.grid.cellSize;
        custom.definition.grid.worldPos = {
            point.x + gridWidth / 2.0f,
            point.y + gridHeight
        };
        custom.definition.initialCamera.reset();
        custom.hasGrid = true;
        m_context->customLevels->Save();
    } else if (m_designTool == DesignTool::Goal &&
               IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        custom.definition.goal.position = point;
        custom.hasGoal = true;
        m_context->customLevels->Save();
    }
}

void CustomLevelEditorScene::Render() const {
    if (m_context->activeCustomLevel < 0 ||
        m_context->activeCustomLevel >= static_cast<int>(m_context->customLevels->GetLevels().size())) return;
    const CustomLevelData& custom = m_context->customLevels->GetLevels()[m_context->activeCustomLevel];

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode2D(m_designCamera);
    DrawGrid(40, 50.0f);
    const Color terrainFill{116, 78, 48, 255};
    const Color terrainOutline{70, 45, 28, 255};
    for (const auto& boundary : Math::SampleTerrain(custom.definition.terrain)) {
        const Math::TerrainMesh mesh = Math::BuildTerrainPolygonMesh(boundary);
        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            DrawTriangle(
                mesh.vertices[mesh.indices[i]].ToRayVec(),
                mesh.vertices[mesh.indices[i + 2]].ToRayVec(),
                mesh.vertices[mesh.indices[i + 1]].ToRayVec(),
                terrainFill
            );
        }
        for (std::size_t i = 0; i < mesh.top.size(); ++i) {
            DrawLineV(
                mesh.top[i].ToRayVec(),
                mesh.top[(i + 1) % mesh.top.size()].ToRayVec(),
                terrainOutline
            );
        }
    }
    for (std::size_t i = 0; i < m_terrainWorkPoints.size(); ++i) {
        const Vector2 point = m_terrainWorkPoints[i].ToRayVec();
        DrawCircleV(point, 5.0f, RED);
        if (i > 0) {
            DrawLineV(m_terrainWorkPoints[i - 1].ToRayVec(), point, RED);
        }
    }

    if (custom.hasGrid || m_designTool == DesignTool::Grid) {
        const float width = custom.definition.grid.dims.x *
            custom.definition.grid.cellSize;
        const float height = custom.definition.grid.dims.y *
            custom.definition.grid.cellSize;
        Math::Vector2D position = custom.definition.grid.worldPos;
        if (m_designTool == DesignTool::Grid && !custom.hasGrid) {
            const Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), m_designCamera);
            position = {mouse.x + width / 2.0f, mouse.y + height};
        }
        const float left = static_cast<float>(position.x) - width / 2.0f;
        const float top = static_cast<float>(position.y) - height;
        DrawRectangle(
            static_cast<int>(left),
            static_cast<int>(top),
            static_cast<int>(width),
            static_cast<int>(height),
            Fade(SKYBLUE, 0.18f)
        );
        DrawRectangleLinesEx(Rectangle{left, top, width, height}, 2.0f, BLUE);
        for (int x = 1; x < custom.definition.grid.dims.x; ++x) {
            DrawLine(
                static_cast<int>(left + x * custom.definition.grid.cellSize),
                static_cast<int>(top),
                static_cast<int>(left + x * custom.definition.grid.cellSize),
                static_cast<int>(top + height),
                Fade(BLUE, 0.65f)
            );
        }
        for (int y = 1; y < custom.definition.grid.dims.y; ++y) {
            DrawLine(
                static_cast<int>(left),
                static_cast<int>(top + y * custom.definition.grid.cellSize),
                static_cast<int>(left + width),
                static_cast<int>(top + y * custom.definition.grid.cellSize),
                Fade(BLUE, 0.65f)
            );
        }
    }

    if (custom.hasGoal || m_designTool == DesignTool::Goal) {
        Math::Vector2D goal = custom.definition.goal.position;
        if (m_designTool == DesignTool::Goal && !custom.hasGoal) {
            const Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), m_designCamera);
            goal = {mouse.x, mouse.y};
        }
        const Vector2 pole{static_cast<float>(goal.x), static_cast<float>(goal.y)};
        DrawLineEx({pole.x, pole.y - 80.0f}, pole, 4.0f, DARKGRAY);
        DrawTriangle(
            {pole.x, pole.y - 80.0f},
            {pole.x, pole.y - 50.0f},
            {pole.x + 55.0f, pole.y - 65.0f},
            RED
        );
    }
    EndMode2D();

    const Rectangle terrainButton{20.0f, 16.0f, 120.0f, 36.0f};
    const Rectangle gridButton{146.0f, 16.0f, 120.0f, 36.0f};
    const Rectangle goalButton{272.0f, 16.0f, 100.0f, 36.0f};
    const Rectangle editButton{
        static_cast<float>(m_context->windowWidth - 132), 16.0f, 112.0f, 36.0f
    };
    UiButton::Draw(
        terrainButton,
        "Terreno",
        m_designTool == DesignTool::Terrain ? SKYBLUE : LIGHTGRAY
    );
    UiButton::Draw(
        gridButton,
        "Grid inicial",
        m_designTool == DesignTool::Grid ? SKYBLUE : LIGHTGRAY
    );
    UiButton::Draw(
        goalButton,
        "Meta",
        m_designTool == DesignTool::Goal ? SKYBLUE : LIGHTGRAY
    );
    const bool ready = custom.hasTerrain && custom.hasGrid && custom.hasGoal;
    UiButton::Draw(editButton, "EDIT", ready ? SKYBLUE : GRAY);

    const char* instructions = m_designTool == DesignTool::Terrain
        ? "Clic izquierdo: puntos del terreno | clic derecho: cerrar"
        : m_designTool == DesignTool::Grid
        ? "Haz clic donde comenzará la esquina superior izquierda del grid"
        : "Haz clic para colocar la meta";
    DrawText(instructions, 20, 64, 16, DARKGRAY);
    DrawText(
        TextFormat(
            "Terreno %s   Grid %s   Meta %s",
            custom.hasTerrain ? "OK" : "-",
            custom.hasGrid ? "OK" : "-",
            custom.hasGoal ? "OK" : "-"
        ),
        20,
        m_context->windowHeight - 28,
        16,
        ready ? DARKGREEN : DARKGRAY
    );
    UiButton::Draw(
        UiButton::CenteredAtBottom(
            m_context->windowWidth,
            m_context->windowHeight,
            110.0f,
            38.0f,
            18.0f
        ),
        "Lista"
    );
    EndDrawing();
}
