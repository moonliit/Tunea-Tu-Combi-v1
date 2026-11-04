#include "game/Game.hpp"
#include "math/GridCoord.hpp"
#include "math/Vector2D.hpp"
#include <optional>
#include <raylib.h>

using namespace Math;

Game::Game(int width, int height, const std::string& title, LevelSpecs specs):
    m_screenWidth(width),
    m_screenHeight(height),
    m_gravity(9.8f * 40.0f),
    m_level(specs),
    m_buildSession(m_level.GetBuildModel()),
    m_playSession(
        m_level.GetBuildModel(),
        m_level.GetCatmullromPoints(),
        Vector2D{0.0f, m_gravity}
    )
{
    InitWindow(width, height, title.c_str());
    SetTargetFPS(60);

    // Initialize Camera2D
    m_camera = Camera2D(Vector2{0});
    m_camera.target = specs.initialCameraPos.ToRayVec();
    m_camera.offset = {m_screenWidth / 2.f, m_screenHeight / 2.f};
    m_camera.rotation = 0.0f;
    m_camera.zoom = 1.0f;

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
    const GameInputFrame input = m_input.Capture(
        dt,
        m_stateMachine.GetState(),
        m_playSession.GetActivationRegistry()
    );

    if (input.toggleMode) {
        if (m_stateMachine.IsBuildMode()) {
            TransitionToPlayMode();
        } else {
            ResetToBuildMode();
        }
    }

    if (m_stateMachine.IsPlayMode()) {
        if (input.activationAction.has_value()) {
            m_playSession.ActivateAction(input.activationAction.value());
        }
    }

    m_camera.target.x += static_cast<float>(input.cameraMovement.x);
    m_camera.target.y += static_cast<float>(input.cameraMovement.y);

    if (m_stateMachine.IsBuildMode()) {
        if (input.rotatePart) {
            m_buildSession.RotateSelectedPart();
        }
        if (input.toolbarPart.has_value()) {
            const PartType partType = static_cast<PartType>(
                input.toolbarPart.value()
            );
            m_buildSession.SelectPartType(partType);
        } else if (m_buildSession.GetSelectedPartType().has_value() &&
                   input.mouseScreenPosition.y <= 520.0f &&
                   (input.placePart || input.removePart)) {
            const Vector2 mouseWorld = GetScreenToWorld2D(
                input.mouseScreenPosition.ToRayVec(),
                m_camera
            );
            auto gridCoordOpt = m_level.GetGrid().WorldToCoord(Math::Vector2D{mouseWorld.x, mouseWorld.y});

            if (gridCoordOpt.has_value()) {
                GridCoord coord = gridCoordOpt.value();

                if (input.placePart) {
                    m_buildSession.PlaceSelectedPart(coord);
                }
                if (input.removePart) {
                    m_buildSession.RemovePart(coord);
                }
            }

        }
    }
}

void Game::Update(float dt) {
    if (m_stateMachine.IsPlayMode()) {
        m_playSession.Update(dt);
    }
}

void Game::TransitionToPlayMode() {
    m_stateMachine.EnterPlay();
    m_playSession.Start();
}

void Game::ResetToBuildMode() {
    m_stateMachine.EnterBuild();

    m_playSession.Reset(m_level.GetCatmullromPoints());
}

void Game::Render() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    m_worldRenderer.Render(
        m_camera,
        m_stateMachine.GetState(),
        m_level.GetGrid(),
        m_playSession,
        m_buildSession.GetSelectedPartType(),
        m_buildSession.GetSelectedOrientation()
    );
    m_uiRenderer.Render(
        m_screenWidth,
        m_screenHeight,
        m_stateMachine.GetState(),
        m_buildSession,
        m_level.GetGrid(),
        m_camera,
        m_playSession.GetActivationRegistry()
    );

    EndDrawing();
}
