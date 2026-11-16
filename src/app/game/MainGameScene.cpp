#include "app/game/MainGameScene.hpp"
#include "ui/UiButton.hpp"
#include "app/game/Level.hpp"
#include "app/game/PartDefinition.hpp"
#include <print>
#include "math/GridCoord.hpp"
#include "math/Vector2D.hpp"
#include "ui/UiControl.hpp"
#include "app/AppContext.hpp"
#include "app/AudioController.hpp"
#include "app/CustomLevelCatalog.hpp"
#include "rendering/Background.hpp"
#include "rendering/PassengerRenderer.hpp"
#include <optional>
#include <raylib.h>
#include <algorithm>
#include <cmath>
#include <random>
#include <string>

using namespace Math;

namespace {

// how far past the level the camera keeps following the driver
constexpr float MapCameraMargin = 500.0f;
constexpr float RoutePreviewHoldSeconds = 0.2f;
constexpr float PreviewZoomOutSeconds = 0.78f;
constexpr float PreviewOverviewHoldSeconds = 0.7f;
constexpr float PreviewZoomInSeconds = 0.95f;
constexpr float PreviewSplitZoomOutSeconds = 0.62f;
constexpr float PreviewSplitHoldSeconds = 0.35f;
constexpr float PreviewSplitPanSeconds = 0.55f;
constexpr float PreviewSplitZoomInSeconds = 0.85f;

Rectangle ResultsReturnButton(int width, int height) {
    return UiButton::CenteredAtBottom(width, height, 210.0f, 64.0f, 20.0f);
}

struct ExitConfirmDialogLayout {
    Rectangle dialog;
    Rectangle cancel;
    Rectangle confirm;
};

ExitConfirmDialogLayout GetExitConfirmDialogLayout(int width, int height) {
    const Rectangle dialog{
        (width - 520.0f) / 2.0f,
        (height - 286.0f) / 2.0f,
        520.0f,
        286.0f
    };
    return {
        dialog,
        Rectangle{dialog.x + 24.0f, dialog.y + 196.0f, 186.0f, 80.0f},
        Rectangle{dialog.x + dialog.width - 210.0f, dialog.y + 196.0f, 186.0f, 80.0f}
    };
}

void DrawExitConfirmAction(
    Rectangle bounds,
    const UiButtonSkin& skin,
    const char* label,
    bool enabled
) {
    const bool hovered = enabled && UiButton::IsHovered(bounds);
    const bool pressed = hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    UiButton::DrawTextureFit(
        UiButton::ResolveButtonTexture(skin, enabled, hovered, pressed),
        Rectangle{bounds.x, bounds.y, bounds.width, 54.0f}
    );
    DrawTextFont(
        label,
        static_cast<int>(bounds.x + (bounds.width - MeasureTextFont(label, 16)) / 2.0f),
        static_cast<int>(bounds.y + 62.0f),
        16,
        enabled ? (hovered ? GOLD : WHITE) : Color{190, 201, 203, 255}
    );
}

void DrawScoreValue(
    const std::string& value,
    const UiTextures& textures,
    float right,
    float top,
    float maxWidth
) {
    constexpr float glyphHeight = 58.0f;
    constexpr float spacing = 2.0f;
    const auto& glyphFor = [&](char character) -> const Texture2D& {
        return character == '-'
            ? textures.scoreMinus
            : textures.scoreDigits[static_cast<std::size_t>(character - '0')];
    };
    float naturalWidth = 0.0f;
    for (char character : value) {
        const Texture2D& glyph = glyphFor(character);
        naturalWidth += glyphHeight * glyph.width / glyph.height + spacing;
    }
    if (naturalWidth <= 0.0f) return;
    naturalWidth -= spacing;
    const float scale = std::min(1.0f, maxWidth / naturalWidth);
    const float drawnHeight = glyphHeight * scale;
    float x = right - naturalWidth * scale;
    for (char character : value) {
        const Texture2D& glyph = glyphFor(character);
        const float drawnWidth = drawnHeight * glyph.width / glyph.height;
        DrawTexturePro(glyph,
            Rectangle{0.0f, 0.0f,
                static_cast<float>(glyph.width), static_cast<float>(glyph.height)},
            Rectangle{x, top + (glyphHeight - drawnHeight) / 2.0f,
                drawnWidth, drawnHeight},
            Vector2{}, 0.0f, WHITE);
        x += drawnWidth + spacing * scale;
    }
}

Rectangle DesignLevelButton(int width, int height) {
    const Rectangle playButton = UiButton::PlaySideButton(width, height);
    return Rectangle{
        playButton.x,
        playButton.y - 98.0f,
        playButton.width,
        90.0f
    };
}

float RoutePreviewProgress(float time) {
    const float t = std::clamp(time, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

void MovePreviewCamera(
    Camera2D& camera,
    Vector2 from,
    float fromZoom,
    Vector2 to,
    float toZoom,
    float progress
) {
    const float t = RoutePreviewProgress(progress);
    camera.target = {
        std::lerp(from.x, to.x, t),
        std::lerp(from.y, to.y, t)
    };
    camera.zoom = std::lerp(fromZoom, toZoom, t);
}

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

Vector2 ClampToBounds(Vector2 point, const AABB& bounds) {
    return {
        std::clamp(
            point.x,
            static_cast<float>(bounds.min.x),
            static_cast<float>(bounds.max.x)
        ),
        std::clamp(
            point.y,
            static_cast<float>(bounds.min.y),
            static_cast<float>(bounds.max.y)
        )
    };
}

// El chofer puede ser una pieza propia o ir dentro de otra (GetContainedPart).
bool IsOrContainsDriver(const Part* part) {
    if (part->GetType() == PartType::Driver) return true;
    const Part* contained = part->GetContainedPart();
    return contained && contained->GetType() == PartType::Driver;
}

struct TrackedPart {
    const Part* part;
    PartRenderState state;
};

void CollectPart(
    const Part& part,
    const EditorGrid& grid,
    std::vector<TrackedPart>& output
) {
    output.push_back({&part, part.GetRenderState(grid)});
    if (part.HasContainedPart()) {
        CollectPart(*part.GetContainedPart(), grid, output);
    }
}

std::vector<TrackedPart> CollectParts(const EditorGrid& grid) {
    std::vector<TrackedPart> output;
    output.reserve(grid.GetPartCount());
    for (const Part* part : grid.GetActiveParts()) {
        CollectPart(*part, grid, output);
    }
    return output;
}

const TrackedPart* FindTrackedPart(
    const std::vector<TrackedPart>& parts,
    PartId id
) {
    const auto iterator = std::find_if(
        parts.begin(), parts.end(),
        [id](const TrackedPart& part) { return part.part->GetId() == id; }
    );
    return iterator == parts.end() ? nullptr : &*iterator;
}

double HorizontalRadius(const PartRenderState& state) {
    return state.type == PartType::Fan
        ? state.cellSize * 0.75
        : state.cellSize * 0.5;
}

Rectangle GhostPreviewButton() {
    return {20.0f, 122.0f, 230.0f, 32.0f};
}

Rectangle GhostPanel() {
    return {14.0f, 94.0f, 350.0f, 126.0f};
}

Rectangle GhostPauseButton() {
    return {258.0f, 122.0f, 100.0f, 32.0f};
}

Rectangle GhostRestartButton() {
    return {20.0f, 160.0f, 110.0f, 30.0f};
}

Rectangle GhostCompeteButton() {
    return {138.0f, 160.0f, 220.0f, 30.0f};
}

bool GhostCanBeRequested(GhostLoadStatus status) {
    return status == GhostLoadStatus::Loading ||
        status == GhostLoadStatus::Available ||
        status == GhostLoadStatus::LoadingRecording ||
        status == GhostLoadStatus::Ready;
}

}

MainGameScene::MainGameScene(
    AppContext* appContext,
    const LevelDefinition& specs
):
    m_appContext(appContext),
    m_gravity(9.8f * 40.0f),
    m_isEditableDraft(specs.levelNumber < 0 && !specs.publicId.has_value()),
    m_levelId(specs.GetLeaderboardId()),
    m_level(specs),
    m_mapBounds(m_level.GetContentBounds(MapCameraMargin)),
    m_buildSession(m_level.GetBuildModel()),
    m_playSession(
        m_level.GetBuildModel(),
        m_level.GetTerrain(),
        Vector2D{0.0f, m_gravity}
    ),
    m_worldRenderer(*appContext->textureLibrary, GetLevelBackground(specs.levelNumber))
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
    m_routePreviewStart = {
        static_cast<float>(specs.goal.position.x),
        static_cast<float>(specs.goal.position.y)
    };
    const Math::AABB content = m_level.GetContentBounds();
    const float contentWidth = std::max(1.0f,
        static_cast<float>(content.max.x - content.min.x));
    const float contentHeight = std::max(1.0f,
        static_cast<float>(content.max.y - content.min.y));
    const float availableWidth = std::max(1.0f,
        static_cast<float>(m_appContext->windowWidth) - 120.0f);
    const float availableHeight = std::max(1.0f,
        static_cast<float>(m_appContext->windowHeight) - 170.0f);
    const float widthFitZoom = availableWidth / contentWidth;
    const float heightFitZoom = availableHeight / contentHeight;
    m_routePreviewSplit = widthFitZoom < 0.32f &&
        widthFitZoom < heightFitZoom;
    m_routeOverviewZoom = std::min(m_editorCameraZoom,
        m_routePreviewSplit
            ? std::min({0.42f, availableWidth / (contentWidth * 0.55f),
                heightFitZoom})
            : std::min(widthFitZoom, heightFitZoom));
    const float centerX = static_cast<float>(content.min.x + content.max.x) / 2.0f;
    const float centerY = static_cast<float>(content.min.y + content.max.y) / 2.0f;
    m_routeOverviewTarget = {centerX, centerY};
    m_routeOverviewSecondTarget = m_routeOverviewTarget;
    if (m_routePreviewSplit) {
        const float leftX = static_cast<float>(content.min.x) + contentWidth * 0.25f;
        const float rightX = static_cast<float>(content.max.x) - contentWidth * 0.25f;
        const bool goalOnLeft = m_routePreviewStart.x <= m_editorCameraTarget.x;
        m_routeOverviewTarget.x = goalOnLeft ? leftX : rightX;
        m_routeOverviewSecondTarget.x = goalOnLeft ? rightX : leftX;
    }
    const float routeDistance = std::hypot(
        m_routePreviewStart.x - m_editorCameraTarget.x,
        m_routePreviewStart.y - m_editorCameraTarget.y
    );
    if (routeDistance > specs.grid.cellSize * 2.0f ||
        m_routeOverviewZoom < m_editorCameraZoom * 0.85f) {
        m_routePreviewDuration = m_routePreviewSplit
            ? PreviewSplitZoomOutSeconds + PreviewSplitHoldSeconds +
                PreviewSplitPanSeconds + PreviewSplitHoldSeconds +
                PreviewSplitZoomInSeconds
            : PreviewZoomOutSeconds + PreviewOverviewHoldSeconds +
                PreviewZoomInSeconds;
        m_routePreviewActive = true;
        m_camera.target = m_routePreviewStart;
    }
    // parallax starts from here
    m_worldRenderer.SetBackgroundOrigin(m_camera.target.x);
    m_passengerCollected.assign(m_level.GetDefinition().passengers.size(), false);
    m_ghostPlayback.Request(m_levelId, m_level.GetGrid().GetCellSize());
}

void MainGameScene::ProcessInput(float dt) {
    if (m_routePreviewActive) {
        if (m_isEditableDraft && IsKeyPressed(KEY_ESCAPE)) {
            ReturnToCustomLevelEditor();
            return;
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
            IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
            IsKeyPressed(KEY_ESCAPE)) {
            FinishRoutePreview();
        }
        return;
    }
    if (m_stateMachine.IsResultsMode()) {
        const Rectangle returnButton = ResultsReturnButton(
            m_appContext->windowWidth, m_appContext->windowHeight
        );
        if (IsKeyPressed(KEY_ENTER) || UiButton::WasClicked(returnButton)) {
            ReturnToLevelSelector();
        }
        return;
    }
    if (m_exitConfirmOpen) {
        const ExitConfirmDialogLayout dialog = GetExitConfirmDialogLayout(
            m_appContext->windowWidth, m_appContext->windowHeight
        );
        if (IsKeyPressed(KEY_ESCAPE) || UiButton::WasClicked(dialog.cancel)) {
            m_appContext->audioController->PlayUiClick();
            m_exitConfirmOpen = false;
            return;
        }
        if (IsKeyPressed(KEY_ENTER) || UiButton::WasClicked(dialog.confirm)) {
            m_exitConfirmOpen = false;
            ReturnToLevelSelector();
            return;
        }
        return;
    }

    if (m_isEditableDraft && m_stateMachine.IsBuildMode() &&
        IsKeyPressed(KEY_ESCAPE)) {
        ReturnToCustomLevelEditor();
        return;
    }

    if ((m_stateMachine.IsBuildMode() || m_stateMachine.IsPlayMode() ||
         m_stateMachine.IsViewMode()) && IsKeyPressed(KEY_ESCAPE)) {
        m_appContext->audioController->PlayUiClick();
        m_exitConfirmOpen = true;
        return;
    }
    if (m_stateMachine.IsBuildMode() && ProcessGhostControls()) return;
    if (m_isEditableDraft && m_stateMachine.IsBuildMode()) {
        const Rectangle designButton = DesignLevelButton(
            m_appContext->windowWidth, m_appContext->windowHeight
        );
        if (UiButton::WasClicked(designButton)) {
            ReturnToCustomLevelEditor();
            return;
        }
    }

    const Rectangle menuButton = UiButton::GameMenuButton(m_appContext->windowWidth);
    if (UiButton::WasClicked(menuButton)) {
        m_appContext->audioController->PlayUiClick();
        m_exitConfirmOpen = true;
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
            if (IsTntAction(input.activationAction.value())) {
                int amount = m_playSession.GetActiveTntAmount();
                m_appContext->audioController->PlayTntExplosion(amount);
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
        const EditorAction action = *input.editorAction;
        if (m_buildSession.HasRepositionedPart() &&
            (action == EditorAction::MoveUp ||
             action == EditorAction::MoveRight ||
             action == EditorAction::MoveDown ||
             action == EditorAction::MoveLeft)) {
            m_buildSession.ClearHand();
        }
        switch (action) {
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
                if (m_buildSession.HasRepositionedPart()) {
                    m_buildSession.DiscardRepositionedPart();
                    m_buildSession.ClearSelection();
                } else if (m_buildSession.HasHandItem()) {
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
            const bool currentlySelected =
                m_buildSession.GetSelectedPartType() == partType;
            const bool driverBoxExists =
                m_level.GetGrid().ContainsPartType(PartType::DriverBox);
            // Order of operations:
            // 1. Unselect current part
            // 2. If pressed number matches current selection, or driver box
            //    number is pressed but a driver box is already on grid,
            //    cancel operation (stay unselected)
            // 3. Otherwise select the new part
            if (currentlySelected ||
                (partType == PartType::DriverBox && driverBoxExists)) {
                m_buildSession.ClearSelection();
            } else {
                m_buildSession.SelectPartType(partType);
                m_appContext->audioController->PlayPartSelect();
            }
        }

        // Right-click removal works independently of any current selection
        // or ghost: clicking on a placed part always removes it.
        if (input.removePart && input.interactionCoord.has_value()) {
            m_buildSession.RemovePart(input.interactionCoord.value());
        }

        if ((m_buildSession.GetSelectedPartType().has_value() ||
                    m_buildSession.HasRepositionedPart()) &&
                   input.mouseScreenPosition.y <=
                       static_cast<float>(
                           m_appContext->windowHeight - UIBarHeight
                       ) &&
                   input.placePart) {
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
            }

        }

        // there can only be a single driver box per build
        // if one is already placed, clear any driver box selection
        if (m_buildSession.GetSelectedPartType() == PartType::DriverBox &&
            m_level.GetGrid().ContainsPartType(PartType::DriverBox)) {
            m_buildSession.ClearSelection();
        }
    }
}

void MainGameScene::Update(float dt) {
    const double frameMs = static_cast<double>(dt) * 1000.0;
    if (m_stateMachine.IsResultsMode()) {
        m_leaderboard.Poll(frameMs);
        m_playSession.Update(dt);
        m_ghostPlayback.Update(frameMs);
        const EditorGrid& runtimeGrid = m_playSession.GetRuntimeGrid();
        const auto& parts = runtimeGrid.GetActiveParts();
        {
            Vector2 center{};
            std::optional<Vector2> driverPosition;
            int partCount = 0;
            for (const Part* part : parts) {
                const auto position = part->GetRenderState(runtimeGrid).position;
                center.x += static_cast<float>(position.x);
                center.y += static_cast<float>(position.y);
                ++partCount;
                if (!driverPosition && IsOrContainsDriver(part)) {
                    driverPosition = Vector2{
                        static_cast<float>(position.x),
                        static_cast<float>(position.y)
                    };
                }
            }
            for (const auto& ejected : m_playSession.GetEjectedParts()) {
                if (!ejected) continue;
                const auto position = ejected->GetRenderState(runtimeGrid).position;
                center.x += static_cast<float>(position.x);
                center.y += static_cast<float>(position.y);
                ++partCount;
                if (ejected->GetType() == PartType::Driver) {
                    driverPosition = Vector2{
                        static_cast<float>(position.x),
                        static_cast<float>(position.y)
                    };
                }
            }
            if (partCount > 0) {
                m_camera.target = ClampToBounds(
                    driverPosition.value_or(Vector2{
                        center.x / static_cast<float>(partCount),
                        center.y / static_cast<float>(partCount)
                    }),
                    m_mapBounds
                );
            }
        }
        UpdateGoalParticles(dt);
        return;
    }
    if (m_routePreviewActive) {
        m_routePreviewElapsed += dt;
        if (m_routePreviewElapsed >=
            RoutePreviewHoldSeconds + m_routePreviewDuration) {
            FinishRoutePreview();
        } else if (m_routePreviewElapsed > RoutePreviewHoldSeconds) {
            float time = m_routePreviewElapsed - RoutePreviewHoldSeconds;
            const float zoomOut = m_routePreviewSplit
                ? PreviewSplitZoomOutSeconds : PreviewZoomOutSeconds;
            if (time < zoomOut) {
                MovePreviewCamera(m_camera, m_routePreviewStart,
                    m_editorCameraZoom, m_routeOverviewTarget,
                    m_routeOverviewZoom, time / zoomOut);
            } else {
                time -= zoomOut;
                const float firstHold = m_routePreviewSplit
                    ? PreviewSplitHoldSeconds : PreviewOverviewHoldSeconds;
                if (time < firstHold) {
                    m_camera.target = m_routeOverviewTarget;
                    m_camera.zoom = m_routeOverviewZoom;
                } else {
                    time -= firstHold;
                    if (m_routePreviewSplit && time < PreviewSplitPanSeconds) {
                        MovePreviewCamera(m_camera, m_routeOverviewTarget,
                            m_routeOverviewZoom, m_routeOverviewSecondTarget,
                            m_routeOverviewZoom, time / PreviewSplitPanSeconds);
                    } else {
                        if (m_routePreviewSplit) {
                            time -= PreviewSplitPanSeconds;
                        }
                        if (m_routePreviewSplit && time < PreviewSplitHoldSeconds) {
                            m_camera.target = m_routeOverviewSecondTarget;
                            m_camera.zoom = m_routeOverviewZoom;
                        } else {
                            if (m_routePreviewSplit) {
                                time -= PreviewSplitHoldSeconds;
                            }
                            const float zoomIn = m_routePreviewSplit
                                ? PreviewSplitZoomInSeconds : PreviewZoomInSeconds;
                            MovePreviewCamera(m_camera, m_routeOverviewSecondTarget,
                                m_routeOverviewZoom, m_editorCameraTarget,
                                m_editorCameraZoom, time / zoomIn);
                        }
                    }
                }
            }
        }
        UpdateGoalParticles(dt);
        return;
    }
    m_ghostPlayback.Update(frameMs);
    if (!GhostCanBeRequested(m_ghostPlayback.GetStatus())) {
        m_competeAgainstGhost = false;
    }
    if (m_stateMachine.IsPlayMode()) {
        m_playSession.Update(dt);
        m_elapsedMs += frameMs;
        for (const std::size_t ejectedPassengerId :
             m_playSession.ConsumeEjectedPassengerIds()) {
            const auto it = std::find_if(
                m_carriedPassengers.begin(),
                m_carriedPassengers.end(),
                [ejectedPassengerId](const CarriedPassenger& carried) {
                    return carried.id == ejectedPassengerId;
                }
            );
            if (it != m_carriedPassengers.end()) {
                m_carriedPassengers.erase(it);
            }
        }
        for (const auto [first, second] :
             m_playSession.ConsumeBrokenConnections()) {
            m_capture.RecordDetach(
                std::llround(m_elapsedMs), first, second
            );
        }
        const EditorGrid& runtimeGrid = m_playSession.GetRuntimeGrid();
        const auto& parts = runtimeGrid.GetActiveParts();
        const auto& passengerDefs = m_level.GetDefinition().passengers;
        for (std::size_t passengerIndex = 0;
             passengerIndex < passengerDefs.size(); ++passengerIndex) {
            if (m_passengerCollected[passengerIndex]) continue;
            const auto& passenger = passengerDefs[passengerIndex];
            bool nearVehicle = false;
            for (const Part* part : parts) {
                const auto position = part->GetRenderState(runtimeGrid).position;
                const float dx = static_cast<float>(position.x - passenger.position.x);
                const float dy = static_cast<float>(position.y - passenger.position.y);
                const float pickupRadius = runtimeGrid.GetCellSize() * 1.5f;
                if (dx * dx + dy * dy <= pickupRadius * pickupRadius) {
                    nearVehicle = true;
                    break;
                }
            }
            if (!nearVehicle) continue;

            std::vector<const Part*> availableBoxes;
            for (const Part* part : parts) {
                const bool holdsPassenger = std::any_of(
                    m_carriedPassengers.begin(), m_carriedPassengers.end(),
                    [part](const CarriedPassenger& carried) {
                        return carried.hostId == part->GetId();
                    }
                );
                if (part->GetType() == PartType::SquareBlock &&
                    !part->HasContainedPart() && !holdsPassenger &&
                    part->GetPhysicsBody()) {
                    availableBoxes.push_back(part);
                }
            }
            if (availableBoxes.empty()) continue;

            static std::mt19937 randomEngine{std::random_device{}()};
            std::uniform_int_distribution<std::size_t> chooseBox(
                0, availableBoxes.size() - 1
            );
            const Part* hostPart = availableBoxes[chooseBox(randomEngine)];
            hostPart->GetPhysicsBody()->AddMass(GetPartDefinition(PartType::Fan).mass);
            m_passengerCollected[passengerIndex] = true;
            m_carriedPassengers.push_back({
                passengerIndex,
                hostPart->GetId(),
                passenger.hasSkirt,
                passenger.face
            });
            m_playSession.RegisterCarriedPassenger(
                hostPart->GetId(),
                CarriedPassengerInfo{
                    passengerIndex,
                    passenger.hasSkirt,
                    passenger.face
                }
            );
            m_capture.RecordPassengerPickup(
                std::llround(m_elapsedMs), passengerIndex, hostPart->GetId()
            );
        }
        {
            std::optional<Vector2> driverPosition;
            Vector2 center{0.0f, 0.0f};
            int partCount = 0;
            for (const Part* part : parts) {
                const auto state = part->GetRenderState(runtimeGrid);
                center.x += static_cast<float>(state.position.x);
                center.y += static_cast<float>(state.position.y);
                ++partCount;
                if (part->HasContainedPart() && part->GetContainedPart() &&
                    part->GetContainedPart()->GetType() == PartType::Driver) {
                    const auto dstate = part->GetContainedPart()->GetRenderState(runtimeGrid);
                    driverPosition = Vector2{
                        static_cast<float>(dstate.position.x),
                        static_cast<float>(dstate.position.y)
                    };
                } else if (!driverPosition && part->GetType() == PartType::Driver) {
                    driverPosition = Vector2{
                        static_cast<float>(state.position.x),
                        static_cast<float>(state.position.y)
                    };
                }
            }
            for (const auto& ejected : m_playSession.GetEjectedParts()) {
                if (!ejected) continue;
                const auto estate = ejected->GetRenderState(runtimeGrid);
                center.x += static_cast<float>(estate.position.x);
                center.y += static_cast<float>(estate.position.y);
                ++partCount;
                if (ejected->GetType() == PartType::Driver) {
                    driverPosition = Vector2{
                        static_cast<float>(estate.position.x),
                        static_cast<float>(estate.position.y)
                    };
                }
            }
            // follow driver (including ejected) or vehicle center, clamped to map
            if (partCount > 0) {
                m_camera.target = ClampToBounds(
                    driverPosition.value_or(Vector2{
                        center.x / static_cast<float>(partCount),
                        center.y / static_cast<float>(partCount)
                    }),
                    m_mapBounds
                );
            }

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
            if (m_playSession.GetTntConsumedAmount() > 0) {
                int amount = m_playSession.GetTntConsumedAmount();
                m_appContext->audioController->PlayTntExplosion(amount);
                m_playSession.GetTntConsumedAmount() = 0;
            }
            if (m_playSession.GetDamageConsumed()) {
                m_appContext->audioController->PlayDamageEffect();
                m_playSession.GetDamageConsumed() = false;
            }
            if (m_playSession.GetScreamingConsumed()) {
                m_appContext->audioController->PlayScreamingEffect();
                m_playSession.GetScreamingConsumed() = false;
            }
            if (m_playSession.GetBreakageConsumed()) {
                m_appContext->audioController->PlayBreakageEffect();
                m_playSession.GetBreakageConsumed() = false;
            }
            for (const ExplosionVisual& visual : m_playSession.ConsumeExplosionVisuals()) {
                m_explosionVisuals.push_back(ExplosionVisualInstance{
                    .position = visual.position,
                    .radius = visual.radius,
                    .frame = 0,
                    .timer = 0.0f
                });
            }
            UpdateExplosionVisuals(dt);

            const float cellSize = runtimeGrid.GetCellSize();
            for (const DamageVisual& visual : m_playSession.ConsumeDamageVisuals()) {
                SpawnDamageStarBurst(
                    visual.position, cellSize,
                    3, 60.0f, 140.0f, 0.65f, 0.85f, 0.35f, 0.60f, 0.05f
                );
            }
            for (const DamageVisual& visual : m_playSession.ConsumeBreakageVisuals()) {
                SpawnDamageStarBurst(
                    visual.position, cellSize,
                    12, 90.0f, 220.0f, 0.75f, 1.05f, 0.45f, 0.85f, 0.18f
                );
            }
            UpdateDamageParticles(dt);
        }
        UpdateGoalWindow(frameMs);
        if (m_stateMachine.IsPlayMode()) {
            m_capture.RecordFrame(
                std::llround(m_elapsedMs), runtimeGrid
            );
        } else {
            m_playSession.ConsumeExplosionVisuals();
            m_playSession.ConsumeDamageVisuals();
            m_playSession.ConsumeBreakageVisuals();
        }
    }
    UpdateGoalParticles(dt);
}

void MainGameScene::TransitionToPlayMode() {
    if (!m_level.GetGrid().ContainsPartType(PartType::DriverBox)) return;
    m_elapsedMs = 0.0;
    m_levelCompleted = false;
    m_initialPartIds.clear();
    m_previousPartPositions.clear();
    m_goalWindow.Reset();
    m_statistics = {};
    m_score = {};
    m_explosionVisuals.clear();
    m_damageParticles.clear();
    const auto& parts = m_level.GetGrid().GetActiveParts();
    const bool hasWheel = std::any_of(
        parts.begin(),
        parts.end(),
        [](const Part* part) {
            return part->GetType() == PartType::CircleWheel;
        }
    );
    m_appContext->audioController->SetWheelActive(hasWheel);
    m_stateMachine.EnterPlay();
    m_runIdentity = m_appContext->identity.CaptureResultScope();
    const nlohmann::json draftSnapshot = m_isEditableDraft &&
        m_appContext->activeCustomLevel >= 0
        ? m_appContext->customLevels->GetDraftSnapshot(
            static_cast<std::size_t>(m_appContext->activeCustomLevel)
        )
        : nlohmann::json(nullptr);
    m_capture.Start(
        m_level.GetBuildModel(),
        m_level.GetDefinition().passengers.size(),
        m_levelId,
        m_runIdentity.combiId,
        m_level.GetDefinition().ToJson(),
        draftSnapshot
    );
    m_playSession.Start();
    if (m_competeAgainstGhost) m_ghostPlayback.StartCompetition();
    else m_ghostPlayback.StopAndRewind();
    auto scenario = m_appContext->audioController->GetCurrentScenario();
    m_appContext->audioController->SetCurrentScenario({
        Scenario::PlayLevel,
        scenario.levelIndex,
        scenario.levelEditorTrackName
    });

    const EditorGrid& runtimeGrid = m_playSession.GetRuntimeGrid();
    const std::vector<TrackedPart> trackedParts = CollectParts(runtimeGrid);
    m_previousPartPositions.clear();
    for (const TrackedPart& tracked : trackedParts) {
        m_initialPartIds.insert(tracked.part->GetId());
        m_previousPartPositions.emplace(
            tracked.part->GetId(),
            tracked.state.position
        );
    }
    m_statistics.initialParts = static_cast<std::int64_t>(
        m_initialPartIds.size()
    );
    m_capture.RecordFrame(0, runtimeGrid, true);
    const auto& runtimeParts = runtimeGrid.GetActiveParts();
    if (!runtimeParts.empty()) {
        Vector2 center{0.0f, 0.0f};
        std::optional<Vector2> driverPosition;
        int partCount = 0;
        for (const Part* part : runtimeParts) {
            const auto state = part->GetRenderState(runtimeGrid);
            center.x += static_cast<float>(state.position.x);
            center.y += static_cast<float>(state.position.y);
            ++partCount;
            if (part->HasContainedPart() && part->GetContainedPart() &&
                part->GetContainedPart()->GetType() == PartType::Driver) {
                const auto dstate = part->GetContainedPart()->GetRenderState(runtimeGrid);
                driverPosition = Vector2{
                    static_cast<float>(dstate.position.x),
                    static_cast<float>(dstate.position.y)
                };
            } else if (!driverPosition && part->GetType() == PartType::Driver) {
                driverPosition = Vector2{
                    static_cast<float>(state.position.x),
                    static_cast<float>(state.position.y)
                };
            }
        }
        for (const auto& ejected : m_playSession.GetEjectedParts()) {
            if (!ejected) continue;
            const auto estate = ejected->GetRenderState(runtimeGrid);
            center.x += static_cast<float>(estate.position.x);
            center.y += static_cast<float>(estate.position.y);
            ++partCount;
            if (ejected->GetType() == PartType::Driver) {
                driverPosition = Vector2{
                    static_cast<float>(estate.position.x),
                    static_cast<float>(estate.position.y)
                };
            }
        }
        if (partCount > 0) {
            m_camera.target = ClampToBounds(
                driverPosition.value_or(Vector2{
                    center.x / static_cast<float>(partCount),
                    center.y / static_cast<float>(partCount)
                }),
                m_mapBounds
            );
        }
    }
}

void MainGameScene::UpdateGoalWindow(double frameMs) {
    const EditorGrid& grid = m_playSession.GetRuntimeGrid();
    const std::vector<TrackedPart> parts = CollectParts(grid);
    std::vector<GoalPartSample> partSamples;
    partSamples.reserve(parts.size() + m_playSession.GetEjectedParts().size());
    for (const TrackedPart& tracked : parts) {
        const PartId id = tracked.part->GetId();
        const double currentX = static_cast<double>(tracked.state.position.x);
        const double currentY = static_cast<double>(tracked.state.position.y);
        const auto previous = m_previousPartPositions.find(id);
        const double previousX = previous == m_previousPartPositions.end()
            ? currentX
            : static_cast<double>(previous->second.x);
        const double previousY = previous == m_previousPartPositions.end()
            ? currentY
            : static_cast<double>(previous->second.y);
        partSamples.push_back({
            .id = id,
            .previousX = previousX,
            .previousY = previousY,
            .currentX = currentX,
            .currentY = currentY,
            .horizontalRadius = HorizontalRadius(tracked.state),
            .initial = m_initialPartIds.contains(id),
            .driver = tracked.part->GetType() == PartType::Driver
        });
    }
    for (const auto& ejected : m_playSession.GetEjectedParts()) {
        if (!ejected) continue;
        const PartRenderState state = ejected->GetRenderState(grid);
        const PartId id = ejected->GetId();
        const double currentX = static_cast<double>(state.position.x);
        const double currentY = static_cast<double>(state.position.y);
        const auto previous = m_previousPartPositions.find(id);
        const double previousX = previous == m_previousPartPositions.end()
            ? currentX
            : static_cast<double>(previous->second.x);
        const double previousY = previous == m_previousPartPositions.end()
            ? currentY
            : static_cast<double>(previous->second.y);
        partSamples.push_back({
            .id = id,
            .previousX = previousX,
            .previousY = previousY,
            .currentX = currentX,
            .currentY = currentY,
            .horizontalRadius = HorizontalRadius(state),
            .initial = false,
            .driver = ejected->GetType() == PartType::Driver
        });
    }
    std::vector<GoalPassengerSample> passengerSamples;
    passengerSamples.reserve(m_carriedPassengers.size());
    for (const CarriedPassenger& passenger : m_carriedPassengers) {
        passengerSamples.push_back({passenger.id, passenger.hostId});
    }
    const GoalWindowUpdate update = m_goalWindow.Update(
        m_elapsedMs - frameMs,
        m_elapsedMs,
        m_level.GetDefinition().goal.GetAABB(),
        partSamples,
        passengerSamples
    );
    if (update.completed && !m_levelCompleted) {
        CompleteRun();
    }

    m_previousPartPositions.clear();
    for (const TrackedPart& tracked : parts) {
        m_previousPartPositions.emplace(
            tracked.part->GetId(),
            tracked.state.position
        );
    }
    for (const auto& ejected : m_playSession.GetEjectedParts()) {
        if (!ejected) continue;
        const PartRenderState state = ejected->GetRenderState(grid);
        m_previousPartPositions.emplace(ejected->GetId(), state.position);
    }
}

void MainGameScene::CompleteRun() {
    m_levelCompleted = true;
    m_statistics.passengersCollected = static_cast<std::int64_t>(
        std::count(m_passengerCollected.begin(), m_passengerCollected.end(), true)
    );
    m_statistics.passengersDelivered = static_cast<std::int64_t>(
        m_goalWindow.GetDeliveredPassengers().size()
    );
    m_statistics.partsDelivered = static_cast<std::int64_t>(
        m_goalWindow.GetDeliveredParts().size()
    );
    m_statistics.driverTimeMs = std::max<std::int64_t>(
        0,
        std::llround(m_goalWindow.GetStartMs())
    );
    m_score = Scoring::CalculateScore(m_statistics);
    m_capture.RecordFrame(
        std::llround(m_elapsedMs),
        m_playSession.GetRuntimeGrid(),
        true
    );
    m_capture.Finish(
        std::llround(m_elapsedMs),
        m_statistics,
        m_score
    );
    if (m_isEditableDraft && m_appContext->activeCustomLevel >= 0) {
        m_appContext->customLevels->MarkCompleted(
            static_cast<std::size_t>(m_appContext->activeCustomLevel),
            m_capture.GetData()
        );
    }
    m_leaderboard.Submit(
        m_levelId,
        m_capture.GetAttemptId(),
        m_runIdentity,
        m_statistics,
        m_score,
        m_capture.GetData()
    );

    m_appContext->audioController->SetFanActive(false);
    m_appContext->audioController->SetWheelActive(false);
    if (m_level.GetDefinition().levelNumber > 0) {
        m_appContext->completedLevel = m_level.GetDefinition().levelNumber;
    }
    m_stateMachine.EnterResults();
}

void MainGameScene::UpdateGoalParticles(float dt) {
    const Math::AABB goalArea = m_level.GetDefinition().goal.GetAABB();
    const float width = static_cast<float>(goalArea.max.x - goalArea.min.x);
    const float height = static_cast<float>(goalArea.max.y - goalArea.min.y);
    if (width <= 0.0f || height <= 0.0f) return;

    if (m_goalParticles.empty()) {
        m_goalParticles.resize(36);
        for (GoalParticle& particle : m_goalParticles) {
            particle.life = -1.0f;
        }
    }

    const auto random01 = []() {
        return static_cast<float>(GetRandomValue(0, 1000)) / 1000.0f;
    };

    const float leewayX = 15.0f;
    const float leewayY = 30.0f;

    for (GoalParticle& particle : m_goalParticles) {
        particle.life -= dt;
        if (particle.life <= 0.0f) {
            particle.maxLife = 2.0f + random01() * 5.0f;
            particle.life = particle.maxLife;
            particle.position = {
                goalArea.min.x - leewayX + (width + 2 * leewayX) * random01(),
                goalArea.max.y - (height + leewayY) * random01() * 0.2f
            };
            particle.velocity = {
                (random01() - 0.5f) * 18.0f,
                -(20.0f + random01() * 35.0f)
            };
            particle.size = 1.5f + random01() * 3.0f;
            continue;
        }
        particle.position.x += particle.velocity.x * dt;
        particle.position.y += particle.velocity.y * dt;

        if (particle.position.x < goalArea.min.x - leewayX) particle.position.x = goalArea.max.x;
        if (particle.position.x > goalArea.max.x + leewayX) particle.position.x = goalArea.min.x;
        if (particle.position.y < goalArea.min.y - leewayY) {
            particle.life = 0.0f;
        }
    }
}

void MainGameScene::RenderGoalParticles() const {
    for (const GoalParticle& particle : m_goalParticles) {
        if (particle.life <= 0.0f || particle.maxLife <= 0.0f) continue;
        const float t = particle.life / particle.maxLife;
        const unsigned char alpha = static_cast<unsigned char>(40 + 180 * t);
        const Color color = Color{255, 255, 255, alpha};
        DrawCircleV(
            Vector2{
                static_cast<float>(particle.position.x),
                static_cast<float>(particle.position.y)
            },
            particle.size * (0.6f + 0.6f * t),
            color
        );
    }
}

void MainGameScene::UpdateExplosionVisuals(float dt) {
    const ExplosionAnimation& animation =
        m_appContext->textureLibrary->GetExplosionAnim();
    if (!animation.IsValid()) {
        m_explosionVisuals.clear();
        return;
    }

    const float frameDuration = animation.frameDuration;
    for (ExplosionVisualInstance& instance : m_explosionVisuals) {
        instance.timer += dt;
        while (instance.timer >= frameDuration) {
            instance.timer -= frameDuration;
            ++instance.frame;
        }
    }

    m_explosionVisuals.erase(
        std::remove_if(
            m_explosionVisuals.begin(),
            m_explosionVisuals.end(),
            [&animation](const ExplosionVisualInstance& instance) {
                return instance.frame >=
                    static_cast<int>(animation.frames.size());
            }
        ),
        m_explosionVisuals.end()
    );
}

void MainGameScene::RenderExplosionVisuals() const {
    const ExplosionAnimation& animation =
        m_appContext->textureLibrary->GetExplosionAnim();
    if (!animation.IsValid()) return;

    for (const ExplosionVisualInstance& instance : m_explosionVisuals) {
        if (instance.frame < 0 ||
            instance.frame >= static_cast<int>(animation.frames.size())) {
            continue;
        }
        const Texture2D& frame = animation.frames[instance.frame];
        const float size = instance.radius * 2.0f;
        DrawTexturePro(
            frame,
            Rectangle{
                0.0f,
                0.0f,
                static_cast<float>(frame.width),
                static_cast<float>(frame.height)
            },
            Rectangle{
                static_cast<float>(instance.position.x),
                static_cast<float>(instance.position.y),
                size,
                size
            },
            Vector2{size / 2.0f, size / 2.0f},
            0.0f,
            WHITE
        );
    }
}

void MainGameScene::SpawnDamageStarBurst(
    const Math::Vector2D& position,
    float cellSize,
    int particleCount,
    float speedMin,
    float speedMax,
    float lifeMin,
    float lifeMax,
    float sizeMinScale,
    float sizeMaxScale,
    float spread
) {
    const int speedMinI = static_cast<int>(speedMin);
    const int speedMaxI = static_cast<int>(speedMax);
    const int lifeMinI = static_cast<int>(lifeMin * 100.0f);
    const int lifeMaxI = static_cast<int>(lifeMax * 100.0f);
    const int sizeMinI = static_cast<int>(sizeMinScale * 100.0f);
    const int sizeMaxI = static_cast<int>(sizeMaxScale * 100.0f);

    for (int i = 0; i < particleCount; ++i) {
        const float angle =
            (static_cast<float>(GetRandomValue(0, 359)) +
             static_cast<float>(GetRandomValue(0, 99)) / 100.0f) *
            DEG2RAD;
        const float speed = static_cast<float>(GetRandomValue(speedMinI, speedMaxI));
        const float maxLife =
            static_cast<float>(GetRandomValue(lifeMinI, lifeMaxI)) / 100.0f;
        const float sizeScale =
            static_cast<float>(GetRandomValue(sizeMinI, sizeMaxI)) / 100.0f;
        const float size = cellSize * sizeScale;
        const Math::Vector2D direction{std::cos(angle), std::sin(angle)};
        m_damageParticles.push_back(DamageStarParticle{
            .position = position + direction * (cellSize * spread),
            .velocity = direction * speed,
            .life = maxLife,
            .maxLife = maxLife,
            .size = size,
            .rotation = static_cast<float>(GetRandomValue(0, 359)),
            .angularVelocity = static_cast<float>(GetRandomValue(-360, 360))
        });
    }
}

void MainGameScene::UpdateDamageParticles(float dt) {
    for (DamageStarParticle& particle : m_damageParticles) {
        particle.life -= dt;
        particle.position.x += particle.velocity.x * dt;
        particle.position.y += particle.velocity.y * dt;
        particle.velocity.x *= std::exp(-4.0f * dt);
        particle.velocity.y *= std::exp(-4.0f * dt);
        particle.rotation += particle.angularVelocity * dt;
    }

    m_damageParticles.erase(
        std::remove_if(
            m_damageParticles.begin(),
            m_damageParticles.end(),
            [](const DamageStarParticle& particle) {
                return particle.life <= 0.0f;
            }
        ),
        m_damageParticles.end()
    );
}

void MainGameScene::RenderDamageParticles() const {
    const Texture2D& star = m_appContext->textureLibrary->GetDamageStarTexture();
    if (star.id == 0) return;

    for (const DamageStarParticle& particle : m_damageParticles) {
        if (particle.life <= 0.0f || particle.maxLife <= 0.0f) continue;
        const float t = std::clamp(particle.life / particle.maxLife, 0.0f, 1.0f);
        const float fade = std::sqrt(t);
        const float scale = 0.7f + 0.3f * t;
        const float size = particle.size * scale;
        const unsigned char alpha =
            static_cast<unsigned char>(std::clamp(fade * 255.0f, 0.0f, 255.0f));

        DrawTexturePro(
            star,
            Rectangle{
                0.0f,
                0.0f,
                static_cast<float>(star.width),
                static_cast<float>(star.height)
            },
            Rectangle{
                static_cast<float>(particle.position.x),
                static_cast<float>(particle.position.y),
                size,
                size
            },
            Vector2{size / 2.0f, size / 2.0f},
            particle.rotation,
            Color{255, 255, 255, alpha}
        );
    }
}

void MainGameScene::ResetToBuildMode() {
    m_elapsedMs = 0.0;
    m_goalWindow.Reset();
    m_previousPartPositions.clear();
    m_capture.Clear();
    m_explosionVisuals.clear();
    m_damageParticles.clear();
    m_playSession.ConsumeDamageVisuals();
    m_playSession.ConsumeBreakageVisuals();
    if (m_competeAgainstGhost) m_ghostPlayback.PrepareCompetition();
    else m_ghostPlayback.StopAndRewind();
    m_leaderboard.Reset();
    m_appContext->audioController->SetFanActive(false);
    m_appContext->audioController->SetWheelActive(false);
    if (!m_stateMachine.IsBuildMode()) {
        m_stateMachine.EnterBuild();
    }
    ResetEditorCamera();

    m_playSession.Reset(m_level.GetTerrain());
    auto scenario = m_appContext->audioController->GetCurrentScenario();
    m_appContext->audioController->SetCurrentScenario({
        Scenario::VehicleBuild,
        scenario.levelIndex,
        scenario.levelEditorTrackName
    });
    std::fill(m_passengerCollected.begin(), m_passengerCollected.end(), false);
    m_carriedPassengers.clear();
    m_playSession.ClearCarriedPassengers();
}

void MainGameScene::ResetEditorCamera() {
    m_camera.target = m_editorCameraTarget;
    m_camera.zoom = m_editorCameraZoom;
}

void MainGameScene::FinishRoutePreview() {
    m_routePreviewActive = false;
    ResetEditorCamera();
}

void MainGameScene::ReturnToCustomLevelEditor() {
    m_appContext->audioController->PlayUiClick();
    m_appContext->audioController->SetFanActive(false);
    m_appContext->audioController->SetWheelActive(false);
    if (m_appContext->activeCustomLevel >= 0 &&
        m_appContext->activeCustomLevel <
            static_cast<int>(m_appContext->customLevels->GetLevels().size())) {
        m_appContext->audioController->StartLevelEditorMusic(
            m_appContext->customLevels->GetLevels()[m_appContext->activeCustomLevel].musicPath,
            false
        );
    }
    m_appContext->focusCustomEditorCamera = true;
    m_appContext->sceneRequester->RequestSceneChange({SceneId::CustomLevelEditor});
}

void MainGameScene::ReturnToLevelSelector() {
    m_appContext->audioController->PlayUiClick();
    m_appContext->audioController->SetFanActive(false);
    m_appContext->audioController->SetWheelActive(false);
    m_appContext->audioController->ReturnToSelector();
    m_appContext->requestedCommunitySelector =
        m_level.GetDefinition().levelNumber == -2;
    if (!m_appContext->requestedCommunitySelector) {
        m_appContext->requestedCustomSelector =
            m_level.GetDefinition().levelNumber < 0;
    }
    m_appContext->sceneRequester->RequestSceneChange({SceneId::MainMenu});
}

bool MainGameScene::ProcessGhostControls() {
    if (m_levelId.empty()) return false;
    const GhostLoadStatus status = m_ghostPlayback.GetStatus();
    const bool canUse = GhostCanBeRequested(status);
    if (UiButton::WasClicked(GhostPreviewButton())) {
        if (canUse && !m_competeAgainstGhost) {
            m_appContext->audioController->PlayUiClick();
            m_ghostPlayback.Play();
        }
        return true;
    }
    if (UiButton::WasClicked(GhostPauseButton())) {
        m_appContext->audioController->PlayUiClick();
        m_ghostPlayback.Pause();
        return true;
    }
    if (UiButton::WasClicked(GhostRestartButton())) {
        if (canUse && !m_competeAgainstGhost) {
            m_appContext->audioController->PlayUiClick();
            m_ghostPlayback.Restart();
        }
        return true;
    }
    if (UiButton::WasClicked(GhostCompeteButton())) {
        if (canUse) {
            m_appContext->audioController->PlayUiClick();
            m_competeAgainstGhost = !m_competeAgainstGhost;
            if (m_competeAgainstGhost) m_ghostPlayback.PrepareCompetition();
            else m_ghostPlayback.StopAndRewind();
        }
        return true;
    }
    if (CheckCollisionPointRec(GetMousePosition(), GhostPanel()) &&
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
         IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))) {
        return true;
    }
    return false;
}

void MainGameScene::RenderGhostControls() const {
    const GhostLoadStatus status = m_ghostPlayback.GetStatus();
    const bool canUse = GhostCanBeRequested(status);
    const Rectangle panel = GhostPanel();
    const Color accentBlue{91, 212, 246, 255};
    const Color accentYellow{243, 237, 33, 255};
    DrawRectangleRec(panel, Color{9, 27, 54, 239});
    DrawRectangleLinesEx(panel, 2.0f, accentBlue);
    DrawRectangle(static_cast<int>(panel.x + 2.0f),
        static_cast<int>(panel.y + 2.0f),
        static_cast<int>(panel.width - 4.0f), 23,
        Color{20, 58, 91, 255});
    DrawRectangle(static_cast<int>(panel.x + 6.0f),
        static_cast<int>(panel.y + 25.0f),
        static_cast<int>(panel.width - 12.0f), 2, accentBlue);
    DrawRectangle(static_cast<int>(panel.x + 6.0f),
        static_cast<int>(panel.y + 25.0f), 70, 2, accentYellow);
    DrawTextFont("FANTASMA DEL CAMPEÓN", 22,
        static_cast<int>(panel.y + 6.0f), 14, WHITE);

    const auto drawGhostButton = [&](Rectangle bounds, const char* label,
        bool enabled, bool active, int fontSize) {
        const bool hovered = enabled && UiButton::IsHovered(bounds);
        DrawRectangleRec(bounds,
            !enabled ? Color{24, 40, 58, 235}
            : hovered ? Color{42, 103, 143, 245}
            : active ? Color{29, 88, 126, 245}
            : Color{20, 58, 91, 245});
        DrawRectangleLinesEx(bounds, active ? 2.0f : 1.0f,
            enabled ? (active ? accentYellow : accentBlue)
                    : Color{65, 107, 127, 255});
        DrawTextFont(label,
            static_cast<int>(bounds.x +
                (bounds.width - MeasureTextFont(label, fontSize)) / 2.0f),
            static_cast<int>(bounds.y + (bounds.height - fontSize) / 2.0f),
            fontSize, enabled ? WHITE : Color{150, 175, 188, 255});
    };
    drawGhostButton(
        GhostPreviewButton(),
        m_competeAgainstGhost
            ? "LISTO PARA COMPETIR"
            : m_ghostPlayback.IsPlaying()
            ? "REPRODUCIENDO RECORRIDO"
            : "VER RECORRIDO DEL CAMPEÓN",
        canUse && !m_competeAgainstGhost,
        m_ghostPlayback.IsPlaying(),
        11
    );
    drawGhostButton(
        GhostPauseButton(),
        "PARAR",
        m_ghostPlayback.IsPlaying(),
        false,
        12
    );
    drawGhostButton(
        GhostRestartButton(),
        "REINICIAR",
        canUse && !m_competeAgainstGhost,
        false,
        12
    );
    drawGhostButton(
        GhostCompeteButton(),
        m_competeAgainstGhost
            ? "COMPETIR CON FANTASMA: SÍ"
            : "COMPETIR CON FANTASMA: NO",
        canUse,
        m_competeAgainstGhost,
        10
    );

    std::string message;
    Color messageColor = LIGHTGRAY;
    switch (status) {
        case GhostLoadStatus::Loading:
            message = "Buscando al campeón global...";
            break;
        case GhostLoadStatus::Available:
            message = "Campeón fijado · recorrido listo para descargar";
            messageColor = SKYBLUE;
            break;
        case GhostLoadStatus::LoadingRecording:
            message = "Descargando recorrido... puedes seguir editando";
            break;
        case GhostLoadStatus::Ready: {
            const int elapsedTenths = static_cast<int>(m_ghostPlayback.GetTimeMs() / 100.0);
            const int durationTenths = static_cast<int>(m_ghostPlayback.GetDurationMs() / 100.0);
            message = "Tiempo " + std::to_string(elapsedTenths / 10) + "." +
                std::to_string(elapsedTenths % 10) + " / " +
                std::to_string(durationTenths / 10) + "." +
                std::to_string(durationTenths % 10) + " s";
            messageColor = SKYBLUE;
            break;
        }
        case GhostLoadStatus::Unavailable:
            message = "El campeón no tiene fantasma disponible";
            break;
        case GhostLoadStatus::Incompatible:
            message = "Grabación incompatible con esta versión";
            messageColor = accentYellow;
            break;
        case GhostLoadStatus::Error:
            message = "Sin conexión: edición y juego siguen disponibles";
            messageColor = accentYellow;
            break;
        case GhostLoadStatus::Idle:
            message = "Fantasma no disponible";
            break;
    }
    DrawRectangle(static_cast<int>(panel.x + 6.0f),
        static_cast<int>(panel.y + 100.0f),
        static_cast<int>(panel.width - 12.0f), 1,
        Fade(accentBlue, 0.65f));
    DrawTextFont(message.c_str(), 22,
        static_cast<int>(panel.y + 104.0f), 11, messageColor);
}

void MainGameScene::Render() const {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    const auto renderPassengersBeforeVehicle = [&]() {
        const std::vector<TrackedPart> trackedParts = CollectParts(
            m_playSession.GetRuntimeGrid()
        );

        const auto& passengerDefinitions = m_level.GetDefinition().passengers;
        for (std::size_t i = 0; i < passengerDefinitions.size(); ++i) {
            if (i < m_passengerCollected.size() && m_passengerCollected[i]) continue;
            const auto& passenger = passengerDefinitions[i];
            DrawPassenger(
                *m_appContext->textureLibrary,
                static_cast<float>(passenger.position.x),
                static_cast<float>(passenger.position.y),
                passenger.hasSkirt,
                1.5f,
                BLACK,
                m_appContext->textureLibrary->FindPassengerFace(passenger.face),
                0.0f
            );
        }

        for (const CarriedPassenger& passenger : m_carriedPassengers) {
            const TrackedPart* host = FindTrackedPart(
                trackedParts, passenger.hostId
            );
            if (host == nullptr) continue;
            const float rotation = static_cast<float>(host->state.rotation);
            // desplazar al pasajero en el espacio local del contenedor para que
            // siga su inclinación
            const float localOffsetY = 7.0f;
            const float sinR = std::sin(rotation);
            const float cosR = std::cos(rotation);
            const float worldX = static_cast<float>(host->state.position.x) -
                localOffsetY * sinR;
            const float worldY = static_cast<float>(host->state.position.y) +
                localOffsetY * cosR;
            DrawPassenger(
                *m_appContext->textureLibrary,
                worldX,
                worldY,
                passenger.hasSkirt,
                1.0f,
                BLACK,
                m_appContext->textureLibrary->FindPassengerFace(passenger.face),
                rotation
            );
        }

        for (const auto& passenger : m_playSession.GetEjectedPassengers()) {
            if (!passenger.body) continue;
            const auto position = passenger.body->GetPosition();
            DrawPassenger(
                *m_appContext->textureLibrary,
                static_cast<float>(position.x),
                static_cast<float>(position.y),
                passenger.hasSkirt,
                1.0f,
                BLACK,
                m_appContext->textureLibrary->FindPassengerFace(passenger.face),
                static_cast<float>(passenger.body->GetRotation())
            );
        }
    };

    m_worldRenderer.Render(
        m_camera,
        m_stateMachine.GetState(),
        m_level.GetGrid(),
        m_playSession,
        m_buildSession.GetSelectedPartType(),
        m_buildSession.GetSelectedOrientation(),
        m_buildSession.GetRepositionedPartType(),
        m_buildSession.GetRepositionedOrientation(),
        m_appContext->windowHeight,
        renderPassengersBeforeVehicle
    );
    BeginMode2D(m_camera);
    if (m_ghostPlayback.IsVisible()) {
        m_worldRenderer.RenderGhostPlayback(m_ghostPlayback.GetRenderStates());
        for (const GhostPassengerState& passenger :
             m_ghostPlayback.GetPassengerStates()) {
            DrawPassenger(
                *m_appContext->textureLibrary,
                static_cast<float>(passenger.position.x),
                static_cast<float>(passenger.position.y),
                passenger.hasSkirt,
                1.0f,
                Color{110, 205, 255, 150},
                m_appContext->textureLibrary->FindPassengerFace(passenger.face),
                0.0f
            );
        }
    }

    Texture2D goalTexture = m_appContext->textureLibrary->GetLevelGoalTexture();
    const GoalDefinition& goal = m_level.GetDefinition().goal;
    const Math::AABB goalArea = goal.GetAABB();
    const Rectangle goalRect{
        static_cast<float>(goalArea.min.x),
        static_cast<float>(goalArea.min.y),
        static_cast<float>(goalArea.max.x - goalArea.min.x),
        static_cast<float>(goalArea.max.y - goalArea.min.y)
    };

    RenderGoalParticles();
    RenderExplosionVisuals();
    RenderDamageParticles();
    DrawTexturePro(
        goalTexture,
        Rectangle{0.0f, 0.0f, static_cast<float>(goalTexture.width), static_cast<float>(goalTexture.height)},
        goalRect,
        Vector2{0.0f, 0.0f},
        0.0f,
        WHITE
    );
    EndMode2D();
    if (m_stateMachine.IsResultsMode()) {
        RenderResults();
        EndDrawing();
        return;
    }
    if (m_routePreviewActive) {
        const int width = m_appContext->windowWidth;
        const int height = m_appContext->windowHeight;
        const float progress = std::clamp(
            (m_routePreviewElapsed - RoutePreviewHoldSeconds) /
                m_routePreviewDuration,
            0.0f, 1.0f
        );
        constexpr int bannerHeight = 42;
        DrawRectangle(0, 0, width, bannerHeight, Fade(DARKBLUE, 0.78f));
        DrawTextFont("VISTA PREVIA DEL RECORRIDO", 20, 13, 20, WHITE);
        DrawRectangle(0, height - 5, width, 5, Fade(DARKBLUE, 0.65f));
        DrawRectangle(0, height - 5, static_cast<int>(width * progress), 5, SKYBLUE);
        constexpr int skipFontSize = 12;
        const char* skipText = "CLIC O ENTER PARA OMITIR";
        DrawTextFont(
            skipText,
            static_cast<int>(width - 20 - MeasureTextFont(skipText, skipFontSize)),
            (bannerHeight - skipFontSize) / 2,
            skipFontSize,
            WHITE
        );
        EndDrawing();
        return;
    }
    m_uiRenderer.Render(
        m_appContext->windowWidth,
        m_appContext->windowHeight,
        m_stateMachine.GetState(),
        m_buildSession,
        m_level.GetGrid(),
        m_camera,
        m_playSession.GetActivationRegistry(),
        *m_appContext->textureLibrary
    );
    if (m_stateMachine.IsBuildMode() && !m_levelId.empty()) {
        RenderGhostControls();
    }
    if (m_stateMachine.IsPlayMode() && m_goalWindow.GetStartMs() >= 0.0 &&
        !m_levelCompleted) {
        const double elapsedWindowMs =
            m_elapsedMs - m_goalWindow.GetStartMs();
        const int count = std::clamp(
            static_cast<int>(std::ceil((3000.0 - elapsedWindowMs) / 1000.0)),
            1,
            3
        );
        const float centerX = m_appContext->windowWidth / 2.0f;
        const float centerY = m_appContext->windowHeight / 2.0f;
        const char* label = "¡Ya llegó el chofer, causa!";
        const int labelX = static_cast<int>(
            centerX - MeasureTextFont(label, 30) / 2
        );
        DrawTextFont(label, labelX + 2,
            static_cast<int>(centerY - 88.0f), 30, BLACK);
        DrawTextFont(
            label,
            labelX,
            static_cast<int>(centerY - 90.0f),
            30,
            WHITE
        );
        const Texture2D& number = m_appContext->textureLibrary->GetUi()
            .game.countdownNumbers[static_cast<std::size_t>(count - 1)];
        DrawTexturePro(
            number,
            Rectangle{0.0f, 0.0f,
                static_cast<float>(number.width), static_cast<float>(number.height)},
            Rectangle{centerX - 80.0f, centerY - 30.0f, 160.0f, 160.0f},
            Vector2{}, 0.0f, WHITE
        );
    }
    UiButton::DrawTextured(
        UiButton::GameMenuButton(m_appContext->windowWidth),
        m_appContext->textureLibrary->GetUi().menuBack,
        true,
        "Menú"
    );
    if (m_isEditableDraft && m_stateMachine.IsBuildMode()) {
        const Rectangle designButton = DesignLevelButton(
            m_appContext->windowWidth, m_appContext->windowHeight
        );
        const UiGameTextures& gameUi = m_appContext->textureLibrary->GetUi().game;
        UiButton::DrawTextureFit(gameUi.toolbarSlot.normal, designButton);
        DrawTexturePro(
            gameUi.navDiseno.normal,
            Rectangle{116.0f, 18.0f, 168.0f, 151.0f},
            Rectangle{designButton.x + 54.0f, designButton.y + 3.0f,
                62.0f, 58.0f},
            Vector2{}, 0.0f, WHITE
        );
        const bool hovered = UiButton::IsHovered(designButton);
        if (hovered) {
            DrawRectangleRec(Rectangle{
                designButton.x + 4.0f, designButton.y + 4.0f,
                designButton.width - 8.0f, designButton.height - 8.0f
            }, Fade(WHITE, IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? 0.10f : 0.18f));
        }
        const char* label = "Diseñar nivel";
        DrawTextFont(label,
            static_cast<int>(designButton.x +
                (designButton.width - MeasureTextFont(label, 17)) / 2.0f),
            static_cast<int>(designButton.y + 66.0f),
            17, WHITE);
    }

    if (m_exitConfirmOpen) {
        const int width = m_appContext->windowWidth;
        const int height = m_appContext->windowHeight;
        DrawRectangle(0, 0, width, height, Fade(BLACK, 0.62f));
        const ExitConfirmDialogLayout layout =
            GetExitConfirmDialogLayout(width, height);
        const Rectangle& dialog = layout.dialog;
        const Color accentGreen{129, 195, 71, 255};
        const Color accentPink{237, 18, 121, 255};
        const Color mutedText{190, 201, 203, 255};
        const UiTextures& ui = m_appContext->textureLibrary->GetUi();

        DrawRectangleRec(
            Rectangle{dialog.x + 7.0f, dialog.y + 8.0f, dialog.width, dialog.height},
            Fade(BLACK, 0.55f)
        );
        DrawRectangleRec(dialog, Color{12, 17, 24, 250});
        DrawRectangleLinesEx(dialog, 3.0f, accentGreen);
        DrawRectangle(
            static_cast<int>(dialog.x + 3.0f),
            static_cast<int>(dialog.y + 84.0f),
            static_cast<int>(dialog.width - 6.0f),
            3,
            accentPink
        );
        DrawTextFont(
            "¿SALIR DEL NIVEL?",
            static_cast<int>(dialog.x + 24.0f),
            static_cast<int>(dialog.y + 18.0f),
            28,
            GOLD
        );
        DrawTextFont(
            "Volverás al selector y perderás el progreso no guardado.",
            static_cast<int>(dialog.x + 24.0f),
            static_cast<int>(dialog.y + 57.0f),
            16,
            mutedText
        );
        DrawTextFont(
            "ESC: cancelar   ENTER: salir",
            static_cast<int>(dialog.x + 24.0f),
            static_cast<int>(dialog.y + 132.0f),
            15,
            mutedText
        );

        DrawExitConfirmAction(layout.cancel, ui.menuDialogCancel, "CANCELAR", true);
        DrawExitConfirmAction(layout.confirm, ui.menuDialogAccept, "SALIR", true);
    }

    EndDrawing();
}

void MainGameScene::RenderResults() const {
    const int width = m_appContext->windowWidth;
    const int height = m_appContext->windowHeight;
    const Color accentPink{237, 18, 121, 255};
    const Color accentGreen{129, 195, 71, 255};
    const Color bodyText{246, 243, 225, 255};
    const Color mutedText{190, 201, 203, 255};
    DrawRectangle(0, 0, width, height, Fade(BLACK, 0.32f));
    DrawRectangle(0, 0, width, 72, Fade(BLACK, 0.86f));
    DrawRectangle(0, 70, width, 3, accentPink);
    DrawTextFont("RESULTADOS", 36, 22, 36, GOLD);

    const float panelWidth = (width - 96.0f) / 2.0f;
    const Rectangle rankingPanel{30.0f, 82.0f, panelWidth, height - 180.0f};
    const Rectangle scorePanel{
        rankingPanel.x + panelWidth + 36.0f,
        rankingPanel.y + (rankingPanel.height - 430.0f) / 2.0f,
        panelWidth, 430.0f
    };
    DrawRectangleRec(rankingPanel, Fade(BLACK, 0.86f));
    DrawRectangleLinesEx(rankingPanel, 3.0f, accentGreen);
    DrawRectangleRec(scorePanel, Fade(BLACK, 0.86f));
    DrawRectangleLinesEx(scorePanel, 3.0f, accentGreen);
    DrawTextFont("RANKING", 52, 106, 26, GOLD);
    const LeaderboardSnapshot& leaderboard = m_leaderboard.GetSnapshot();
    if (leaderboard.status == LeaderboardStatus::Loading) {
        DrawTextFont("Esperando confirmación del servidor...", 52, 150, 15, mutedText);
    } else if (leaderboard.status == LeaderboardStatus::Unavailable) {
        DrawTextFont("Ranking global no disponible.", 52, 150, 17, bodyText);
        if (leaderboard.personalBest.available) {
            const std::string local = "Récord personal local: " +
                std::to_string(leaderboard.personalBest.total);
            DrawTextFont(local.c_str(), 52, 183, 16, mutedText);
        }
    } else if (leaderboard.status == LeaderboardStatus::Ready) {
        if (leaderboard.entries.empty()) {
            DrawTextFont("Aún no hay marcas clasificadas.", 52, 150, 16, mutedText);
        }
        int rowY = 142;
        for (const LeaderboardEntry& entry : leaderboard.entries) {
            if (rowY + 49 > rankingPanel.y + rankingPanel.height - 62) break;
            const bool own = !m_leaderboard.GetCombiId().empty() &&
                entry.combiId == m_leaderboard.GetCombiId();
            const bool current = own &&
                entry.attemptId == m_leaderboard.GetAttemptId();
            const Rectangle rowPanel{
                42.0f,
                static_cast<float>(rowY),
                rankingPanel.width - 24.0f,
                49.0f
            };
            if (own) {
                DrawRectangleRec(
                    rowPanel,
                    Fade(current ? SKYBLUE : GOLD, 0.25f)
                );
            }
            const std::string name = entry.name.empty()
                ? (own ? "Tu combi" : "Combi sin nombre")
                : entry.name;
            const std::string label = "#" + std::to_string(entry.rank) +
                "  " + name;
            DrawTextFont(label.c_str(), 50, rowY + 5, 16, own ? GOLD : bodyText);
            const std::string totalValue = std::to_string(entry.total);
            DrawTextFont(
                totalValue.c_str(),
                static_cast<int>(rankingPanel.x + rankingPanel.width - 14.0f) -
                    MeasureTextFont(totalValue.c_str(), 18),
                rowY + 4,
                18,
                GOLD
            );
            std::string detail = std::to_string(entry.driverTimeMs) +
                " ms · " + std::to_string(entry.initialParts) + " piezas";
            if (own) {
                detail += current ? " · esta partida" : " · tu récord anterior";
            }
            DrawTextFont(detail.c_str(), 50, rowY + 28, 12, own ? SKYBLUE : mutedText);
            rowY += 53;
        }
        DrawTextFont(
            leaderboard.championRunRef.empty()
                ? "Fantasma campeón: no disponible."
                : "Fantasma campeón: grabación lista.",
            46,
            static_cast<int>(rankingPanel.y + rankingPanel.height - 46.0f),
            11,
            leaderboard.championRunRef.empty() ? mutedText : accentGreen
        );
    }

    const int panelX = static_cast<int>(scorePanel.x + 22.0f);
    const int valueX = static_cast<int>(scorePanel.x + scorePanel.width - 22.0f);
    const int scoreY = static_cast<int>(scorePanel.y);
    DrawTextFont("PUNTUACIÓN", panelX, scoreY + 24, 28, GOLD);
    const auto row = [panelX, valueX, bodyText, accentPink](
        const std::string& label,
        std::int64_t value,
        int y
    ) {
        DrawTextFont(label.c_str(), panelX, y, 18, bodyText);
        const std::string number = std::to_string(value);
        DrawTextFont(
            number.c_str(),
            static_cast<int>(valueX - MeasureTextFont(number.c_str(), 18)),
            y,
            18,
            value < 0 ? accentPink : GOLD
        );
    };
    row(
        "Pasajeros recogidos (" +
            std::to_string(m_statistics.passengersCollected) + ")",
        m_score.passengerPickup,
        scoreY + 68
    );
    row(
        "Pasajeros entregados (" +
            std::to_string(m_statistics.passengersDelivered) + ")",
        m_score.passengerDelivery,
        scoreY + 104
    );
    row(
        "Piezas entregadas (" +
            std::to_string(m_statistics.partsDelivered) + "/" +
            std::to_string(m_statistics.initialParts) + ")",
        m_score.partDelivery,
        scoreY + 140
    );
    row(
        "Construcción (" +
            std::to_string(m_statistics.initialParts) + " piezas)",
        m_score.constructionPenalty,
        scoreY + 176
    );
    row(
        "Rapidez (" + std::to_string(m_statistics.driverTimeMs) + " ms)",
        m_score.speed,
        scoreY + 212
    );
    DrawLine(panelX, scoreY + 283, valueX, scoreY + 283, accentGreen);
    const UiTextures& ui = m_appContext->textureLibrary->GetUi();
    const Texture2D& totalLabel = ui.scoreTotalLabel;
    constexpr float totalLabelHeight = 54.0f;
    const float totalLabelWidth = totalLabelHeight *
        totalLabel.width / totalLabel.height;
    UiButton::DrawTextureFit(totalLabel,
        Rectangle{static_cast<float>(panelX), static_cast<float>(scoreY + 294),
            totalLabelWidth, totalLabelHeight});
    const std::string total = std::to_string(m_score.total);
    DrawScoreValue(total, ui, static_cast<float>(valueX),
        static_cast<float>(scoreY + 292),
        std::max(20.0f, valueX - panelX - totalLabelWidth - 16.0f)
    );
    if (leaderboard.personalBest.available) {
        const std::string personal = "Récord personal local: " +
            std::to_string(leaderboard.personalBest.total);
        DrawTextFont(personal.c_str(), panelX, scoreY + 353, 16, bodyText);
        DrawTextFont(
            leaderboard.personalBest.currentAttempt
                ? "Nueva mejor marca local"
                : "Tu mejor marca local anterior sigue vigente",
            panelX,
            scoreY + 378,
            13,
            leaderboard.personalBest.currentAttempt ? accentGreen : mutedText
        );
    }

    const Rectangle returnButton = ResultsReturnButton(width, height);
    UiButton::DrawTextured(
        returnButton,
        m_appContext->textureLibrary->GetUi().resultsBack,
        true,
        "Volver"
    );
    constexpr int enterSize = 11;
    const char* enterLabel = "ENTER";
    DrawTextFont(
        enterLabel,
        static_cast<int>(returnButton.x + returnButton.width -
            MeasureTextFont(enterLabel, enterSize) - 10.0f),
        static_cast<int>(returnButton.y + 8.0f),
        enterSize,
        bodyText
    );
    if (m_isEditableDraft && m_appContext->activeCustomLevel >= 0 &&
        m_appContext->customLevels->HasValidApproval(
            static_cast<std::size_t>(m_appContext->activeCustomLevel))) {
        const char* publishMessage = "¡Ya puedes publicar tu nivel!";
        const int messageX = static_cast<int>(
            returnButton.x + returnButton.width + 18.0f
        );
        const int messageY = static_cast<int>(returnButton.y + 22.0f);
        DrawTextFont(publishMessage, messageX + 1,
            messageY + 1, 17, BLACK);
        DrawTextFont(publishMessage, messageX, messageY, 17, GOLD);
    }
}

std::optional<std::string> MainGameScene::PersonalRecordOwner() const {
    return m_appContext->identity.ResultOwner(m_runIdentity);
}
