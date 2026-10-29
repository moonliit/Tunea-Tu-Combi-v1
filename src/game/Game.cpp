#include "game/Game.hpp"
#include "game/Part.hpp"
#include "math/GridCoord.hpp"
#include "math/Vector2D.hpp"
#include "physics/Box2DPhysics.hpp"
#include <optional>
#include <raylib.h>
#include <format>

using namespace Math;

Game::Game(int width, int height, const std::string& title, LevelSpecs specs):
    m_screenWidth(width),
    m_screenHeight(height),
    m_gravity(9.8f * 40.0f),
    m_currentState(GameState::Edit),
    m_currentPartType(std::nullopt),
    m_level(specs)
{
    InitWindow(width, height, title.c_str());
    SetTargetFPS(60);

    // Initialize Camera2D
    m_camera = Camera2D(Vector2{0});
    m_camera.target = specs.initialCameraPos.ToRayVec();
    m_camera.offset = {m_screenWidth / 2.f, m_screenHeight / 2.f};
    m_camera.rotation = 0.0f;
    m_camera.zoom = 1.0f;

    // Initialize Physics World
    m_physics = std::make_unique<Box2DWorld>(Vector2D{0.0f, m_gravity});

    // Setup Terrain Control Points
    m_terrainVertices = m_physics->CreateTerrainChain(m_level.GetCatmullromPoints());
}

void Game::SetPartType(PartType type) {
    if (m_currentPartType.has_value() && m_currentPartType.value() == type) {
        m_currentPartType = std::nullopt;
    } else {
        m_currentPartType = type;
    }
}

void Game::Run() {
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.033f) dt = 0.033f;

        ProcessInput(dt);
        Update(dt);
        Render();
    }
}

void Game::ProcessInput(float dt) {
    // 1. Toggle State (ENTER)
    if (IsKeyPressed(KEY_ENTER)) {
        if (m_currentState == GameState::Edit) {
            TransitionToPlayMode();
        } else {
            ResetToEditMode();
        }
    }

    // 2. Camera Panning via WASD
    float cameraSpeed = 300.0f * dt;
    if (IsKeyDown(KEY_W)) m_camera.target.y -= cameraSpeed;
    if (IsKeyDown(KEY_S)) m_camera.target.y += cameraSpeed;
    if (IsKeyDown(KEY_A)) m_camera.target.x -= cameraSpeed;
    if (IsKeyDown(KEY_D)) m_camera.target.x += cameraSpeed;

    // 3. Edit Mode Toolbar & Grid Placement
    if (m_currentState == GameState::Edit) {
        Vector2 mouseScreen = GetMousePosition();

        // Toolbar UI Click Checks (Bottom Bar)
        if (mouseScreen.y > 520.0f) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (mouseScreen.x >= 20 && mouseScreen.x <= 120)
                    SetPartType(PartType::SquareBlock);
                if (mouseScreen.x >= 140 && mouseScreen.x <= 240)
                    SetPartType(PartType::CircleWheel);
            }
        } else if (m_currentPartType.has_value()) {
            // Workspace Grid Interaction
            Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, m_camera);
            auto gridCoordOpt = m_level.GetGrid().WorldToCoord(Math::Vector2D{mouseWorld.x, mouseWorld.y});

            if (gridCoordOpt.has_value()) {
                GridCoord coord = gridCoordOpt.value();

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    m_level.GetGrid().PlacePart(coord, m_currentPartType.value());
                }
                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    m_level.GetGrid().RemovePart(coord);
                }
            }

        }
    }
}

void Game::Update(float dt) {
    if (m_currentState == GameState::Play) {
        m_physics->Step(dt);
    }
}

void Game::TransitionToPlayMode() {
    m_currentState = GameState::Play;

    for (auto& part : m_level.GetGrid().GetActiveParts()) {
        part->SetPhysicsBody(m_level.GetGrid(), m_physics);
    }
}

void Game::ResetToEditMode() {
    m_currentState = GameState::Edit;

    // Re-initialize physics world for clean slate
    for (auto& part : m_level.GetGrid().GetActiveParts()) {
        part->ResetPhysicsBody();
    }
    m_physics = std::make_unique<Box2DWorld>(Vector2D{0.0f, m_gravity});
    m_terrainVertices = m_physics->CreateTerrainChain(m_level.GetCatmullromPoints());
}

void Game::Render() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // --- World Space Rendering ---
    BeginMode2D(m_camera);

    // Draw Terrain
    for (size_t i = 0; i < m_terrainVertices.size() - 1; ++i) {
        DrawLineV(
            m_terrainVertices[i].ToRayVec(),
            m_terrainVertices[i+1].ToRayVec(),
            DARKGRAY
        );
    }

    // If in Edit Mode, render grid items as blueprints
    if (m_currentState == GameState::Edit) {
        m_level.GetGrid().DrawSelf();

        for (const auto& part : m_level.GetGrid().GetActiveParts()) {
            part->DrawSelf(m_level.GetGrid());
        }

        // Draw Ghost Preview under mouse cursor
        Vector2 mouseScreen = GetMousePosition();
        if (m_currentPartType.has_value() && mouseScreen.y <= 520.0f) {
            Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, m_camera);
            auto gridCoordOpt = m_level.GetGrid().WorldToCoord(Math::Vector2D{mouseWorld.x, mouseWorld.y});

            if (gridCoordOpt.has_value()) {
                GridCoord coord = gridCoordOpt.value();

                Part::DrawGhost(m_currentPartType.value(), coord, m_level.GetGrid());

                // if (m_currentPartType == PartType::SquareBlock) {
                //     DrawRectangle(previewX, previewY, cellSize, cellSize, Fade(BLUE, 0.2f));
                //     DrawRectangleLines(previewX, previewY, cellSize, cellSize, BLUE);
                // } else {
                //     DrawCircle(previewX + cellSize / 2, previewY + cellSize / 2, cellSize / 2, Fade(GREEN, 0.2f));
                //     DrawCircleLines(previewX + cellSize / 2, previewY + cellSize / 2, cellSize / 2, GREEN);
                // }
            }

        }
    }
    // If in Play Mode, render active physics bodies
    else if (m_currentState == GameState::Play) {
        for (const auto& part : m_level.GetGrid().GetActiveParts()) {
            part->DrawSelf(m_level.GetGrid());
        }
    }

    EndMode2D();

    // --- Screen Space UI Layer ---
    DrawRectangle(0, 520, m_screenWidth, 80, LIGHTGRAY);
    DrawRectangleLines(0, 520, m_screenWidth, 80, GRAY);

    // Toolbar Buttons
    DrawRectangle(20, 535, 100, 50, (m_currentPartType == PartType::SquareBlock) ? BLUE : DARKGRAY);
    DrawText("Square", 45, 550, 14, WHITE);

    DrawRectangle(140, 535, 100, 50, (m_currentPartType == PartType::CircleWheel) ? GREEN : DARKGRAY);
    DrawText("Circle", 168, 550, 14, WHITE);

    // Status instructions text
    std::string modeStr = (m_currentState == GameState::Edit)
        ? "MODE: EDIT (Left Click: Place, Right Click: Delete, ENTER: Play)"
        : "MODE: PLAY (Press ENTER to Edit)";
    DrawText(modeStr.c_str(), 20, 20, 18, (m_currentState == GameState::Edit) ? DARKGREEN : MAROON);

    Vector2 mouseScreen = GetMousePosition();
    Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, m_camera);
    auto coordOpt = m_level.GetGrid().WorldToCoord({mouseWorld.x, mouseWorld.y});

    auto xd = coordOpt.has_value() ?
                std::format("{}, {}", coordOpt.value().x, coordOpt.value().y)
                : "none";
    std::string xdd = "none";
    if (coordOpt.has_value()) {
        auto x = m_level.GetGrid().CoordToWorld(coordOpt.value()).x;
        auto y = m_level.GetGrid().CoordToWorld(coordOpt.value()).y;
        xdd = std::format("{}, {}", (float)x, (float)y);
    }
    // DrawText("Use WASD to pan camera", 20, 45, 14, DARKGRAY);

    DrawText(
        std::format(
            "({}, {})\n{}\n{}",
            mouseWorld.x, mouseWorld.y, xd, xdd
        ).c_str()
        , 20, 45, 14, DARKGRAY
    );

    EndDrawing();
}
