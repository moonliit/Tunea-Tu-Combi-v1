#include "app/editor/EditorScene.hpp"
#include "app/game/Level.hpp"
#include "app/AppContext.hpp"
#include <fstream>
#include "imgui.h"
#include "raylib.h"
#include "rlImGui.h"
#include <raymath.h>
#include <stdio.h>
using namespace Math;

EditorScene::EditorScene(AppContext* appContext):
    m_appContext(appContext),
    m_currentTool(EditorTool::PlacePoint),
    m_activeSegmentType(TerrainSegmentType::Line)
{
    rlImGuiSetup(true);
    m_camera = { 0 };
    m_camera.zoom = 1.0f;
    m_camera.offset = {
        (float)m_appContext->windowWidth / 2.0f,
        (float)m_appContext->windowHeight / 2.0f
    };
    m_camera.target = { 0.0f, 0.0f };

    // init filename buffer with a default name
    std::strcpy(m_filenameBuffer, "level_1");
}

EditorScene::~EditorScene() {
    rlImGuiShutdown();
}

void EditorScene::ProcessInput(float deltaTime) {
    HandleCameraInput(deltaTime);
    HandleMouseInput();
}

void EditorScene::HandleCameraInput(float deltaTime) {
    // WASD camera panning
    float panSpeed = 400.0f / m_camera.zoom;
    if (IsKeyDown(KEY_W)) m_camera.target.y -= panSpeed * deltaTime;
    if (IsKeyDown(KEY_S)) m_camera.target.y += panSpeed * deltaTime;
    if (IsKeyDown(KEY_A)) m_camera.target.x -= panSpeed * deltaTime;
    if (IsKeyDown(KEY_D)) m_camera.target.x += panSpeed * deltaTime;

    // zooming with mouse wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        // zoom relative to mouse position
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), m_camera);
        m_camera.offset = GetMousePosition();
        m_camera.target = mouseWorldPos;

        m_camera.zoom += wheel * 0.1f;
        if (m_camera.zoom < 0.1f) m_camera.zoom = 0.1f;
    }
}

void EditorScene::HandleMouseInput() {
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), m_camera);
        m_currentPolygonWorkPoints.push_back(WorkingSegmentPoint{
            .point = {mousePos.x, mousePos.y},
            .type = m_activeSegmentType
        });
    }

    // right click to finish the polygon loop
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && m_currentPolygonWorkPoints.size() >= 3) {
        Math::TerrainPolygon poly;
        size_t n = m_currentPolygonWorkPoints.size();
        size_t i = 0;

        while (i < n) {
            TerrainSegmentType segType = m_currentPolygonWorkPoints[i].type;
            std::vector<Math::Vector2D> segPoints;

            segPoints.push_back(m_currentPolygonWorkPoints[i].point);

            size_t j = i + 1;
            // consume subsequent points as long as they share the same segment type
            while (j < n && m_currentPolygonWorkPoints[j].type == segType) {
                segPoints.push_back(m_currentPolygonWorkPoints[j].point);
                j++;
            }

            if (j < n) {
                // we hit a type change at index j
                // include this junction point to cleanly end the current segment,
                // and let the next segment start at this exact same point (sharing c1 == p1).
                segPoints.push_back(m_currentPolygonWorkPoints[j].point);
                poly.segments.push_back(Math::TerrainSegment{
                    .type = segType,
                    .points = segPoints,
                    .resolution = (segType == TerrainSegmentType::CatmullRom) ? 12 : 1
                });
                i = j; // next segment picks up right at index j
            } else {
                segPoints.push_back(m_currentPolygonWorkPoints[0].point);
                poly.segments.push_back(Math::TerrainSegment{
                    .type = segType,
                    .points = segPoints,
                    .resolution = (segType == TerrainSegmentType::CatmullRom) ? 12 : 1
                });
                break;
            }
        }

        m_completedPolygons.push_back(poly);
        m_currentPolygonWorkPoints.clear();
    }
}

void EditorScene::Update(float deltaTime) {
    // Any per-frame updates
}

void EditorScene::Render() const {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode2D(m_camera);
    DrawGrid(40, 50.0f);

    // draw completed polygons
    for (const auto& poly : m_completedPolygons) {
        Math::TerrainDefinition def{ .polygons = { poly } };
        auto sampledSections = Math::SampleTerrain(def);

        // draw the smoothly evaluated terrain sections
        for (const auto& section : sampledSections) {
            for (size_t i = 0; i < section.size(); ++i) {
                Vector2 p0 = section[i].ToRayVec();
                Vector2 p1 = section[(i + 1) % section.size()].ToRayVec();

                DrawLineV(p0, p1, BLUE);
                DrawCircleV(p0, 3.0f, DARKBLUE);
            }
        }
    }

    // draw active polygon being built
    for (size_t i = 0; i < m_currentPolygonWorkPoints.size(); ++i) {
        auto curr = m_currentPolygonWorkPoints[i];
        Vector2 p0 = curr.point.ToRayVec();
        Color currentColor = (curr.type == TerrainSegmentType::Line) ? RED : BLUE;
        DrawCircleV(p0, 5.0f, currentColor);
        if (i > 0) {
            auto prev = m_currentPolygonWorkPoints[i - 1];
            Vector2 p1 = prev.point.ToRayVec();
            DrawLineV(p1, p0, currentColor);
        }
    }

    EndMode2D();

    // imgui for the debug drawing of controls
    rlImGuiBegin();

    ImGui::Begin("Terrain Editor Controls");
    ImGui::Text("WASD to Pan | Mouse Wheel to Zoom");
    ImGui::Text("Left Click: Add Point | Right Click: Close Polygon");
    ImGui::Separator();

    ImGui::Text("Points in current polygon: %zu", m_currentPolygonWorkPoints.size());

    if (ImGui::Button("Clear Current Points")) {
        m_currentPolygonWorkPoints.clear();
    }

    // segment type selector (Line vs CatmullRom)
    const char* segmentItems[] = { "Line", "CatmullRom" };
    int currentItem = (m_activeSegmentType == TerrainSegmentType::Line) ? 0 : 1;
    if (ImGui::Combo("Next Segment Type", &currentItem, segmentItems, 2)) {
        m_activeSegmentType = (currentItem == 0) ?
            TerrainSegmentType::Line :
            TerrainSegmentType::CatmullRom;
    }

    ImGui::Separator();
    ImGui::Text("Completed Polygons: %zu", m_completedPolygons.size());

    // text input for file naming
    ImGui::InputText("Filename", m_filenameBuffer, sizeof(m_filenameBuffer));
    ImGui::SameLine();
    ImGui::Text(".json");

    if (ImGui::Button("Export LevelDefinition to JSON")) {
        // build path: assets/levels/<filename>.json
        std::string fileName = m_filenameBuffer;
        if (fileName.empty()) {
            fileName = "unnamed_level";
        }
        std::string fullPath = "assets/levels/" + fileName + ".json";

        // build terrain definition polygons from completed editor data
        Math::TerrainDefinition terrainDef;
        for (const auto& poly : m_completedPolygons) {
            terrainDef.polygons.push_back(poly);
        }

        // populate the level definition object cleanly
        LevelDefinition levelDef{
            .terrain = terrainDef,
            .grid = GridDefinition{
                .dims = {6, 4},
                .worldPos = {-50.0f, 480.0f},
                .cellSize = 40.0f
            },
            .initialCamera = CameraDefinition{
                .target = {300.0f, 360.0f},
                .zoom = 0.72f
            },
            .goal = GoalDefinition{
                .position = {900.0f, 430.0f}
            }
        };

        // convert level definition object to json
        nlohmann::json j = levelDef.ToJson();

        // write to disk using the custom path
        std::ofstream file(fullPath);
        if (file.is_open()) {
            file << j.dump(4);
            file.close();
        }
    }

    ImGui::End();

    rlImGuiEnd();
    EndDrawing();
}
