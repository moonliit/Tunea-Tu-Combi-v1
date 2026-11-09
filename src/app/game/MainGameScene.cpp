#include "app/game/MainGameScene.hpp"
#include "ui/UiButton.hpp"
#include "app/game/Level.hpp"
#include "math/GridCoord.hpp"
#include "math/Vector2D.hpp"
#include "ui/UiControl.hpp"
#include "app/AppContext.hpp"
#include "app/AudioController.hpp"
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
    m_isCustomLevel(specs.levelNumber < 0),
    m_level(specs),
    m_buildSession(m_level.GetBuildModel()),
    m_playSession(
        m_level.GetBuildModel(),
        m_level.GetTerrain(),
        Vector2D{0.0f, m_gravity}
    )
{
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
    m_camera.offset = {m_appContext->windowWidth / 2.f, m_appContext->windowHeight / 2.f};
    m_camera.rotation = 0.0f;
    m_camera.zoom = std::max(minimumZoom, cameraDefinition.zoom);
    m_editorCameraTarget = m_camera.target;
    m_editorCameraZoom = m_camera.zoom;
}

void MainGameScene::ProcessInput(float dt) {
    if (m_isCustomLevel && m_stateMachine.IsBuildMode()) {
        const Rectangle designButton = UiButton::CenteredAbove(
            UiButton::PlaySideButton(
                m_appContext->windowWidth, m_appContext->windowHeight
            ),
            52.0f,
            32.0f,
            8.0f
        );
        if (UiButton::WasClicked(designButton)) {
            m_appContext->audioController->SetFanActive(false);
            m_appContext->audioController->SetWheelActive(false);
            m_appContext->returnToCustomDesign = true;
            m_appContext->sceneRequester->RequestSceneChange({SceneId::MainMenu});
            return;
        }
    }

    const Rectangle menuButton = UiButton::AtTopRight(
        m_appContext->windowWidth, 12.0f, 20.0f, 78.0f, 32.0f
    );
    if (UiButton::WasClicked(menuButton)) {
        m_appContext->audioController->SetFanActive(false);
        m_appContext->audioController->SetWheelActive(false);
        m_appContext->audioController->ReturnToSelector();
        m_appContext->sceneRequester->RequestSceneChange({SceneId::MainMenu});
        return;
    }

    const GameInputFrame input = m_input.Capture(GameInputContext{
        .deltaTime = dt,
        .state = m_stateMachine.GetState(),
        .activationRegistry = m_playSession.GetActivationRegistry(),
        .grid = m_level.GetGrid(),
        .camera = m_camera,
        .selectedPartType = m_buildSession.GetSelectedPartType(),
        .hasRepositionedPart = m_buildSession.HasRepositionedPart(),
        .screenWidth = m_appContext->windowWidth,
        .screenHeight = m_appContext->windowHeight
    });

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
            if (IsWindSailAction(input.activationAction.value())) {
                m_appContext->audioController->PlaySail();
            }
            if (input.activationAction == ActivationAction::DriverHonk) {
                m_appContext->audioController->PlayDriverHorn();
            }
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
            m_appContext->audioController->PlayPartSelect();
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
                        if (m_buildSession.PlaceRepositionedPart(coord)) {
                            m_appContext->audioController->PlayPartPlace();
                        }
                    } else {
                        if (m_buildSession.PlaceSelectedPart(coord)) {
                            m_appContext->audioController->PlayPartPlace();
                        }
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
        const auto& parts = m_level.GetGrid().GetActiveParts();
        if (!parts.empty()) {
            Vector2 center{0.0f, 0.0f};
            bool entireCarPassedFinish = true;
            for (const Part* part : parts) {
                const auto position = part->GetRenderState(m_level.GetGrid()).position;
                center.x += static_cast<float>(position.x);
                center.y += static_cast<float>(position.y);
                entireCarPassedFinish = entireCarPassedFinish &&
                    position.x > m_level.GetDefinition().goal.position.x;
            }
            m_camera.target = {
                center.x / parts.size(),
                center.y / parts.size()
            };
            bool fanActive = false;
            for (ActivationAction action : m_playSession.GetActivationRegistry().GetVisibleActions()) {
                fanActive = fanActive || (
                    IsFanAction(action) &&
                    m_playSession.GetActivationRegistry().IsActive(action)
                );
            }
            m_appContext->audioController->SetFanActive(fanActive);
            const bool hasWheel = std::any_of(
                parts.begin(),
                parts.end(),
                [](const Part* part) {
                    return part->GetType() == PartType::CircleWheel;
                }
            );
            m_appContext->audioController->SetWheelActive(hasWheel);
            if (entireCarPassedFinish && !m_levelCompleted) {
                m_levelCompleted = true;
                m_appContext->audioController->SetFanActive(false);
                m_appContext->audioController->SetWheelActive(false);
                if (!m_isCustomLevel) {
                    m_appContext->completedLevel = m_level.GetDefinition().levelNumber;
                }
                m_appContext->audioController->ReturnToSelector();
                m_appContext->sceneRequester->RequestSceneChange({SceneId::MainMenu});
            }
        }
    }
}

void MainGameScene::TransitionToPlayMode() {
    if (!m_level.GetGrid().ContainsPartType(PartType::Driver)) return;
    const auto& parts = m_level.GetGrid().GetActiveParts();
    const bool hasWheel = std::any_of(
        parts.begin(),
        parts.end(),
        [](const Part* part) {
            return part->GetType() == PartType::CircleWheel;
        }
    );
    m_appContext->audioController->SetWheelActive(hasWheel);
    const auto placements = m_level.GetBuildModel().GetPlacements();
    if (!placements.empty()) {
        Vector2 center{0.0f, 0.0f};
        for (const auto& placement : placements) {
            const auto position = m_level.GetGrid().CoordToWorld(placement.coord);
            center.x += static_cast<float>(position.x);
            center.y += static_cast<float>(position.y);
        }
        m_camera.target = {
            center.x / placements.size(),
            center.y / placements.size()
        };
    }
    m_stateMachine.EnterPlay();
    m_playSession.Start();
}

void MainGameScene::ResetToBuildMode() {
    m_appContext->audioController->SetFanActive(false);
    m_appContext->audioController->SetWheelActive(false);
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
    BeginMode2D(m_camera);
    const auto& finish = m_level.GetDefinition().goal.position;
    const Vector2 pole{static_cast<float>(finish.x), static_cast<float>(finish.y)};
    DrawLineEx({pole.x, pole.y - 100.0f}, pole, 5.0f, DARKGRAY);
    DrawTriangle(
        {pole.x, pole.y - 100.0f},
        {pole.x, pole.y - 64.0f},
        {pole.x + 65.0f, pole.y - 82.0f},
        RED
    );
    DrawTriangleLines(
        {pole.x, pole.y - 100.0f},
        {pole.x, pole.y - 64.0f},
        {pole.x + 65.0f, pole.y - 82.0f},
        MAROON
    );
    EndMode2D();
    m_uiRenderer.Render(
        m_appContext->windowWidth,
        m_appContext->windowHeight,
        m_stateMachine.GetState(),
        m_buildSession,
        m_level.GetGrid(),
        m_camera,
        m_playSession.GetActivationRegistry()
    );
    UiButton::Draw(
        UiButton::AtTopRight(
            m_appContext->windowWidth, 12.0f, 20.0f, 78.0f, 32.0f
        ),
        "Menú",
        DARKGRAY,
        WHITE,
        16,
        false,
        11,
        false
    );
    if (m_isCustomLevel && m_stateMachine.IsBuildMode()) {
        UiButton::Draw(
            UiButton::CenteredAbove(
                UiButton::PlaySideButton(
                    m_appContext->windowWidth, m_appContext->windowHeight
                ),
                52.0f,
                32.0f,
                8.0f
            ),
            "v",
            DARKGRAY,
            WHITE,
            16
        );
    }

    EndDrawing();
}
