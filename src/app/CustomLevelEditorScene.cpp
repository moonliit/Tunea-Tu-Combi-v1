#include "app/CustomLevelEditorScene.hpp"
#include "app/AppContext.hpp"
#include "app/CustomLevelCatalog.hpp"
#include "app/CustomLevelData.hpp"
#include "app/SceneManager.hpp"
#include "app/AudioController.hpp"
#include "app/CustomMusicCatalog.hpp"
#include "app/game/Level.hpp"
#include "app/SceneManager.hpp"
#include "ui/UiButton.hpp"
#include <raylib.h>
#include <algorithm>
#include <cmath>
#include <optional>
#include <random>
#include <utility>
#include <print>

namespace {

float PassengerEditorScale(float zoom) {
    return std::clamp(1.5f / zoom, 0.9f, 3.0f);
}

void LocateMusic(const std::string& path, int& categoryIndex, int& trackIndex) {
    const auto& categories = GetCustomMusicCategories();
    for (std::size_t category = 0; category < categories.size(); ++category) {
        for (std::size_t track = 0; track < categories[category].tracks.size(); ++track) {
            if (categories[category].tracks[track].path == path) {
                categoryIndex = static_cast<int>(category);
                trackIndex = static_cast<int>(track);
                return;
            }
        }
    }
}

bool IsInsidePolygon(
    const Math::Vector2D& point,
    const std::vector<Math::Vector2D>& polygon
) {
    bool inside = false;
    for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const auto& a = polygon[i];
        const auto& b = polygon[j];
        if ((a.y > point.y) != (b.y > point.y) &&
            point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x) {
            inside = !inside;
        }
    }
    return inside;
}

std::optional<float> FindTerrainSurfaceY(
    const Math::TerrainDefinition& terrain,
    float x,
    float nearY
) {
    std::optional<float> surfaceY;
    for (const auto& boundary : Math::SampleTerrain(terrain)) {
        for (std::size_t i = 0; i < boundary.size(); ++i) {
            const auto& first = boundary[i];
            const auto& second = boundary[(i + 1) % boundary.size()];
            const float x1 = static_cast<float>(first.x);
            const float y1 = static_cast<float>(first.y);
            const float x2 = static_cast<float>(second.x);
            const float y2 = static_cast<float>(second.y);
            if (x < std::min(x1, x2) || x > std::max(x1, x2)) continue;

            float intersectionY;
            if (std::abs(x2 - x1) < 0.0001f) {
                intersectionY = std::min(y1, y2);
            } else {
                const float t = (x - x1) / (x2 - x1);
                intersectionY = y1 + t * (y2 - y1);
            }
            if (!surfaceY.has_value() ||
                std::abs(intersectionY - nearY) < std::abs(surfaceY.value() - nearY)) {
                surfaceY = intersectionY;
            }
        }
    }
    return surfaceY;
}

}

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
    if (m_context->openCustomMusicPicker) {
        m_context->openCustomMusicPicker = false;
        m_musicPickerOpen = true;
        LocateMusic(custom.musicPath, m_musicCategory, m_musicTrack);
    }

    const Rectangle terrainButton{20.0f, 16.0f, 120.0f, 36.0f};
    const Rectangle gridButton{146.0f, 16.0f, 120.0f, 36.0f};
    const Rectangle goalButton{272.0f, 16.0f, 100.0f, 36.0f};
    const Rectangle musicButton{382.0f, 16.0f, 112.0f, 36.0f};
    const Rectangle eraserButton{504.0f, 16.0f, 120.0f, 36.0f};
    const Rectangle passengerButton{632.0f, 16.0f, 112.0f, 36.0f};
    const Rectangle menuButton{
        static_cast<float>(m_context->windowWidth - 128), 16.0f, 110.0f, 38.0f
    };
    const Rectangle editButton = UiButton::PlaySideButton(
        m_context->windowWidth, m_context->windowHeight
    );

    if (!m_musicPickerOpen && UiButton::WasClicked(terrainButton)) {
        m_designTool = DesignTool::Terrain;
        return;
    }
    if (!m_musicPickerOpen && UiButton::WasClicked(gridButton)) {
        m_designTool = DesignTool::Grid;
        return;
    }
    if (!m_musicPickerOpen && UiButton::WasClicked(goalButton)) {
        m_designTool = DesignTool::Goal;
        return;
    }
    if (!m_musicPickerOpen && UiButton::WasClicked(musicButton)) {
        m_musicCategory = 0;
        m_musicTrack = 0;
        LocateMusic(custom.musicPath, m_musicCategory, m_musicTrack);
        m_musicListScroll = 0.0f;
        m_musicPickerOpen = true;
        m_context->audioController->StartLevelEditorMusic(custom.musicPath, true);
        return;
    }
    if (!m_musicPickerOpen && UiButton::WasClicked(eraserButton)) {
        m_designTool = DesignTool::Eraser;
        return;
    }
    if (!m_musicPickerOpen && UiButton::WasClicked(passengerButton)) {
        m_designTool = DesignTool::Passenger;
        return;
    }
    if (m_musicPickerOpen) {
        const auto& categories = GetCustomMusicCategories();
        const float panelX = (m_context->windowWidth - 700.0f) / 2.0f;
        const float panelY = (m_context->windowHeight - 540.0f) / 2.0f;
        for (std::size_t i = 0; i < categories.size(); ++i) {
            const Rectangle tab{panelX + 18.0f + (i % 4) * 164.0f,
                panelY + 50.0f + (i / 4) * 36.0f, 158.0f, 30.0f};
            if (UiButton::WasClicked(tab)) {
                m_musicCategory = static_cast<int>(i);
                m_musicTrack = 0;
                m_musicListScroll = 0.0f;
                return;
            }
        }
        const auto& tracks = categories[m_musicCategory].tracks;
        const Rectangle list{panelX + 18.0f, panelY + 164.0f, 664.0f, 300.0f};
        m_musicListScroll = std::clamp(m_musicListScroll - GetMouseWheelMove() * 36.0f,
            0.0f, std::max(0.0f, tracks.size() * 52.0f - list.height));
        for (std::size_t i = 0; i < tracks.size(); ++i) {
            const float y = list.y + i * 52.0f - m_musicListScroll;
            if (y < list.y || y + 46.0f > list.y + list.height) continue;
            if (UiButton::WasClicked(Rectangle{list.x, y, list.width, 46.0f})) {
                m_musicTrack = static_cast<int>(i);
                // m_context->audioController->SetCurrentScenario({Scenario::LevelEditorMusicPreview, 0, tracks[i].path});
                m_context->audioController->StartLevelEditorMusic(tracks[i].path, true);
                return;
            }
        }
        const Rectangle confirm{panelX + 372.0f, panelY + 484.0f, 150.0f, 38.0f};
        const Rectangle cancel{panelX + 532.0f, panelY + 484.0f, 150.0f, 38.0f};
        if (UiButton::WasClicked(confirm)) {
            const auto& track = tracks[m_musicTrack];
            custom.musicPath = track.path;
            custom.musicTitle = track.title;
            custom.musicArtist = track.artist;
            m_context->customLevels->Save();
            // m_context->audioController->SetCurrentScenario({Scenario::LevelEditorBuild, 0, track.path});
            m_context->audioController->StartLevelEditorMusic(custom.musicPath, false);
            m_musicPickerOpen = false;
            return;
        }
        if (UiButton::WasClicked(cancel)) {
            auto scenario = m_context->audioController->GetCurrentScenario();
            // m_context->audioController->SetCurrentScenario({Scenario::LevelEditorBuild, 0, scenario.customMusicPath});
            m_context->audioController->StartLevelEditorMusic(custom.musicPath, false);
            m_musicPickerOpen = false;
            return;
        }
        return;
    }
    if (UiButton::WasClicked(menuButton)) {
        m_context->audioController->PlayUiClick();
        m_context->audioController->ReturnToSelector();
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
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
        m_designTool != DesignTool::Eraser) {
        m_lastEraserMouseScreen.reset();
        m_erasedPassengerPositions.clear();
    }
    if (screenMouse.y < 65.0f || screenMouse.y > m_context->windowHeight - 58.0f) {
        return;
    }
    const Vector2 worldMouse = GetScreenToWorld2D(screenMouse, m_designCamera);
    const Math::Vector2D point{worldMouse.x, worldMouse.y};

    if (m_designTool == DesignTool::Eraser && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        const Vector2 start = m_lastEraserMouseScreen.value_or(screenMouse);
        const float distance = std::hypot(screenMouse.x - start.x, screenMouse.y - start.y);
        const int steps = std::max(1, static_cast<int>(std::ceil(distance / 8.0f)));
        const float scale = PassengerEditorScale(m_designCamera.zoom);
        bool changed = false;
        bool terrainChanged = false;
        for (int step = 0; step <= steps; ++step) {
            const float t = static_cast<float>(step) / steps;
            const Vector2 sampleScreen{
                start.x + (screenMouse.x - start.x) * t,
                start.y + (screenMouse.y - start.y) * t
            };
            if (sampleScreen.y < 65.0f ||
                sampleScreen.y > m_context->windowHeight - 58.0f) continue;
            const Vector2 sampleWorld = GetScreenToWorld2D(sampleScreen, m_designCamera);
            const Math::Vector2D samplePoint{sampleWorld.x, sampleWorld.y};
            const auto hitsPassenger = [&](const Math::Vector2D& position) {
                const Vector2 feet = GetWorldToScreen2D(
                    {static_cast<float>(position.x), static_cast<float>(position.y)},
                    m_designCamera
                );
                const float dx = std::abs(sampleScreen.x - feet.x);
                const float dy = sampleScreen.y - feet.y;
                return dx <= 12.0f * scale * m_designCamera.zoom + 8.0f &&
                    dy >= -28.0f * scale * m_designCamera.zoom - 8.0f &&
                    dy <= 10.0f * scale * m_designCamera.zoom + 8.0f;
            };
            bool overPassenger = std::any_of(
                m_erasedPassengerPositions.begin(), m_erasedPassengerPositions.end(),
                hitsPassenger
            );
            for (std::size_t i = 0; i < custom.definition.passengers.size();) {
                if (!hitsPassenger(custom.definition.passengers[i].position)) {
                    ++i;
                    continue;
                }
                m_erasedPassengerPositions.push_back(custom.definition.passengers[i].position);
                custom.definition.passengers.erase(custom.definition.passengers.begin() + i);
                overPassenger = true;
                changed = true;
            }
            if (overPassenger) continue;
            const auto boundaries = Math::SampleTerrain(custom.definition.terrain);
            for (std::size_t i = 0; i < boundaries.size(); ++i) {
                if (!IsInsidePolygon(samplePoint, boundaries[i])) continue;
                custom.definition.terrain.polygons.erase(
                    custom.definition.terrain.polygons.begin() + i
                );
                terrainChanged = true;
                changed = true;
                break;
            }
        }
        m_lastEraserMouseScreen = screenMouse;
        if (terrainChanged) {
            custom.hasTerrain = !custom.definition.terrain.polygons.empty();
            if (custom.hasGoal) {
                const auto surfaceY = FindTerrainSurfaceY(
                    custom.definition.terrain,
                    static_cast<float>(custom.definition.goal.position.x),
                    static_cast<float>(custom.definition.goal.position.y)
                );
                if (surfaceY.has_value()) {
                    custom.definition.goal.position.y = surfaceY.value();
                } else {
                    custom.hasGoal = false;
                }
            }
        }
        if (changed) m_context->customLevels->Save();
        return;
    }

    if (m_designTool == DesignTool::Passenger &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const auto surfaceY = FindTerrainSurfaceY(
            custom.definition.terrain,
            static_cast<float>(point.x),
            static_cast<float>(point.y)
        );
        if (surfaceY.has_value()) {
            static std::mt19937 randomEngine{std::random_device{}()};
            static std::bernoulli_distribution skirtChance(0.5);
            custom.definition.passengers.push_back({
                .position = {point.x, surfaceY.value()},
                .hasSkirt = skirtChance(randomEngine)
            });
            m_context->customLevels->Save();
        }
        return;
    }

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
        if (custom.hasGoal) {
            const auto surfaceY = FindTerrainSurfaceY(
                custom.definition.terrain,
                static_cast<float>(custom.definition.goal.position.x),
                static_cast<float>(custom.definition.goal.position.y)
            );
            if (surfaceY.has_value()) {
                custom.definition.goal.position.y = surfaceY.value();
            }
        }
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
        const auto surfaceY = FindTerrainSurfaceY(
            custom.definition.terrain,
            static_cast<float>(point.x),
            static_cast<float>(point.y)
        );
        if (surfaceY.has_value()) {
            custom.definition.goal.position = {point.x, surfaceY.value()};
            custom.hasGoal = true;
            m_context->customLevels->Save();
        }
    }
}

void CustomLevelEditorScene::Render() const {
    if (m_context->activeCustomLevel < 0 ||
        m_context->activeCustomLevel >= static_cast<int>(m_context->customLevels->GetLevels().size())) return;
    const CustomLevelData& custom = m_context->customLevels->GetLevels()[m_context->activeCustomLevel];

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode2D(m_designCamera);
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
    for (const auto& passenger : custom.definition.passengers) {
        const float x = static_cast<float>(passenger.position.x);
        const float y = static_cast<float>(passenger.position.y);
        const float scale = PassengerEditorScale(m_designCamera.zoom);
        DrawCircleV({x, y - 19.0f * scale}, 6.0f * scale, BLACK);
        DrawLineEx({x, y - 13.0f * scale}, {x, y - 2.0f * scale}, 3.0f * scale, BLACK);
        DrawLineEx({x, y - 9.0f * scale}, {x - 7.0f * scale, y - 4.0f * scale}, 2.0f * scale, BLACK);
        DrawLineEx({x, y - 9.0f * scale}, {x + 7.0f * scale, y - 4.0f * scale}, 2.0f * scale, BLACK);
        if (passenger.hasSkirt) {
            DrawTriangle({x - 5.0f * scale, y - 2.0f * scale},
                {x + 5.0f * scale, y - 2.0f * scale},
                {x, y + 4.0f * scale}, PINK);
        }
        DrawLineEx({x, y - 2.0f * scale}, {x - 4.0f * scale, y + 5.0f * scale}, 2.0f * scale, BLACK);
        DrawLineEx({x, y - 2.0f * scale}, {x + 4.0f * scale, y + 5.0f * scale}, 2.0f * scale, BLACK);
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
    const Rectangle musicButton{382.0f, 16.0f, 112.0f, 36.0f};
    const Rectangle eraserButton{504.0f, 16.0f, 120.0f, 36.0f};
    const Rectangle passengerButton{632.0f, 16.0f, 112.0f, 36.0f};
    const Rectangle menuButton{
        static_cast<float>(m_context->windowWidth - 128), 16.0f, 110.0f, 38.0f
    };
    UiButton::Draw(
        terrainButton,
        "Terreno",
        m_designTool == DesignTool::Terrain ? SKYBLUE : LIGHTGRAY
    );
    UiButton::Draw(
        gridButton,
        "Grid inicial",
        custom.hasGrid ? LIGHTGRAY : RED
    );
    UiButton::Draw(
        goalButton,
        "Meta",
        custom.hasGoal ? LIGHTGRAY : RED
    );
    if (!custom.hasGrid) {
        const char* required = "¡Obligatorio!";
        DrawText(required,
            static_cast<int>(gridButton.x + (gridButton.width - MeasureText(required, 12)) / 2),
            static_cast<int>(gridButton.y + gridButton.height + 2.0f), 12, RED);
    }
    if (!custom.hasGoal) {
        const char* required = "¡Obligatorio!";
        DrawText(required,
            static_cast<int>(goalButton.x + (goalButton.width - MeasureText(required, 12)) / 2),
            static_cast<int>(goalButton.y + goalButton.height + 2.0f), 12, RED);
    }
    UiButton::Draw(musicButton, "Música", SKYBLUE);
    UiButton::Draw(passengerButton, "Pasajero",
        m_designTool == DesignTool::Passenger ? SKYBLUE : LIGHTGRAY);
    UiButton::Draw(
        eraserButton,
        "Borrador",
        m_designTool == DesignTool::Eraser ? SKYBLUE : LIGHTGRAY
    );
    const bool ready = custom.hasTerrain && custom.hasGrid && custom.hasGoal;
    UiButton::Draw(menuButton, "MENÚ", LIGHTGRAY);
    const Rectangle editButton = UiButton::PlaySideButton(
        m_context->windowWidth, m_context->windowHeight
    );
    DrawRectangleRec(editButton, ready ? DARKGREEN : GRAY);
    DrawRectangleLinesEx(editButton, 1.0f, DARKGRAY);
    const char* editLabel = "EDITAR";
    const char* combiLabel = "COMBI";
    DrawText(
        editLabel,
        static_cast<int>(editButton.x + (editButton.width - MeasureText(editLabel, 20)) / 2),
        static_cast<int>(editButton.y + 22),
        20,
        WHITE
    );
    DrawText(
        combiLabel,
        static_cast<int>(editButton.x + (editButton.width - MeasureText(combiLabel, 16)) / 2),
        static_cast<int>(editButton.y + 50),
        16,
        WHITE
    );
    const Rectangle enterBadge{
        editButton.x + editButton.width - 42.0f,
        editButton.y + 4.0f,
        38.0f,
        16.0f
    };
    DrawRectangleRec(enterBadge, Fade(BLACK, 0.65f));
    DrawText(
        "ENTER",
        static_cast<int>(enterBadge.x + 3.0f),
        static_cast<int>(enterBadge.y + 2.0f),
        11,
        WHITE
    );

    const char* instructions = m_designTool == DesignTool::Terrain
        ? "Clic izquierdo: puntos del terreno | clic derecho: cerrar"
        : m_designTool == DesignTool::Grid
        ? "Haz clic donde comenzará la esquina superior izquierda del grid"
        : m_designTool == DesignTool::Eraser
        ? "Haz clic dentro de una forma de terreno para borrarla"
        : m_designTool == DesignTool::Passenger
        ? "Haz clic sobre el terreno para colocar un pasajero"
        : "Haz clic sobre el terreno para colocar la meta en su superficie";
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
    if (m_musicPickerOpen) {
        const auto& categories = GetCustomMusicCategories();
        const auto& tracks = categories[m_musicCategory].tracks;
        const float panelX = (m_context->windowWidth - 700.0f) / 2.0f;
        const float panelY = (m_context->windowHeight - 540.0f) / 2.0f;
        DrawRectangle(0, 0, m_context->windowWidth, m_context->windowHeight, Fade(BLACK, 0.5f));
        const Rectangle panel{panelX, panelY, 700.0f, 540.0f};
        DrawRectangleRec(panel, RAYWHITE);
        DrawRectangleLinesEx(panel, 2.0f, DARKGRAY);
        DrawText("Música del nivel", static_cast<int>(panelX + 18), static_cast<int>(panelY + 18), 22, BLACK);
        for (std::size_t i = 0; i < categories.size(); ++i) {
            const Rectangle tab{panelX + 18.0f + (i % 4) * 164.0f,
                panelY + 50.0f + (i / 4) * 36.0f, 158.0f, 30.0f};
            UiButton::Draw(tab, categories[i].name.c_str(),
                static_cast<int>(i) == m_musicCategory ? SKYBLUE : LIGHTGRAY, BLACK, 13);
        }
        const Rectangle list{panelX + 18.0f, panelY + 164.0f, 664.0f, 300.0f};
        DrawRectangleRec(list, RAYWHITE);
        for (std::size_t i = 0; i < tracks.size(); ++i) {
            const float y = list.y + i * 52.0f - m_musicListScroll;
            if (y < list.y || y + 46.0f > list.y + list.height) continue;
            const Rectangle row{list.x, y, list.width, 46.0f};
            DrawRectangleRec(row, static_cast<int>(i) == m_musicTrack ? Fade(SKYBLUE, 0.45f) : Fade(LIGHTGRAY, 0.35f));
            const std::string label = tracks[i].artist.empty()
                ? tracks[i].title
                : tracks[i].title + " - " + tracks[i].artist;
            DrawText(label.c_str(), static_cast<int>(row.x + 12), static_cast<int>(row.y + 13), 16, BLACK);
        }
        UiButton::Draw(Rectangle{panelX + 372.0f, panelY + 484.0f, 150.0f, 38.0f}, "Confirmar", SKYBLUE);
        UiButton::Draw(Rectangle{panelX + 532.0f, panelY + 484.0f, 150.0f, 38.0f}, "Cancelar", LIGHTGRAY);
    }
    EndDrawing();
}
