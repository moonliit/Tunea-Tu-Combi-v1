#include "app/editor/EditorScene.hpp"
#include "input/EditorInput.hpp"
#include "math/GridCoord.hpp"
#include "math/Vector2D.hpp"
#include "ui/UiControl.hpp"
#include <optional>
#include <raylib.h>
using namespace Math;


EditorScene::EditorScene(int width, int height):
    m_screenWidth(width),
    m_screenHeight(height)
{
    // Initialize Camera2D
    m_camera = Camera2D(Vector2{0});
    m_camera.target = {
        static_cast<float>(width), // placeholder
        static_cast<float>(height)
    };
    m_camera.offset = {m_screenWidth / 2.f, m_screenHeight / 2.f};
    m_camera.rotation = 0.0f;
}

void EditorScene::ProcessInput(float dt) {
    // const EditorInputFrame input = m_input.Capture(
    //     dt,
    //     m_stateMachine.GetState(),
    //     m_playSession.GetActivationRegistry(),
    //     m_level.GetGrid(),
    //     m_camera,
    //     m_buildSession.GetSelectedPartType(),
    //     m_buildSession.HasRepositionedPart(),
    //     m_screenWidth,
    //     m_screenHeight
    // );
    //
    // if (input.toggleMode) {
    //     if (m_stateMachine.IsBuildMode()) {
    //         TransitionToPlayMode();
    //     } else if (m_stateMachine.IsPlayMode() ||
    //                m_stateMachine.IsViewMode()) {
    //         ResetToBuildMode();
    //     }
    // }
    // if (m_stateMachine.IsViewMode() && input.clearSelection) {
    //     ResetToBuildMode();
    // }
    //
    // if (m_stateMachine.IsPlayMode()) {
    //     if (input.activationAction.has_value()) {
    //         m_playSession.ActivateAction(input.activationAction.value());
    //     }
    // }
    //
    // if (input.editorAction.has_value() &&
    //     (m_stateMachine.IsBuildMode() ||
    //      m_stateMachine.IsViewMode() ||
    //      (m_stateMachine.IsPlayMode() &&
    //       input.editorAction == EditorAction::ReturnToBuild))) {
    //     switch (input.editorAction.value()) {
    //         case EditorAction::MoveUp:
    //             m_level.GetBuildModel().ShiftContents({0, -1});
    //             break;
    //         case EditorAction::MoveRight:
    //             m_level.GetBuildModel().ShiftContents({1, 0});
    //             break;
    //         case EditorAction::MoveDown:
    //             m_level.GetBuildModel().ShiftContents({0, 1});
    //             break;
    //         case EditorAction::MoveLeft:
    //             m_level.GetBuildModel().ShiftContents({-1, 0});
    //             break;
    //         case EditorAction::ClearBuild:
    //             if (m_buildSession.HasHandItem()) {
    //                 m_buildSession.ClearHand();
    //             } else {
    //                 m_level.GetBuildModel().Clear();
    //             }
    //             break;
    //         case EditorAction::StartPlay:
    //             TransitionToPlayMode();
    //             break;
    //         case EditorAction::ClearSelection:
    //             m_buildSession.ClearHand();
    //             break;
    //         case EditorAction::EnterView:
    //             m_stateMachine.EnterView();
    //             break;
    //         case EditorAction::ReturnToBuild:
    //             ResetToBuildMode();
    //             break;
    //     }
    // }
    //
    // m_camera.target.x += static_cast<float>(input.cameraMovement.x);
    // m_camera.target.y += static_cast<float>(input.cameraMovement.y);
    //
    // if (m_stateMachine.IsBuildMode()) {
    //     if (input.clearSelection) {
    //         m_buildSession.ClearHand();
    //     }
    //     if (input.discardRepositionedPart) {
    //         m_buildSession.DiscardRepositionedPart();
    //     }
    //     if (input.pickupPart && input.interactionCoord.has_value()) {
    //         m_buildSession.PickUpPart(input.interactionCoord.value());
    //     }
    //     if (input.rotatePlacedPart && input.interactionCoord.has_value()) {
    //         m_buildSession.RotatePart(input.interactionCoord.value());
    //     }
    //     if (input.rotatePart) {
    //         m_buildSession.RotateSelectedPart();
    //     }
    //     if (input.toolbarPart.has_value()) {
    //         const PartType partType = static_cast<PartType>(
    //             input.toolbarPart.value()
    //         );
    //         m_buildSession.SelectPartType(partType);
    //     } else if ((m_buildSession.GetSelectedPartType().has_value() ||
    //                 m_buildSession.HasRepositionedPart()) &&
    //                input.mouseScreenPosition.y <= 520.0f &&
    //                (input.placePart || input.removePart)) {
    //         const Vector2 mouseWorld = GetScreenToWorld2D(
    //             input.mouseScreenPosition.ToRayVec(),
    //             m_camera
    //         );
    //         auto gridCoordOpt = m_level.GetGrid().WorldToCoord(Math::Vector2D{mouseWorld.x, mouseWorld.y});
    //
    //         if (gridCoordOpt.has_value()) {
    //             GridCoord coord = gridCoordOpt.value();
    //
    //             if (input.placePart) {
    //                 if (m_buildSession.HasRepositionedPart()) {
    //                     m_buildSession.PlaceRepositionedPart(coord);
    //                 } else {
    //                     m_buildSession.PlaceSelectedPart(coord);
    //                 }
    //             }
    //             if (input.removePart) {
    //                 m_buildSession.RemovePart(coord);
    //             }
    //         }
    //
    //     }
    // }
}

void EditorScene::Update(float dt) {
    // if (m_stateMachine.IsPlayMode()) {
    //     m_playSession.Update(dt);
    // }
}

void EditorScene::Render() const {
    // BeginDrawing();
    // ClearBackground(RAYWHITE);
    //
    // m_worldRenderer.Render(
    //     m_camera,
    //     m_stateMachine.GetState(),
    //     m_level.GetGrid(),
    //     m_playSession,
    //     m_buildSession.GetSelectedPartType(),
    //     m_buildSession.GetSelectedOrientation(),
    //     m_buildSession.GetRepositionedPartType(),
    //     m_buildSession.GetRepositionedOrientation()
    // );
    // m_uiRenderer.Render(
    //     m_screenWidth,
    //     m_screenHeight,
    //     m_stateMachine.GetState(),
    //     m_buildSession,
    //     m_level.GetGrid(),
    //     m_camera,
    //     m_playSession.GetActivationRegistry()
    // );
    //
    // EndDrawing();
}
