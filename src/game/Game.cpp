#include "game/Game.hpp"
#include "game/Part.hpp"
#include "physics/Box2DPhysics.hpp"
#include <cmath>

using namespace Math;

Game::Game(int width, int height, const std::string& title) :
    m_screenWidth(width),
    m_screenHeight(height),
    m_window(width, height, title),
    m_currentState(GameState::Edit),
    m_grid(10, 10, 40.0f), // 40px cell size
    m_gravity(9.8f * 40.0f)
{
    m_window.SetTargetFPS(60);

    // Initialize Camera2D
    m_camera = raylib::Camera2D(Vector2{0});
    m_camera.target = {400.0f, 300.0f};
    m_camera.offset = {static_cast<float>(m_screenWidth) / 2.0f, static_cast<float>(m_screenHeight) / 2.0f};
    m_camera.rotation = 0.0f;
    m_camera.zoom = 1.0f;

    // Initialize Physics World
    m_physics = std::make_unique<Box2DWorld>(Vector2D{0.0f, m_gravity});

    // Setup Terrain Control Points
    m_terrainControlPoints = {
        {50.0f, 450.0f},
        {250.0f, 480.0f},
        {450.0f, 400.0f},
        {650.0f, 520.0f},
        {750.0f, 520.0f}
    };
    m_terrainVertices = m_physics->CreateTerrainChain(m_terrainControlPoints, 15);
}

void Game::Run() {
    while (!m_window.ShouldClose()) {
        float dt = raylib::Window::GetFrameTime();
        if (dt > 0.033f) dt = 0.033f;

        ProcessInput(dt);
        Update(dt);
        Render();
    }
}

void Game::ProcessInput(float dt) {
    // 1. Toggle State (ENTER)
    if (raylib::Keyboard::IsKeyPressed(KEY_ENTER)) {
        if (m_currentState == GameState::Edit) {
            TransitionToPlayMode();
        } else {
            ResetToEditMode();
        }
    }

    // 2. Camera Panning via WASD
    float cameraSpeed = 300.0f * dt;
    if (raylib::Keyboard::IsKeyDown(KEY_W)) m_camera.target.y -= cameraSpeed;
    if (raylib::Keyboard::IsKeyDown(KEY_S)) m_camera.target.y += cameraSpeed;
    if (raylib::Keyboard::IsKeyDown(KEY_A)) m_camera.target.x -= cameraSpeed;
    if (raylib::Keyboard::IsKeyDown(KEY_D)) m_camera.target.x += cameraSpeed;

    // 3. Edit Mode Toolbar & Grid Placement
    if (m_currentState == GameState::Edit) {
        Vector2 mouseScreen = GetMousePosition();

        // Toolbar UI Click Checks (Bottom Bar)
        if (mouseScreen.y > 520.0f) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (mouseScreen.x >= 20 && mouseScreen.x <= 120) m_currentPartType = PartType::SquareBlock;
                if (mouseScreen.x >= 140 && mouseScreen.x <= 240) m_currentPartType = PartType::CircleWheel;
            }
        } else {
            // Workspace Grid Interaction
            Vector2 mouseWorld = m_camera.GetScreenToWorld(mouseScreen);
            int gx = static_cast<int>(floor(mouseWorld.x / m_grid.GetCellSize()));
            int gy = static_cast<int>(floor(mouseWorld.y / m_grid.GetCellSize()));

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                m_grid.PlacePart(gx, gy, m_currentPartType);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                m_grid.RemovePart(gx, gy);
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

    float cellSize = m_grid.GetCellSize();
    for (auto& part : m_grid.GetActiveParts()) {
        part->SetPhysicsBody(cellSize, m_physics);
    }
}

void Game::ResetToEditMode() {
    m_currentState = GameState::Edit;

    // Re-initialize physics world for clean slate
    for (auto& part : m_grid.GetActiveParts()) {
        part->ResetPhysicsBody();
    }
    m_physics = std::make_unique<Box2DWorld>(Vector2D{0.0f, m_gravity});
    m_terrainVertices = m_physics->CreateTerrainChain(m_terrainControlPoints, 15);
}

void Game::Render() {
    m_window.BeginDrawing();
    m_window.ClearBackground(raylib::Color::RayWhite());

    // --- World Space Rendering ---
    m_camera.BeginMode();

    // Draw Terrain
    for (size_t i = 0; i < m_terrainVertices.size() - 1; ++i) {
        m_terrainVertices[i].ToRaylib().DrawLine(
            m_terrainVertices[i + 1].ToRaylib(),
            4.0f,
            DARKGRAY
        );
    }

    float cellSize = m_grid.GetCellSize();

    // If in Edit Mode, render grid items as blueprints
    if (m_currentState == GameState::Edit) {
        for (const auto& part : m_grid.GetActiveParts()) {
            part->Render(cellSize);
        }

        // Draw Ghost Preview under mouse cursor
        Vector2 mouseScreen = GetMousePosition();
        if (mouseScreen.y <= 520.0f) {
            Vector2 mouseWorld = m_camera.GetScreenToWorld(mouseScreen);
            int gx = static_cast<int>(floor(mouseWorld.x / cellSize));
            int gy = static_cast<int>(floor(mouseWorld.y / cellSize));
            float previewX = gx * cellSize;
            float previewY = gy * cellSize;

            if (m_currentPartType == PartType::SquareBlock) {
                DrawRectangle(previewX, previewY, cellSize, cellSize, Fade(BLUE, 0.2f));
                DrawRectangleLines(previewX, previewY, cellSize, cellSize, BLUE);
            } else {
                DrawCircle(previewX + cellSize / 2, previewY + cellSize / 2, cellSize / 2, Fade(GREEN, 0.2f));
                DrawCircleLines(previewX + cellSize / 2, previewY + cellSize / 2, cellSize / 2, GREEN);
            }
        }
    }
    // If in Play Mode, render active physics bodies
    else if (m_currentState == GameState::Play) {
        for (const auto& part : m_grid.GetActiveParts()) {
            part->Render(cellSize);
        }
    }

    m_camera.EndMode();

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
    DrawText("Use WASD to pan camera", 20, 45, 14, DARKGRAY);

    m_window.EndDrawing();
}
