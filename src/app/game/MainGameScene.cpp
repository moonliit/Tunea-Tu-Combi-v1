#include "app/game/MainGameScene.hpp"
#include "app/game/Level.hpp"
#include "math/GridCoord.hpp"
#include "math/Vector2D.hpp"
#include "ui/UiControl.hpp"
#include "app/AppContext.hpp"
#include <optional>
#include <raylib.h>
#include <algorithm>

using namespace Math;

namespace {

float CalculateGridFitZoom(
    int screenWidth,
    int screenHeight,
    const LevelDefinition& levelDef
) {
    const float gridWidth =
        static_cast<float>(levelDef.grid.dims.x) * levelDef.grid.cellSize;
    const float gridHeight =
        static_cast<float>(levelDef.grid.dims.y) * levelDef.grid.cellSize;
    const float availableHeight = static_cast<float>(screenHeight - 100);
    return std::min(
        1.0f,
        std::min(
            static_cast<float>(screenWidth - 40) / gridWidth,
            availableHeight / gridHeight
        )
    );
}

Vector2 CalculateGridTarget(const LevelDefinition& specs) {
    const float gridHeight =
        static_cast<float>(specs.grid.dims.y) * specs.grid.cellSize;
    return {
        static_cast<float>(specs.grid.worldPos.x),
        static_cast<float>(
            specs.grid.worldPos.y - gridHeight / 2.0f + specs.grid.cellSize
        )
    };
}

}

MainGameScene::MainGameScene(
    AppContext* appContext,
    const LevelDefinition& specs
):
    m_appContext(appContext),
    m_gravity(9.8f * 40.0f),
    m_level(specs),
    m_buildSession(m_level.GetBuildModel()),
    m_playSession(
        m_level.GetBuildModel(),
        m_level.GetTerrain(),
        Vector2D{0.0f, m_gravity}
    )
{
    // Initialize Camera2D
    m_camera = Camera2D(Vector2{0});
    const Vector2 automaticTarget = CalculateGridTarget(specs);
    const float minimumZoom = CalculateGridFitZoom(
        m_appContext->windowWidth,
        m_appContext->windowHeight,
        specs
    );
    const CameraDefinition cameraDefinition =
        specs.initialCamera.value_or(CameraDefinition{
            .target = {
                automaticTarget.x,
                automaticTarget.y
            },
            .zoom = minimumZoom
        });
    m_camera.target = {
        static_cast<float>(cameraDefinition.target.x),
        static_cast<float>(cameraDefinition.target.y)
    };
    m_camera.offset = {m_appContext->windowWidth / 2.f, m_appContext->windowWidth / 2.f};
    m_camera.rotation = 0.0f;
    m_camera.zoom = std::max(minimumZoom, cameraDefinition.zoom);
    m_editorCameraTarget = m_camera.target;
    m_editorCameraZoom = m_camera.zoom;
}

void MainGameScene::ProcessInput(float dt) {
    const GameInputFrame input = m_input.Capture(
        dt,
        m_stateMachine.GetState(),
        m_playSession.GetActivationRegistry(),
        m_level.GetGrid(),
        m_camera,
        m_buildSession.GetSelectedPartType(),
        m_buildSession.HasRepositionedPart(),
        m_appContext->windowWidth,
        m_appContext->windowHeight
    );

    if (input.toggleMode) {
        if (m_stateMachine.IsBuildMode()) {
            TransitionToPlayMode();
        } else if (m_stateMachine.IsPlayMode() ||
                   m_stateMachine.IsViewMode()) {
            ResetToBuildMode();
        }
    }
    if (m_stateMachine.IsViewMode() && input.clearSelection) {
        ResetToBuildMode();
    }

    if (m_stateMachine.IsPlayMode()) {
        if (input.activationAction.has_value()) {
            m_playSession.ActivateAction(input.activationAction.value());
        }
    }

    if (input.editorAction.has_value() &&
        (m_stateMachine.IsBuildMode() ||
         m_stateMachine.IsViewMode() ||
         (m_stateMachine.IsPlayMode() &&
          input.editorAction == EditorAction::ReturnToBuild))) {
        switch (input.editorAction.value()) {
            case EditorAction::MoveUp:
                m_level.GetBuildModel().ShiftContents({0, -1});
                break;
            case EditorAction::MoveRight:
                m_level.GetBuildModel().ShiftContents({1, 0});
                break;
            case EditorAction::MoveDown:
                m_level.GetBuildModel().ShiftContents({0, 1});
                break;
            case EditorAction::MoveLeft:
                m_level.GetBuildModel().ShiftContents({-1, 0});
                break;
            case EditorAction::ClearBuild:
                if (m_buildSession.HasHandItem()) {
                    m_buildSession.ClearHand();
                } else {
                    m_level.GetBuildModel().Clear();
                }
                break;
            case EditorAction::StartPlay:
                TransitionToPlayMode();
                break;
            case EditorAction::ClearSelection:
                m_buildSession.ClearHand();
                break;
            case EditorAction::EnterView:
                m_stateMachine.EnterView();
                break;
            case EditorAction::ReturnToBuild:
                ResetToBuildMode();
                break;
        }
    }

    m_camera.target.x += static_cast<float>(input.cameraMovement.x);
    m_camera.target.y += static_cast<float>(input.cameraMovement.y);

    if (m_stateMachine.IsBuildMode()) {
        if (input.clearSelection) {
            m_buildSession.ClearHand();
        }
        if (input.discardRepositionedPart) {
            m_buildSession.DiscardRepositionedPart();
        }
        if (input.pickupPart && input.interactionCoord.has_value()) {
            m_buildSession.PickUpPart(input.interactionCoord.value());
        }
        if (input.rotatePlacedPart && input.interactionCoord.has_value()) {
            m_buildSession.RotatePart(input.interactionCoord.value());
        }
        if (input.rotatePart) {
            m_buildSession.RotateSelectedPart();
        }
        if (input.toolbarPart.has_value()) {
            const PartType partType = static_cast<PartType>(
                input.toolbarPart.value()
            );
            m_buildSession.SelectPartType(partType);
        } else if ((m_buildSession.GetSelectedPartType().has_value() ||
                    m_buildSession.HasRepositionedPart()) &&
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
                    if (m_buildSession.HasRepositionedPart()) {
                        m_buildSession.PlaceRepositionedPart(coord);
                    } else {
                        m_buildSession.PlaceSelectedPart(coord);
                    }
                }
                if (input.removePart) {
                    m_buildSession.RemovePart(coord);
                }
            }

        }
    }
}

void MainGameScene::Update(float dt) {
    if (m_stateMachine.IsPlayMode()) {
        m_playSession.Update(dt);
    }
}

void MainGameScene::TransitionToPlayMode() {
    m_stateMachine.EnterPlay();
    m_playSession.Start();
}

void MainGameScene::ResetToBuildMode() {
    if (!m_stateMachine.IsBuildMode()) {
        m_stateMachine.EnterBuild();
    }
    ResetEditorCamera();

    m_playSession.Reset(m_level.GetTerrain());
}

void MainGameScene::ResetEditorCamera() {
    m_camera.target = m_editorCameraTarget;
    m_camera.zoom = m_editorCameraZoom;
}

void MainGameScene::Render() const {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    m_worldRenderer.Render(
        m_camera,
        m_stateMachine.GetState(),
        m_level.GetGrid(),
        m_playSession,
        m_buildSession.GetSelectedPartType(),
        m_buildSession.GetSelectedOrientation(),
        m_buildSession.GetRepositionedPartType(),
        m_buildSession.GetRepositionedOrientation()
    );
    m_uiRenderer.Render(
        m_appContext->windowWidth,
        m_appContext->windowHeight,
        m_stateMachine.GetState(),
        m_buildSession,
        m_level.GetGrid(),
        m_camera,
        m_playSession.GetActivationRegistry()
    );

    EndDrawing();
}
