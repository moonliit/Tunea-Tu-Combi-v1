#include "app/CustomLevelEditorScene.hpp"
#include "app/AppContext.hpp"
#include "app/CustomLevelCatalog.hpp"
#include "app/CustomLevelData.hpp"
#include "app/SceneManager.hpp"
#include "app/AudioController.hpp"
#include "app/CustomMusicCatalog.hpp"
#include "app/game/Level.hpp"
#include "rendering/TextureLibrary.hpp"
#include "rendering/PassengerRenderer.hpp"
#include "math/Spline.hpp"
#include "ui/UiButton.hpp"
#include <raylib.h>
#include <algorithm>
#include <cmath>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <print>

namespace {

float PassengerEditorScale(float zoom) {
    return std::clamp(1.5f / zoom, 0.9f, 3.0f);
}

float GoalCenterY(float surfaceY) {
    return surfaceY - static_cast<float>(GoalSize.y) / 2.0f;
}

constexpr int MaxGridColumns = 10;
constexpr int MaxGridRows = 8;

GridDefinition PreviewGrid(
    const Math::Vector2D& anchor,
    const Math::Vector2D& cursor,
    float cellSize
) {
    const int columnOffset = std::clamp(
        static_cast<int>(std::floor((cursor.x - anchor.x) / cellSize)),
        1 - MaxGridColumns, MaxGridColumns - 1
    );
    const int rowOffset = std::clamp(
        static_cast<int>(std::floor((cursor.y - anchor.y) / cellSize)),
        1 - MaxGridRows, MaxGridRows - 1
    );
    const int columns = std::abs(columnOffset) + 1;
    const int rows = std::abs(rowOffset) + 1;
    const double left = anchor.x + std::min(0, columnOffset) * cellSize;
    const double top = anchor.y + std::min(0, rowOffset) * cellSize;
    return GridDefinition{
        .dims = {columns, rows},
        .worldPos = {
            left + columns * cellSize / 2.0,
            top + rows * cellSize
        },
        .cellSize = cellSize
    };
}

Rectangle EditorCombiButton(int width, int height) {
    return Rectangle{
        static_cast<float>(width - 190),
        (height - 90.0f) / 2.0f,
        170.0f,
        90.0f
    };
}

Rectangle EditorToolbarButton(int index) {
    return Rectangle{20.0f + index * 100.0f, 14.0f, 94.0f, 78.0f};
}

Rectangle LevelEditorMenuButton(int width) {
    return Rectangle{static_cast<float>(width - 210), 20.0f,
        190.0f, 66.0f};
}

Rectangle LevelEditorPublishButton(int width) {
    const Rectangle menu = LevelEditorMenuButton(width);
    return Rectangle{menu.x - 132.0f, menu.y, 120.0f, menu.height};
}

Rectangle GraphicModeButton(int width) {
    const Rectangle publish = LevelEditorPublishButton(width);
    return Rectangle{publish.x - 130.0f, publish.y,
        publish.width, publish.height};
}

Rectangle TerrainCurveButton(int width, int height) {
    return Rectangle{static_cast<float>(width - 590),
        static_cast<float>(height - 48), 270.0f, 38.0f};
}

Math::TerrainPolygon BuildTerrainPolygon(
    const std::vector<Math::Vector2D>& points,
    const std::vector<bool>& curveEdges
) {
    Math::TerrainPolygon polygon;
    if (points.size() < 3) return polygon;
    std::vector<Math::Vector2D> closed = points;
    closed.push_back(points.front());
    for (std::size_t begin = 0; begin < points.size();) {
        const bool curved = begin < curveEdges.size() && curveEdges[begin];
        std::size_t end = begin + 1;
        while (end < points.size() &&
               (end < curveEdges.size() && curveEdges[end]) == curved) {
            ++end;
        }
        Math::TerrainSegment segment;
        segment.type = curved ? Math::TerrainSegmentType::CatmullRom
                              : Math::TerrainSegmentType::Line;
        segment.resolution = curved ? 12 : 1;
        segment.points.assign(closed.begin() + begin, closed.begin() + end + 1);
        polygon.segments.push_back(std::move(segment));
        begin = end;
    }
    return polygon;
}

Math::PolygonTriangulation ValidateTerrainDraft(
    const std::vector<Math::Vector2D>& points,
    const std::vector<bool>& curveEdges
) {
    if (points.size() < 3) return Math::TriangulateTerrainPolygon(points);
    Math::TerrainDefinition terrain;
    terrain.polygons.push_back(BuildTerrainPolygon(points, curveEdges));
    auto boundary = Math::SampleTerrain(terrain).front();
    if (boundary.front() == boundary.back()) boundary.pop_back();
    return Math::TriangulateTerrainPolygon(boundary);
}

void DrawEditorToolButton(
    Rectangle bounds,
    int icon,
    bool selected,
    bool missing,
    bool completed,
    const TextureLibrary& textures
) {
    const UiTextures& ui = textures.GetUi();
    UiButton::DrawTextured(bounds, ui.game.toolbarSlot);
    const bool mandatory = icon == 1 || icon == 2;
    if (missing) {
        DrawRectangleRec(Rectangle{bounds.x + 4, bounds.y + 4,
            bounds.width - 8, bounds.height - 8}, Fade(RED, 0.22f));
    }
    if (mandatory) DrawRectangleLinesEx(bounds, missing ? 3.0f : 2.0f, RED);
    if (selected) DrawRectangleRec(Rectangle{
        bounds.x + 4, bounds.y + 4,
        bounds.width - 8, bounds.height - 8
    }, Fade(WHITE, 0.18f));

    const float cx = bounds.x + bounds.width / 2.0f;
    const float top = bounds.y + 6.0f;
    switch (icon) {
        case 0: {
            const Texture2D& dirt = textures.GetTerrainTexture();
            DrawTexturePro(dirt,
                Rectangle{0, 0, static_cast<float>(dirt.width),
                    static_cast<float>(dirt.height)},
                Rectangle{cx - 26, top + 4, 52, 42},
                Vector2{}, 0, WHITE);
            DrawRectangleLinesEx(Rectangle{cx - 26, top + 4, 52, 42},
                2, Fade(WHITE, 0.85f));
            break;
        }
        case 1: {
            const Color color = RED;
            DrawRectangleRec(Rectangle{cx - 22, top + 2, 44, 44},
                Fade(color, 0.26f));
            for (int line = 0; line <= 4; ++line) {
                const float offset = line * 11.0f;
                DrawLineEx({cx - 22 + offset, top + 2},
                    {cx - 22 + offset, top + 46}, 2, color);
                DrawLineEx({cx - 22, top + 2 + offset},
                    {cx + 22, top + 2 + offset}, 2, color);
            }
            break;
        }
        case 2: {
            const Texture2D& goal = textures.GetLevelGoalTexture();
            DrawCircleV({cx, top + 25}, 25, Fade(RED, 0.25f));
            DrawTexturePro(goal,
                Rectangle{0, 0, static_cast<float>(goal.width),
                    static_cast<float>(goal.height)},
                Rectangle{cx - 10, top, 20, 49},
                Vector2{}, 0, WHITE);
            break;
        }
        case 3:
            DrawLineEx({cx + 12, top + 5}, {cx + 12, top + 34},
                5, SKYBLUE);
            DrawLineEx({cx - 5, top + 10}, {cx - 5, top + 40},
                5, SKYBLUE);
            DrawLineEx({cx - 5, top + 10}, {cx + 12, top + 5},
                5, SKYBLUE);
            DrawEllipse(static_cast<int>(cx - 11), static_cast<int>(top + 39),
                9, 6, GOLD);
            DrawEllipse(static_cast<int>(cx + 6), static_cast<int>(top + 33),
                9, 6, GOLD);
            break;
        case 4:
            DrawRectanglePro(Rectangle{cx, top + 24, 30, 38},
                Vector2{15, 19}, 28, Color{237, 18, 121, 255});
            DrawLineEx({cx - 23, top + 43}, {cx + 24, top + 43},
                3, WHITE);
            break;
        case 5:
            DrawCircleV({cx - 12, top + 13}, 8, RAYWHITE);
            DrawCircleV({cx + 13, top + 13}, 8, SKYBLUE);
            DrawLineEx({cx - 12, top + 22}, {cx - 12, top + 39},
                6, RAYWHITE);
            DrawLineEx({cx + 13, top + 22}, {cx + 13, top + 33},
                6, SKYBLUE);
            DrawTriangle({cx + 13, top + 27},
                {cx + 1, top + 43}, {cx + 25, top + 43},
                Color{237, 18, 121, 255});
            DrawLineEx({cx - 20, top + 42}, {cx - 4, top + 42},
                3, WHITE);
            break;
        case 6:
            DrawTexturePro(
                textures.GetPartTexture(PartType::TNTBarrel),
                Rectangle{0, 0,
                    static_cast<float>(textures.GetPartTexture(PartType::TNTBarrel).width),
                    static_cast<float>(textures.GetPartTexture(PartType::TNTBarrel).height)},
                Rectangle{cx - 25, top + 2, 50, 50},
                Vector2{}, 0.0f, WHITE
            );
            break;
    }
    static constexpr const char* labels[]{
        "Terreno", "Cuadrícula", "Meta", "Música", "Borrador", "Pasajero", "SOLGAS"
    };
    const char* label = labels[icon];
    const int fontSize = icon == 1 ? 12 : 13;
    DrawTextFont(label,
        static_cast<int>(cx - MeasureTextFont(label, fontSize) / 2.0f),
        static_cast<int>(bounds.y + bounds.height - 18),
        fontSize, WHITE);
    if (completed) DrawRectangleRec(Rectangle{
        bounds.x + 4, bounds.y + 4,
        bounds.width - 8, bounds.height - 8
    }, Fade(BLACK, 0.38f));
    UiButton::DrawHotkeyNumber(bounds, icon + 1);
}

void DrawEditorTextButton(Rectangle bounds, const char* label,
    const UiButtonSkin& skin, bool selected = false) {
    UiButton::DrawTextured(bounds, skin);
    if (selected) DrawRectangleRec(Rectangle{
        bounds.x + 4, bounds.y + 4,
        bounds.width - 8, bounds.height - 8
    }, Fade(WHITE, 0.18f));
    const int fontSize = 14;
    DrawTextFont(label,
        static_cast<int>(bounds.x + (bounds.width - MeasureTextFont(label, fontSize)) / 2.0f),
        static_cast<int>(bounds.y + (bounds.height - fontSize) / 2.0f),
        fontSize, WHITE);
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

void RepositionTntObstacles(CustomLevelData& custom) {
    const float halfCell = custom.definition.grid.cellSize / 2.0f;
    for (std::size_t i = 0; i < custom.definition.tntObstacles.size();) {
        auto& obstacle = custom.definition.tntObstacles[i];
        const auto surfaceY = FindTerrainSurfaceY(
            custom.definition.terrain,
            static_cast<float>(obstacle.position.x),
            static_cast<float>(obstacle.position.y)
        );
        if (!surfaceY.has_value()) {
            custom.definition.tntObstacles.erase(
                custom.definition.tntObstacles.begin() + i
            );
            continue;
        }
        obstacle.position.y = *surfaceY - halfCell;
        ++i;
    }
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
    if (m_context->focusCustomEditorCamera) {
        m_context->focusCustomEditorCamera = false;
        m_designCamera.zoom = 1.0f;
        if (custom.hasGrid) {
            const GridDefinition& grid = custom.definition.grid;
            m_designCamera.target = {
                static_cast<float>(grid.worldPos.x),
                static_cast<float>(grid.worldPos.y -
                    grid.dims.y * grid.cellSize / 2.0f)
            };
        } else if (custom.hasGoal) {
            m_designCamera.target = {
                static_cast<float>(custom.definition.goal.position.x),
                static_cast<float>(custom.definition.goal.position.y)
            };
        } else if (custom.hasTerrain) {
            float widestSection = -1.0f;
            for (const auto& boundary : Math::SampleTerrain(custom.definition.terrain)) {
                if (boundary.empty()) continue;
                float minX = static_cast<float>(boundary.front().x);
                float maxX = minX;
                float topY = static_cast<float>(boundary.front().y);
                for (const Math::Vector2D& point : boundary) {
                    minX = std::min(minX, static_cast<float>(point.x));
                    maxX = std::max(maxX, static_cast<float>(point.x));
                    topY = std::min(topY, static_cast<float>(point.y));
                }
                if (maxX - minX <= widestSection) continue;
                widestSection = maxX - minX;
                const float centerX = (minX + maxX) / 2.0f;
                const float surfaceY = FindTerrainSurfaceY(
                    custom.definition.terrain, centerX, topY
                ).value_or(topY);
                m_designCamera.target = {centerX, surfaceY};
            }
        } else {
            m_designCamera.target = m_designCamera.offset;
        }
        m_terrainWorkPoints.clear();
        m_terrainCurveEdges.clear();
        m_gridAnchor.reset();
        m_lastEraserMouseScreen.reset();
    }
    if (m_gridAnchor.has_value() &&
        m_gridAnchorLevel != m_context->activeCustomLevel) {
        m_gridAnchor.reset();
    }
    const auto finishGrid = [&] {
        custom.definition.grid = PreviewGrid(
            *m_gridAnchor, m_gridPreviewCursor,
            custom.definition.grid.cellSize
        );
        custom.definition.initialCamera.reset();
        custom.hasGrid = true;
        RepositionTntObstacles(custom);
        m_gridAnchor.reset();
        m_gridAnchorLevel = -1;
        m_context->customLevels->Save();
    };
    const auto selectTool = [&](DesignTool tool) {
        if (m_designTool == DesignTool::Terrain &&
            tool != DesignTool::Terrain) {
            m_terrainWorkPoints.clear();
            m_terrainCurveEdges.clear();
        }
        m_gridAnchor.reset();
        m_designTool = tool;
    };
    if (custom.hasGoal) {
        const auto surfaceY = FindTerrainSurfaceY(
            custom.definition.terrain,
            static_cast<float>(custom.definition.goal.position.x),
            static_cast<float>(custom.definition.goal.position.y)
        );
        if (surfaceY.has_value() &&
            std::abs(static_cast<float>(custom.definition.goal.position.y) - *surfaceY) < 0.5f) {
            custom.definition.goal.position.y = GoalCenterY(*surfaceY);
            m_context->customLevels->Save();
        }
    }
    if (m_context->openCustomMusicPicker) {
        m_context->openCustomMusicPicker = false;
        m_musicPickerOpen = true;
        LocateMusic(custom.musicPath, m_musicCategory, m_musicTrack);
    }

    const Vector2 screenMouse = GetMousePosition();
    bool finishTerrainClick = false;
    if (m_musicPickerOpen) {
        m_rightDragStart.reset();
    } else {
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            m_rightDragStart = screenMouse;
            m_rightDragged = false;
            m_rightPressOnCanvas = screenMouse.y >= 130.0f &&
                screenMouse.y <= m_context->windowHeight - 58.0f;
        }
        if (m_rightDragStart.has_value() &&
            (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ||
             IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))) {
            if (std::hypot(screenMouse.x - m_rightDragStart->x,
                    screenMouse.y - m_rightDragStart->y) > 5.0f) {
                m_rightDragged = true;
            }
            if (m_rightDragged) {
                const Vector2 delta = GetMouseDelta();
                m_designCamera.target.x -= delta.x / m_designCamera.zoom;
                m_designCamera.target.y -= delta.y / m_designCamera.zoom;
            }
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
            finishTerrainClick = m_rightDragStart.has_value() &&
                !m_rightDragged && m_rightPressOnCanvas;
            m_rightDragStart.reset();
        }
    }

    const Rectangle terrainButton = EditorToolbarButton(0);
    const Rectangle gridButton = EditorToolbarButton(1);
    const Rectangle goalButton = EditorToolbarButton(2);
    const Rectangle musicButton = EditorToolbarButton(3);
    const Rectangle eraserButton = EditorToolbarButton(4);
    const Rectangle passengerButton = EditorToolbarButton(5);
    const Rectangle tntButton = EditorToolbarButton(6);
    const Rectangle publishButton = LevelEditorPublishButton(m_context->windowWidth);
    const Rectangle graphicButton = GraphicModeButton(m_context->windowWidth);
    const Rectangle curveButton = TerrainCurveButton(
        m_context->windowWidth, m_context->windowHeight);
    const Rectangle menuButton = LevelEditorMenuButton(m_context->windowWidth);
    const Rectangle editButton = EditorCombiButton(
        m_context->windowWidth, m_context->windowHeight
    );
    const auto returnToMyLevels = [&] {
        m_gridAnchor.reset();
        m_gridAnchorLevel = -1;
        m_terrainWorkPoints.clear();
        m_terrainCurveEdges.clear();
        m_musicPickerOpen = false;
        m_context->requestedCustomSelector = true;
        m_context->requestedCommunitySelector = false;
        m_context->audioController->PlayUiClick();
        m_context->audioController->ReturnToSelector();
        m_context->sceneRequester->RequestSceneChange({SceneId::MainMenu});
    };
    if (IsKeyPressed(KEY_ESCAPE)) {
        returnToMyLevels();
        return;
    }
    if (!m_musicPickerOpen && UiButton::WasClicked(graphicButton)) {
        m_context->graphicMode = !m_context->graphicMode;
        m_context->audioController->PlayUiClick();
        return;
    }
    if (!m_musicPickerOpen && m_designTool == DesignTool::Terrain &&
        (IsKeyPressed(KEY_C) || UiButton::WasClicked(curveButton))) {
        m_terrainCurveMode = !m_terrainCurveMode;
        m_terrainCloseRejected = false;
        m_context->audioController->PlayUiClick();
        return;
    }

    if (!m_musicPickerOpen &&
        (IsKeyPressed(KEY_ONE) || UiButton::WasClicked(terrainButton))) {
        selectTool(DesignTool::Terrain);
        return;
    }
    if (!m_musicPickerOpen &&
        (IsKeyPressed(KEY_TWO) || UiButton::WasClicked(gridButton))) {
        if (m_gridAnchor.has_value()) finishGrid();
        selectTool(DesignTool::Grid);
        return;
    }
    if (!m_musicPickerOpen &&
        (IsKeyPressed(KEY_THREE) || UiButton::WasClicked(goalButton))) {
        selectTool(DesignTool::Goal);
        return;
    }
    if (!m_musicPickerOpen &&
        (IsKeyPressed(KEY_FOUR) || UiButton::WasClicked(musicButton))) {
        m_gridAnchor.reset();
        m_terrainWorkPoints.clear();
        m_terrainCurveEdges.clear();
        m_musicCategory = 0;
        m_musicTrack = 0;
        LocateMusic(custom.musicPath, m_musicCategory, m_musicTrack);
        m_musicListScroll = 0.0f;
        m_musicPickerOpen = true;
        m_context->audioController->StartLevelEditorMusic(custom.musicPath, true);
        return;
    }
    if (!m_musicPickerOpen &&
        (IsKeyPressed(KEY_FIVE) || UiButton::WasClicked(eraserButton))) {
        selectTool(DesignTool::Eraser);
        return;
    }
    if (!m_musicPickerOpen &&
        (IsKeyPressed(KEY_SIX) || UiButton::WasClicked(passengerButton))) {
        selectTool(DesignTool::Passenger);
        return;
    }
    if (!m_musicPickerOpen &&
        (IsKeyPressed(KEY_SEVEN) || UiButton::WasClicked(tntButton))) {
        selectTool(DesignTool::TntObstacle);
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
    if (UiButton::WasClicked(publishButton)) {
        const std::size_t index = static_cast<std::size_t>(m_context->activeCustomLevel);
        if (m_context->customLevels->HasValidApproval(index)) {
            m_context->audioController->PlayUiClick();
            m_publishFeedbackDraftId = custom.draftId;
            m_context->publications.Publish(
                m_context->customLevels->GetPublicationPayload(index)
            );
        }
        return;
    }
    if (UiButton::WasClicked(menuButton)) {
        returnToMyLevels();
        return;
    }
    if (IsKeyPressed(KEY_ENTER) || UiButton::WasClicked(editButton)) {
        if (!custom.hasTerrain || !custom.hasGrid || !custom.hasGoal) return;
        m_gridAnchor.reset();
        m_context->audioController->PlayUiClick();
        m_context->customLevels->Save();
        LevelDefinition definition = custom.definition;
        definition.levelNumber = -1;
        definition.initialCamera.reset();
        auto scenario = m_context->audioController->GetCurrentScenario();
        m_context->audioController->StartLevel(-1, scenario.levelEditorTrackName);
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

    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
        m_designTool != DesignTool::Eraser) {
        m_lastEraserMouseScreen.reset();
        m_erasedPassengerPositions.clear();
    }
    if (screenMouse.y < 130.0f || screenMouse.y > m_context->windowHeight - 58.0f) {
        return;
    }
    const Vector2 worldMouse = GetScreenToWorld2D(screenMouse, m_designCamera);
    const Math::Vector2D point{worldMouse.x, worldMouse.y};
    if (m_designTool == DesignTool::Grid && m_gridAnchor.has_value()) {
        m_gridPreviewCursor = point;
    }

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
            if (sampleScreen.y < 130.0f ||
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
                    dy >= -33.0f * scale * m_designCamera.zoom - 8.0f &&
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
            for (std::size_t i = 0; i < custom.definition.tntObstacles.size();) {
                const auto& obstacle = custom.definition.tntObstacles[i];
                const float radius = 20.0f;
                if (std::abs(samplePoint.x - obstacle.position.x) > radius ||
                    std::abs(samplePoint.y - obstacle.position.y) > radius) {
                    ++i;
                    continue;
                }
                custom.definition.tntObstacles.erase(
                    custom.definition.tntObstacles.begin() + i
                );
                changed = true;
                break;
            }
            if (changed) continue;
            if (custom.hasGoal) {
                const Math::AABB goalArea = custom.definition.goal.GetAABB();
                const Rectangle goalBounds{
                    static_cast<float>(goalArea.min.x),
                    static_cast<float>(goalArea.min.y),
                    static_cast<float>(goalArea.max.x - goalArea.min.x),
                    static_cast<float>(goalArea.max.y - goalArea.min.y)
                };
                if (CheckCollisionPointRec(sampleWorld, goalBounds)) {
                    custom.hasGoal = false;
                    changed = true;
                    break;
                }
            }
            if (custom.hasGrid) {
                const GridDefinition& grid = custom.definition.grid;
                const float width = grid.dims.x * grid.cellSize;
                const float height = grid.dims.y * grid.cellSize;
                const Rectangle gridBounds{
                    static_cast<float>(grid.worldPos.x) - width / 2.0f,
                    static_cast<float>(grid.worldPos.y) - height,
                    width, height
                };
                if (CheckCollisionPointRec(sampleWorld, gridBounds)) {
                    custom.hasGrid = false;
                    custom.definition.initialCamera.reset();
                    changed = true;
                    break;
                }
            }
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
            RepositionTntObstacles(custom);
            if (custom.hasGoal) {
                const auto surfaceY = FindTerrainSurfaceY(
                    custom.definition.terrain,
                    static_cast<float>(custom.definition.goal.position.x),
                    static_cast<float>(custom.definition.goal.position.y)
                );
                if (surfaceY.has_value()) {
                    custom.definition.goal.position.y = GoalCenterY(surfaceY.value());
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
            // cada pasajero nuevo recibe una cara al azar de assets/textures/passengers/faces
            const auto& faces = m_context->textureLibrary->GetPassengerFaces();
            std::string face;
            if (!faces.empty()) {
                std::uniform_int_distribution<std::size_t> chooseFace(0, faces.size() - 1);
                face = faces[chooseFace(randomEngine)].name;
            }
            custom.definition.passengers.push_back({
                .position = {point.x, surfaceY.value()},
                .hasSkirt = skirtChance(randomEngine),
                .face = std::move(face)
            });
            m_context->customLevels->Save();
        }
        return;
    }

    if (m_designTool == DesignTool::TntObstacle &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const auto surfaceY = FindTerrainSurfaceY(
            custom.definition.terrain,
            static_cast<float>(point.x),
            static_cast<float>(point.y)
        );
        if (surfaceY.has_value()) {
            const Math::Vector2D position{point.x, surfaceY.value() - custom.definition.grid.cellSize / 2.0f};
            const bool exists = std::any_of(
                custom.definition.tntObstacles.begin(),
                custom.definition.tntObstacles.end(),
                [&](const TntObstacleDefinition& obstacle) {
                    return std::abs(static_cast<float>(obstacle.position.x - position.x)) <
                        custom.definition.grid.cellSize * 0.5f;
                }
            );
            if (!exists) {
                custom.definition.tntObstacles.push_back({position});
                m_context->customLevels->Save();
            }
        }
        return;
    }

    if (m_designTool == DesignTool::Terrain &&
        IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        m_dragTerrainVertex.reset();
    }
    if (m_designTool == DesignTool::Terrain &&
        IsMouseButtonDown(MOUSE_BUTTON_LEFT) && m_dragTerrainVertex.has_value()) {
        m_terrainWorkPoints[*m_dragTerrainVertex] = point;
        m_terrainCloseRejected = false;
        return;
    }
    if (m_designTool == DesignTool::Terrain &&
        IsKeyPressed(KEY_BACKSPACE) && !m_terrainWorkPoints.empty()) {
        m_terrainWorkPoints.pop_back();
        if (!m_terrainCurveEdges.empty()) m_terrainCurveEdges.pop_back();
        m_terrainCloseRejected = false;
        return;
    }
    if (m_designTool == DesignTool::Terrain &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (std::size_t i = 0; i < m_terrainWorkPoints.size(); ++i) {
            const auto screen = GetWorldToScreen2D(m_terrainWorkPoints[i].ToRayVec(), m_designCamera);
            if (std::hypot(screen.x-screenMouse.x, screen.y-screenMouse.y) <= 12.0f) {
                m_dragTerrainVertex = i;
                m_terrainCloseRejected = false;
                return;
            }
        }
        if (!m_terrainWorkPoints.empty()) {
            m_terrainCurveEdges.push_back(m_terrainCurveMode);
        }
        m_terrainWorkPoints.push_back(point);
        m_terrainCloseRejected = false;
    } else if (m_designTool == DesignTool::Terrain &&
               finishTerrainClick) {
        const auto triangulation = ValidateTerrainDraft(
            m_terrainWorkPoints, m_terrainCurveEdges);
        if (!triangulation.validation.valid()) {
            m_terrainCloseRejected = true;
            return;
        }
        custom.definition.terrain.polygons.push_back(
            BuildTerrainPolygon(m_terrainWorkPoints, m_terrainCurveEdges));
        custom.hasTerrain = true;
        m_terrainWorkPoints.clear();
        m_terrainCurveEdges.clear();
        RepositionTntObstacles(custom);
        m_terrainCloseRejected = false;
        if (custom.hasGoal) {
            const auto surfaceY = FindTerrainSurfaceY(
                custom.definition.terrain,
                static_cast<float>(custom.definition.goal.position.x),
                static_cast<float>(custom.definition.goal.position.y)
            );
            if (surfaceY.has_value()) {
                custom.definition.goal.position.y = GoalCenterY(surfaceY.value());
            }
        }
        m_context->customLevels->Save();
    } else if (m_designTool == DesignTool::Grid &&
               IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (m_gridAnchor.has_value()) {
            finishGrid();
        } else {
            m_gridAnchor = point;
            m_gridPreviewCursor = point;
            m_gridAnchorLevel = m_context->activeCustomLevel;
        }
    } else if (m_designTool == DesignTool::Goal &&
               IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const auto surfaceY = FindTerrainSurfaceY(
            custom.definition.terrain,
            static_cast<float>(point.x),
            static_cast<float>(point.y)
        );
        if (surfaceY.has_value()) {
            custom.definition.goal.position = {point.x, GoalCenterY(surfaceY.value())};
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
    const Texture2D& background = m_context->textureLibrary->GetBackground(
        BackgroundId::Miraflores
    );
    DrawTexturePro(background,
        Rectangle{0.0f, 0.0f,
            static_cast<float>(background.width),
            static_cast<float>(background.height)},
        Rectangle{0.0f, 0.0f,
            static_cast<float>(m_context->windowWidth),
            static_cast<float>(m_context->windowHeight)},
        Vector2{}, 0.0f, WHITE);
    DrawRectangle(0, 0, m_context->windowWidth,
        m_context->windowHeight, Fade(WHITE, 0.22f));
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
        if (m_context->graphicMode) {
            for (std::size_t i = 0; i+2 < mesh.indices.size(); i += 3) {
                const auto a = mesh.vertices[mesh.indices[i]].ToRayVec();
                const auto b = mesh.vertices[mesh.indices[i+1]].ToRayVec();
                const auto c = mesh.vertices[mesh.indices[i+2]].ToRayVec();
                DrawTriangleLines(a,b,c,Color{65,190,255,255});
            }
        }
    }
    if (m_designTool == DesignTool::Terrain || m_context->graphicMode) {
        for (const auto& polygon : custom.definition.terrain.polygons) {
            for (const auto& segment : polygon.segments) {
                if (segment.type != Math::TerrainSegmentType::CatmullRom) continue;
                const auto curve = Math::SampleCatmullRom(
                    segment.points, std::max(1, segment.resolution));
                for (std::size_t i = 1; i < curve.size(); ++i) {
                    DrawLineEx(curve[i - 1].ToRayVec(), curve[i].ToRayVec(),
                        4.0f / m_designCamera.zoom, ORANGE);
                }
            }
        }
    }
    for (const auto& passenger : custom.definition.passengers) {
        DrawPassenger(
            *m_context->textureLibrary,
            static_cast<float>(passenger.position.x),
            static_cast<float>(passenger.position.y),
            passenger.hasSkirt,
            PassengerEditorScale(m_designCamera.zoom),
            BLACK,
            m_context->textureLibrary->FindPassengerFace(passenger.face),
            0.0f
        );
    }
    for (const auto& obstacle : custom.definition.tntObstacles) {
        DrawTexturePro(
            m_context->textureLibrary->GetPartTexture(PartType::TNTBarrel),
            Rectangle{0, 0,
                static_cast<float>(m_context->textureLibrary->GetPartTexture(PartType::TNTBarrel).width),
                static_cast<float>(m_context->textureLibrary->GetPartTexture(PartType::TNTBarrel).height)},
            Rectangle{
                static_cast<float>(obstacle.position.x - custom.definition.grid.cellSize / 2.0f),
                static_cast<float>(obstacle.position.y - custom.definition.grid.cellSize / 2.0f),
                custom.definition.grid.cellSize, custom.definition.grid.cellSize
            }, Vector2{}, 0.0f, WHITE
        );
    }
    if (m_designTool == DesignTool::TntObstacle) {
        const Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), m_designCamera);
        const auto surfaceY = FindTerrainSurfaceY(
            custom.definition.terrain, mouse.x, mouse.y
        );
        if (surfaceY.has_value()) {
            const float size = custom.definition.grid.cellSize;
            const Texture2D& texture =
                m_context->textureLibrary->GetPartTexture(PartType::TNTBarrel);
            DrawTexturePro(
                texture,
                Rectangle{0, 0, static_cast<float>(texture.width),
                    static_cast<float>(texture.height)},
                Rectangle{mouse.x - size / 2.0f, *surfaceY - size, size, size},
                Vector2{}, 0.0f, Fade(WHITE, 0.55f)
            );
        }
    }
    const auto workValidation = Math::ValidateTerrainPolygon(m_terrainWorkPoints);
    const auto draftValidation = ValidateTerrainDraft(
        m_terrainWorkPoints, m_terrainCurveEdges).validation;
    const bool curvedShapeInvalid = m_terrainWorkPoints.size() >= 3 &&
        workValidation.valid() && !draftValidation.valid();
    const Color terrainDraftBlue{39, 78, 235, 255};
    const auto drawTerrainDraftEdge = [&](Vector2 start, Vector2 end, bool invalid) {
        DrawLineEx(start, end, 7.0f / m_designCamera.zoom, BLACK);
        DrawLineEx(start, end, 4.5f / m_designCamera.zoom,
            invalid ? RED : terrainDraftBlue);
    };
    for (std::size_t i = 0; i < m_terrainWorkPoints.size(); ++i) {
        const Vector2 point = m_terrainWorkPoints[i].ToRayVec();
        const bool badVertex = std::find(workValidation.badVertices.begin(),
            workValidation.badVertices.end(), i) != workValidation.badVertices.end();
        DrawCircleV(point, (badVertex ? 10.0f : 8.0f) /
            m_designCamera.zoom, BLACK);
        DrawCircleV(point, (badVertex ? 8.0f : 6.0f) /
            m_designCamera.zoom, badVertex ? RED : terrainDraftBlue);
        if (i > 0) {
            const bool badEdge = std::find(workValidation.badEdges.begin(),
                workValidation.badEdges.end(), i-1) != workValidation.badEdges.end();
            drawTerrainDraftEdge(m_terrainWorkPoints[i - 1].ToRayVec(),
                point, badEdge || curvedShapeInvalid);
        }
    }
    if (m_terrainWorkPoints.size() >= 3) {
        const std::size_t last = m_terrainWorkPoints.size()-1;
        const bool badEdge = std::find(workValidation.badEdges.begin(),
            workValidation.badEdges.end(), last) != workValidation.badEdges.end();
        drawTerrainDraftEdge(m_terrainWorkPoints[last].ToRayVec(),
            m_terrainWorkPoints.front().ToRayVec(),
            badEdge || curvedShapeInvalid);
        const auto draftPolygon = BuildTerrainPolygon(
            m_terrainWorkPoints, m_terrainCurveEdges);
        for (const auto& segment : draftPolygon.segments) {
            if (segment.type != Math::TerrainSegmentType::CatmullRom) continue;
            const auto curve = Math::SampleCatmullRom(segment.points, 12);
            for (std::size_t i = 1; i < curve.size(); ++i) {
                DrawLineEx(curve[i - 1].ToRayVec(), curve[i].ToRayVec(),
                    5.0f / m_designCamera.zoom,
                    draftValidation.valid() ? ORANGE : RED);
            }
        }
    }

    std::optional<GridDefinition> visibleGrid;
    const bool gridPreview = m_designTool == DesignTool::Grid &&
        m_gridAnchor.has_value();
    if (gridPreview) {
        visibleGrid = PreviewGrid(*m_gridAnchor, m_gridPreviewCursor,
            custom.definition.grid.cellSize);
    } else if (custom.hasGrid) {
        visibleGrid = custom.definition.grid;
    } else if (m_designTool == DesignTool::Grid) {
        const Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), m_designCamera);
        visibleGrid = PreviewGrid({mouse.x, mouse.y}, {mouse.x, mouse.y},
            custom.definition.grid.cellSize);
    }
    if (visibleGrid.has_value()) {
        const GridDefinition& grid = *visibleGrid;
        const float width = grid.dims.x * grid.cellSize;
        const float height = grid.dims.y * grid.cellSize;
        const float left = static_cast<float>(grid.worldPos.x) - width / 2.0f;
        const float top = static_cast<float>(grid.worldPos.y) - height;
        DrawRectangle(
            static_cast<int>(left),
            static_cast<int>(top),
            static_cast<int>(width),
            static_cast<int>(height),
            Fade(SKYBLUE, gridPreview ? 0.28f : 0.18f)
        );
        DrawRectangleLinesEx(Rectangle{left, top, width, height},
            gridPreview ? 3.0f : 2.0f, gridPreview ? GOLD : BLUE);
        for (int x = 1; x < grid.dims.x; ++x) {
            DrawLine(
                static_cast<int>(left + x * grid.cellSize),
                static_cast<int>(top),
                static_cast<int>(left + x * grid.cellSize),
                static_cast<int>(top + height),
                Fade(BLUE, 0.65f)
            );
        }
        for (int y = 1; y < grid.dims.y; ++y) {
            DrawLine(
                static_cast<int>(left),
                static_cast<int>(top + y * grid.cellSize),
                static_cast<int>(left + width),
                static_cast<int>(top + y * grid.cellSize),
                Fade(BLUE, 0.65f)
            );
        }
        if (gridPreview) {
            DrawRectangleLinesEx(Rectangle{
                static_cast<float>(m_gridAnchor->x),
                static_cast<float>(m_gridAnchor->y),
                grid.cellSize, grid.cellSize
            }, 3.0f, WHITE);
        }
    }

    if (custom.hasGoal || m_designTool == DesignTool::Goal) {
        GoalDefinition goalDef = custom.definition.goal;
        if (m_designTool == DesignTool::Goal && !custom.hasGoal) {
            const Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), m_designCamera);
            const auto surfaceY = FindTerrainSurfaceY(
                custom.definition.terrain, mouse.x, mouse.y
            );
            goalDef.position = {
                mouse.x,
                surfaceY.has_value() ? GoalCenterY(*surfaceY) : mouse.y
            };
        }
        const Math::AABB goalArea = goalDef.GetAABB();
        const Rectangle goalRect{
            static_cast<float>(goalArea.min.x),
            static_cast<float>(goalArea.min.y),
            static_cast<float>(goalArea.max.x - goalArea.min.x),
            static_cast<float>(goalArea.max.y - goalArea.min.y)
        };
        DrawRectangleRec(goalRect, Fade(GREEN, 0.12f));
        DrawRectangleLinesEx(goalRect, 2.0f, Fade(GOLD, 0.85f));

        Texture2D goalTexture = m_context->textureLibrary->GetLevelGoalTexture();
        DrawTexturePro(
            goalTexture,
            Rectangle{0.0f, 0.0f, static_cast<float>(goalTexture.width), static_cast<float>(goalTexture.height)},
            goalRect,
            Vector2{0.0f, 0.0f},
            0.0f,
            WHITE
        );
    }
    EndMode2D();

    const Rectangle terrainButton = EditorToolbarButton(0);
    const Rectangle gridButton = EditorToolbarButton(1);
    const Rectangle goalButton = EditorToolbarButton(2);
    const Rectangle musicButton = EditorToolbarButton(3);
    const Rectangle eraserButton = EditorToolbarButton(4);
    const Rectangle passengerButton = EditorToolbarButton(5);
    const Rectangle tntButton = EditorToolbarButton(6);
    const Rectangle publishButton = LevelEditorPublishButton(m_context->windowWidth);
    const Rectangle graphicButton = GraphicModeButton(m_context->windowWidth);
    const Rectangle menuButton = LevelEditorMenuButton(m_context->windowWidth);
    const UiTextures& ui = m_context->textureLibrary->GetUi();
    DrawRectangle(0, 0, m_context->windowWidth, 129, Fade(BLACK, 0.88f));
    DrawRectangle(0, 127, m_context->windowWidth, 3,
        Color{237, 18, 121, 255});
    DrawEditorToolButton(terrainButton, 0,
        m_designTool == DesignTool::Terrain, false, false,
        *m_context->textureLibrary);
    DrawEditorToolButton(gridButton, 1,
        m_designTool == DesignTool::Grid, !custom.hasGrid, custom.hasGrid,
        *m_context->textureLibrary);
    DrawEditorToolButton(goalButton, 2,
        m_designTool == DesignTool::Goal, !custom.hasGoal, custom.hasGoal,
        *m_context->textureLibrary);
    DrawEditorToolButton(musicButton, 3, m_musicPickerOpen, false, false,
        *m_context->textureLibrary);
    DrawEditorToolButton(eraserButton, 4,
        m_designTool == DesignTool::Eraser, false, false,
        *m_context->textureLibrary);
    DrawEditorToolButton(passengerButton, 5,
        m_designTool == DesignTool::Passenger, false, false,
        *m_context->textureLibrary);
    DrawEditorToolButton(tntButton, 6,
        m_designTool == DesignTool::TntObstacle, false, false,
        *m_context->textureLibrary);
    const bool ready = custom.hasTerrain && custom.hasGrid && custom.hasGoal;
    const bool approved = m_context->customLevels->HasValidApproval(
        static_cast<std::size_t>(m_context->activeCustomLevel)
    );
    UiButton::DrawTextured(publishButton, ui.menuPublish, approved, "Publicar");
    UiButton::DrawGraphicModeToggle(graphicButton,
        m_context->graphicMode, ui);
    UiButton::DrawTextured(menuButton,
        ui.menuBack, true, "Menú");
    const Rectangle editButton = EditorCombiButton(
        m_context->windowWidth, m_context->windowHeight
    );
    UiButton::DrawTextured(editButton, ui.game.toolbarSlot, ready);
    DrawTexturePro(ui.game.editHammer,
        Rectangle{0, 0, static_cast<float>(ui.game.editHammer.width),
            static_cast<float>(ui.game.editHammer.height)},
        Rectangle{editButton.x + 24, editButton.y + 2, 60, 60},
        Vector2{}, 0, ready ? WHITE : Fade(WHITE, 0.55f));
    UiButton::DrawTextureFit(ui.keyEnterBadge,
        Rectangle{editButton.x + editButton.width - 80.0f,
            editButton.y + 4.0f, 76.0f, 32.0f},
        ready ? WHITE : Fade(WHITE, 0.55f));
    const char* editLabel = "Editar combi";
    DrawTextFont(editLabel,
        static_cast<int>(editButton.x +
            (editButton.width - MeasureTextFont(editLabel, 17)) / 2.0f),
        static_cast<int>(editButton.y + 66), 17,
        ready ? WHITE : LIGHTGRAY);
    DrawRectangle(0, m_context->windowHeight - 58,
        m_context->windowWidth, 58, Fade(BLACK, 0.88f));
    DrawRectangle(0, m_context->windowHeight - 58,
        m_context->windowWidth, 3, Color{237, 18, 121, 255});
    if (m_designTool == DesignTool::Terrain) {
        const Rectangle curveButton = TerrainCurveButton(
            m_context->windowWidth, m_context->windowHeight);
        const bool hovered = UiButton::IsHovered(curveButton);
        DrawRectangleRec(curveButton,
            Fade(m_terrainCurveMode ? ORANGE : DARKGRAY,
                hovered ? 0.55f : 0.35f));
        DrawRectangleLinesEx(curveButton, hovered ? 2.0f : 1.0f,
            m_terrainCurveMode ? ORANGE : LIGHTGRAY);
        DrawTextFont("CURVA CATMULL-ROM [C]",
            static_cast<int>(curveButton.x + 10),
            static_cast<int>(curveButton.y + 11), 13, WHITE);
        DrawTextFont(m_terrainCurveMode ? "ON" : "OFF",
            static_cast<int>(curveButton.x + curveButton.width - 39),
            static_cast<int>(curveButton.y + 11), 13,
            m_terrainCurveMode ? GOLD : LIGHTGRAY);
    }

    std::string instructions;
    if (m_designTool == DesignTool::Grid) {
        if (m_gridAnchor.has_value()) {
            const GridDefinition preview = PreviewGrid(
                *m_gridAnchor, m_gridPreviewCursor,
                custom.definition.grid.cellSize
            );
            instructions = "Cuadrícula " +
                std::to_string(preview.dims.x) + " x " +
                std::to_string(preview.dims.y) +
                " | segundo clic o botón Cuadrícula: confirmar | Esc: Mis niveles";
        } else {
            instructions = "Actual " + std::to_string(custom.definition.grid.dims.x) +
                " x " + std::to_string(custom.definition.grid.dims.y) +
                " | primer clic: celda inicial | mueve el mouse para redimensionar | máximo 10 x 8";
        }
    } else {
        instructions = m_designTool == DesignTool::Terrain
            ? "Terreno: clic para puntos | C: curva en próximos tramos | clic derecho: cerrar | arrastra derecho: mover"
            : m_designTool == DesignTool::Eraser
            ? "Borrador: clic en pasajero, meta, cuadrícula o terreno | arrastra para borrar varios"
            : m_designTool == DesignTool::Passenger
            ? "Haz clic sobre el terreno para colocar un pasajero"
            : m_designTool == DesignTool::TntObstacle
            ? "Haz clic sobre el terreno para colocar un TNT estático"
            : "Haz clic sobre el terreno para colocar la meta en su superficie";
    }
    if (m_terrainCloseRejected && !draftValidation.valid()) {
        instructions = std::string(Math::PolygonIssueMessage(draftValidation.issue)) +
            " | arrastra vértices o Backspace para corregir";
    }
    DrawTextFont(instructions.c_str(), 20, 106, 15,
        m_terrainCloseRejected ? RED : WHITE);
    DrawTextFont(
        TextFormat(
            "Terreno %s   Cuadrícula %s   Meta %s   TNT %d",
            custom.hasTerrain ? "OK" : "-",
            custom.hasGrid ? "OK" : "-",
            custom.hasGoal ? "OK" : "-",
            static_cast<int>(custom.definition.tntObstacles.size())
        ),
        20,
        m_context->windowHeight - 28,
        16,
        ready ? GREEN : WHITE
    );
    DrawTextFont("Arrastra clic derecho: mover vista",
        m_context->windowWidth - 290,
        m_context->windowHeight - 27, 13, WHITE);
    const PublicationSnapshot& publication = m_context->publications.GetSnapshot();
    const bool showPublicationFeedback =
        m_publishFeedbackDraftId == custom.draftId &&
        !publication.message.empty();
    const char* publicationText = showPublicationFeedback
        ? publication.message.c_str()
        : approved
            ? "Esta versión fue completada y puede publicarse con el botón de arriba."
            : "Para publicar, completa esta versión exacta. Los cambios invalidan la victoria.";
    if (showPublicationFeedback) {
        const Rectangle publicationBadge{
            14.0f,
            static_cast<float>(m_context->windowHeight - 55),
            MeasureTextFont(publicationText, 13) + 12.0f,
            27.0f
        };
        DrawRectangleRec(publicationBadge, Fade(BLACK, 0.78f));
        DrawRectangleLinesEx(publicationBadge, 1.0f,
            Color{237, 18, 121, 255});
    }
    DrawTextFont(publicationText, 20, m_context->windowHeight - 48, 13,
        showPublicationFeedback
            ? (publication.status == PublicationStatus::Error ||
                publication.status == PublicationStatus::Unavailable ? GOLD : GREEN)
            : approved ? GREEN : GOLD);
    if (m_musicPickerOpen) {
        const auto& categories = GetCustomMusicCategories();
        const auto& tracks = categories[m_musicCategory].tracks;
        const float panelX = (m_context->windowWidth - 700.0f) / 2.0f;
        const float panelY = (m_context->windowHeight - 540.0f) / 2.0f;
        DrawRectangle(0, 0, m_context->windowWidth, m_context->windowHeight, Fade(BLACK, 0.5f));
        const Rectangle panel{panelX, panelY, 700.0f, 540.0f};
        DrawRectangleRec(panel, Fade(BLACK, 0.96f));
        DrawRectangleLinesEx(panel, 3.0f, Color{129, 195, 71, 255});
        DrawTextFont("Música del nivel", static_cast<int>(panelX + 18), static_cast<int>(panelY + 18), 22, GOLD);
        for (std::size_t i = 0; i < categories.size(); ++i) {
            const Rectangle tab{panelX + 18.0f + (i % 4) * 164.0f,
                panelY + 50.0f + (i / 4) * 36.0f, 158.0f, 30.0f};
            DrawEditorTextButton(tab, categories[i].name.c_str(),
                ui.game.toolbarSlot,
                static_cast<int>(i) == m_musicCategory);
        }
        const Rectangle list{panelX + 18.0f, panelY + 164.0f, 664.0f, 300.0f};
        DrawRectangleRec(list, Color{22, 22, 22, 255});
        DrawRectangleLinesEx(list, 1.0f, Color{129, 195, 71, 255});
        for (std::size_t i = 0; i < tracks.size(); ++i) {
            const float y = list.y + i * 52.0f - m_musicListScroll;
            if (y < list.y || y + 46.0f > list.y + list.height) continue;
            const Rectangle row{list.x, y, list.width, 46.0f};
            DrawRectangleRec(row, static_cast<int>(i) == m_musicTrack
                ? Fade(SKYBLUE, 0.32f) : Fade(DARKGRAY, 0.35f));
            const std::string label = tracks[i].artist.empty()
                ? tracks[i].title
                : tracks[i].title + " - " + tracks[i].artist;
            DrawTextFont(label.c_str(), static_cast<int>(row.x + 12), static_cast<int>(row.y + 13), 16, WHITE);
        }
        DrawEditorTextButton(
            Rectangle{panelX + 372.0f, panelY + 484.0f, 150.0f, 38.0f},
            "Confirmar", ui.game.toolbarSlot);
        DrawEditorTextButton(
            Rectangle{panelX + 532.0f, panelY + 484.0f, 150.0f, 38.0f},
            "Cancelar", ui.game.toolbarSlot);
    }
    EndDrawing();
}
