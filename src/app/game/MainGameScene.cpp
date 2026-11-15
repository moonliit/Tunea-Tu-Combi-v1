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

void DrawPassenger(
    float x,
    float y,
    bool hasSkirt,
    float scale = 1.0f,
    Color tint = BLACK
) {
    DrawCircleV({x, y - 19.0f * scale}, 6.0f * scale, tint);
    DrawLineEx({x, y - 13.0f * scale}, {x, y - 2.0f * scale}, 3.0f * scale, tint);
    DrawLineEx({x, y - 9.0f * scale}, {x - 7.0f * scale, y - 4.0f * scale}, 2.0f * scale, tint);
    DrawLineEx({x, y - 9.0f * scale}, {x + 7.0f * scale, y - 4.0f * scale}, 2.0f * scale, tint);
    if (hasSkirt) {
        const bool standardPassenger = tint.r == BLACK.r &&
            tint.g == BLACK.g && tint.b == BLACK.b;
        DrawTriangle({x - 5.0f * scale, y - 2.0f * scale},
            {x + 5.0f * scale, y - 2.0f * scale},
            {x, y + 4.0f * scale}, standardPassenger ? PINK : tint);
    }
    DrawLineEx({x, y - 2.0f * scale}, {x - 4.0f * scale, y + 5.0f * scale}, 2.0f * scale, tint);
    DrawLineEx({x, y - 2.0f * scale}, {x + 4.0f * scale, y + 5.0f * scale}, 2.0f * scale, tint);
}

Rectangle GhostPreviewButton() {
    return {20.0f, 104.0f, 230.0f, 32.0f};
}

Rectangle GhostPanel() {
    return {14.0f, 76.0f, 350.0f, 126.0f};
}

Rectangle GhostPauseButton() {
    return {258.0f, 104.0f, 100.0f, 32.0f};
}

Rectangle GhostRestartButton() {
    return {20.0f, 142.0f, 110.0f, 30.0f};
}

Rectangle GhostCompeteButton() {
    return {138.0f, 142.0f, 220.0f, 30.0f};
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
    // parallax starts from here
    m_worldRenderer.SetBackgroundOrigin(m_camera.target.x);
    m_passengerCollected.assign(m_level.GetDefinition().passengers.size(), false);
    m_ghostPlayback.Request(m_levelId, m_level.GetGrid().GetCellSize());
}

void MainGameScene::ProcessInput(float dt) {
    if (m_stateMachine.IsResultsMode()) {
        const Rectangle returnButton = UiButton::CenteredAtBottom(
            m_appContext->windowWidth,
            m_appContext->windowHeight,
            220.0f,
            42.0f,
            30.0f
        );
        if (IsKeyPressed(KEY_ENTER) || UiButton::WasClicked(returnButton)) {
            m_appContext->audioController->PlayUiClick();
            m_appContext->audioController->ReturnToSelector();
            m_appContext->sceneRequester->RequestSceneChange({SceneId::MainMenu});
        }
        return;
    }
    if (m_stateMachine.IsBuildMode() && ProcessGhostControls()) return;
    if (m_isEditableDraft && m_stateMachine.IsBuildMode()) {
        const Rectangle designButton = UiButton::CenteredAbove(
            UiButton::PlaySideButton(
                m_appContext->windowWidth, m_appContext->windowHeight
            ),
            110.0f,
            52.0f,
            8.0f
        );
        if (UiButton::WasClicked(designButton)) {
            m_appContext->audioController->SetFanActive(false);
            m_appContext->audioController->SetWheelActive(false);
            if (m_appContext->activeCustomLevel >= 0 &&
                m_appContext->activeCustomLevel <
                    static_cast<int>(m_appContext->customLevels->GetLevels().size())) {
                m_appContext->audioController->StartLevelEditorMusic(
                    m_appContext->customLevels->GetLevels()[m_appContext->activeCustomLevel].musicPath,
                    false
                );
            } else {
                auto scenario = m_appContext->audioController->GetCurrentScenario();
                if (!scenario.levelEditorTrackName.empty()) {
                    m_appContext->audioController->SetCurrentScenario(
                        {Scenario::LevelEditorBuild, -1, scenario.levelEditorTrackName}
                    );
                } else {
                    m_appContext->audioController->SetCurrentScenario({Scenario::PlayLevel});
                }
            }
            m_appContext->returnToCustomDesign = true;
            m_appContext->sceneRequester->RequestSceneChange({SceneId::MainMenu});
            return;
        }
    }

    const Rectangle menuButton = UiButton::AtTopRight(
        m_appContext->windowWidth, 12.0f, 20.0f, 102.0f, 52.0f
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
        return;
    }
    m_ghostPlayback.Update(frameMs);
    if (!GhostCanBeRequested(m_ghostPlayback.GetStatus())) {
        m_competeAgainstGhost = false;
    }
    if (m_stateMachine.IsPlayMode()) {
        m_playSession.Update(dt);
        m_elapsedMs += frameMs;
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
                passenger.hasSkirt
            });
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
            if (m_playSession.GetBreakageConsumed()) {
                m_appContext->audioController->PlayBreakageEffect();
                m_playSession.GetBreakageConsumed() = false;
            }
        }
        UpdateGoalWindow(frameMs);
        if (m_stateMachine.IsPlayMode()) {
            m_capture.RecordFrame(
                std::llround(m_elapsedMs), runtimeGrid
            );
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

void MainGameScene::ResetToBuildMode() {
    m_elapsedMs = 0.0;
    m_goalWindow.Reset();
    m_previousPartPositions.clear();
    m_capture.Clear();
    m_ghostPlayback.StopAndRewind();
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
}

void MainGameScene::ResetEditorCamera() {
    m_camera.target = m_editorCameraTarget;
    m_camera.zoom = m_editorCameraZoom;
}

bool MainGameScene::ProcessGhostControls() {
    if (m_levelId.empty()) return false;
    const GhostLoadStatus status = m_ghostPlayback.GetStatus();
    const bool canUse = GhostCanBeRequested(status);
    if (UiButton::WasClicked(GhostPreviewButton())) {
        if (canUse) {
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
        if (canUse) {
            m_appContext->audioController->PlayUiClick();
            m_ghostPlayback.Restart();
        }
        return true;
    }
    if (UiButton::WasClicked(GhostCompeteButton())) {
        if (canUse) {
            m_appContext->audioController->PlayUiClick();
            m_competeAgainstGhost = !m_competeAgainstGhost;
            if (m_competeAgainstGhost) m_ghostPlayback.EnsureLoaded();
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
    DrawRectangleRec(GhostPanel(), Fade(DARKBLUE, 0.84f));
    DrawRectangleLinesEx(GhostPanel(), 2.0f, SKYBLUE);
    DrawTextFont("FANTASMA DEL CAMPEÓN", 22, 82, 14, WHITE);
    UiButton::Draw(
        GhostPreviewButton(),
        m_ghostPlayback.IsPlaying()
            ? "REPRODUCIENDO RECORRIDO"
            : "VER RECORRIDO DEL CAMPEÓN",
        canUse ? DARKGREEN : DARKGRAY,
        canUse ? WHITE : GRAY,
        11
    );
    UiButton::Draw(
        GhostPauseButton(),
        "PARAR",
        m_ghostPlayback.IsPlaying() ? MAROON : DARKGRAY,
        WHITE,
        12
    );
    UiButton::Draw(
        GhostRestartButton(),
        "REINICIAR",
        canUse ? ORANGE : DARKGRAY,
        canUse ? WHITE : GRAY,
        12
    );
    UiButton::Draw(
        GhostCompeteButton(),
        m_competeAgainstGhost
            ? "COMPETIR CON FANTASMA: SÍ"
            : "COMPETIR CON FANTASMA: NO",
        m_competeAgainstGhost ? DARKGREEN : DARKGRAY,
        canUse ? WHITE : GRAY,
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
            messageColor = ORANGE;
            break;
        case GhostLoadStatus::Error:
            message = "Sin conexión: edición y juego siguen disponibles";
            messageColor = ORANGE;
            break;
        case GhostLoadStatus::Idle:
            message = "Fantasma no disponible";
            break;
    }
    DrawTextFont(message.c_str(), 22, 180, 11, messageColor);
}

void MainGameScene::Render() const {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (m_stateMachine.IsResultsMode()) {
        RenderResults();
        EndDrawing();
        return;
    }

    m_worldRenderer.Render(
        m_camera,
        m_stateMachine.GetState(),
        m_level.GetGrid(),
        m_playSession,
        m_buildSession.GetSelectedPartType(),
        m_buildSession.GetSelectedOrientation(),
        m_buildSession.GetRepositionedPartType(),
        m_buildSession.GetRepositionedOrientation(),
        m_appContext->windowHeight
    );
    BeginMode2D(m_camera);
    if (m_ghostPlayback.IsVisible()) {
        m_worldRenderer.RenderGhostPlayback(m_ghostPlayback.GetRenderStates());
        for (const GhostPassengerState& passenger :
             m_ghostPlayback.GetPassengerStates()) {
            DrawPassenger(
                static_cast<float>(passenger.position.x),
                static_cast<float>(passenger.position.y),
                passenger.hasSkirt,
                1.0f,
                Color{110, 205, 255, 150}
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
    DrawTexturePro(
        goalTexture,
        Rectangle{0.0f, 0.0f, static_cast<float>(goalTexture.width), static_cast<float>(goalTexture.height)},
        goalRect,
        Vector2{0.0f, 0.0f},
        0.0f,
        WHITE
    );

    const auto& passengerDefinitions = m_level.GetDefinition().passengers;
    for (std::size_t i = 0; i < passengerDefinitions.size(); ++i) {
        if (i < m_passengerCollected.size() && m_passengerCollected[i]) continue;
        const auto& passenger = passengerDefinitions[i];
        DrawPassenger(
            static_cast<float>(passenger.position.x),
            static_cast<float>(passenger.position.y),
            passenger.hasSkirt,
            1.5f
        );
    }
    const std::vector<TrackedPart> trackedParts = CollectParts(
        m_playSession.GetRuntimeGrid()
    );
    for (const CarriedPassenger& passenger : m_carriedPassengers) {
        const TrackedPart* host = FindTrackedPart(
            trackedParts, passenger.hostId
        );
        if (host == nullptr) continue;
        const auto position = host->state.position;
        DrawPassenger(
            static_cast<float>(position.x),
            static_cast<float>(position.y) + 7.0f,
            passenger.hasSkirt
        );
    }
    EndMode2D();
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
        DrawRectangle(
            0, 0,
            m_appContext->windowWidth,
            m_appContext->windowHeight,
            Fade(DARKGRAY, 0.5f)
        );
        const char* label = "Ya llego el chofer, causa!";
        DrawTextFont(
            label,
            static_cast<int>(centerX - MeasureText(label, 30) / 2),
            static_cast<int>(centerY - 90.0f),
            30,
            WHITE
        );
        const char* number = TextFormat("%d", count);
        DrawTextFont(
            number,
            static_cast<int>(centerX - MeasureText(number, 120) / 2),
            static_cast<int>(centerY - 35.0f),
            120,
            WHITE
        );
    }
    UiButton::DrawTextured(
        UiButton::AtTopRight(
            m_appContext->windowWidth, 12.0f, 20.0f, 102.0f, 52.0f
        ),
        m_appContext->textureLibrary->GetUi().game.navMenu,
        true,
        "Menú"
    );
    if (m_isEditableDraft && m_stateMachine.IsBuildMode()) {
        UiButton::DrawTextured(
            UiButton::CenteredAbove(
                UiButton::PlaySideButton(
                    m_appContext->windowWidth, m_appContext->windowHeight
                ),
                110.0f,
                50.0f,
                // 100.0f,
                // 52.0f,
                8.0f
            ),
            m_appContext->textureLibrary->GetUi().game.navDiseno,
            true,
            "DISEÑO"
        );
    }

    EndDrawing();
}

void MainGameScene::RenderResults() const {
    const int width = m_appContext->windowWidth;
    const int height = m_appContext->windowHeight;
    DrawTextFont("Resultados", 36, 28, 34, BLACK);

    const Rectangle rankingPanel{30.0f, 82.0f, width * 0.44f, height - 165.0f};
    DrawRectangleRec(rankingPanel, Fade(LIGHTGRAY, 0.55f));
    DrawRectangleLinesEx(rankingPanel, 2.0f, GRAY);
    DrawTextFont("Ranking", 52, 106, 26, DARKGRAY);
    const LeaderboardSnapshot& leaderboard = m_leaderboard.GetSnapshot();
    if (leaderboard.status == LeaderboardStatus::Loading) {
        DrawTextFont("Esperando confirmación del servidor...", 52, 150, 15, GRAY);
    } else if (leaderboard.status == LeaderboardStatus::Unavailable) {
        DrawTextFont("Ranking global no disponible.", 52, 150, 17, MAROON);
        if (leaderboard.personalBest.available) {
            const std::string local = "Récord personal local: " +
                std::to_string(leaderboard.personalBest.total);
            DrawTextFont(local.c_str(), 52, 183, 16, DARKGRAY);
        }
    } else if (leaderboard.status == LeaderboardStatus::Ready) {
        if (leaderboard.entries.empty()) {
            DrawTextFont("Aún no hay marcas clasificadas.", 52, 150, 16, GRAY);
        }
        int rowY = 142;
        for (const LeaderboardEntry& entry : leaderboard.entries) {
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
                    Fade(current ? SKYBLUE : GOLD, 0.42f)
                );
            }
            const std::string name = entry.name.empty()
                ? (own ? "Tu combi" : "Combi sin nombre")
                : entry.name;
            const std::string label = "#" + std::to_string(entry.rank) +
                "  " + name;
            DrawTextFont(label.c_str(), 50, rowY + 5, 16, own ? BLACK : DARKGRAY);
            const std::string totalValue = std::to_string(entry.total);
            DrawTextFont(
                totalValue.c_str(),
                static_cast<int>(rankingPanel.x + rankingPanel.width - 14.0f) -
                    MeasureText(totalValue.c_str(), 18),
                rowY + 4,
                18,
                BLACK
            );
            std::string detail = std::to_string(entry.driverTimeMs) +
                " ms · " + std::to_string(entry.initialParts) + " piezas";
            if (own) {
                detail += current ? " · esta partida" : " · tu récord anterior";
            }
            DrawTextFont(detail.c_str(), 50, rowY + 28, 12, own ? MAROON : GRAY);
            rowY += 53;
        }
        DrawTextFont(
            leaderboard.submissionConfirmed
                ? "Puntos recalculados; física no verificada."
                : "Resultado local no enviado al ranking.",
            46,
            static_cast<int>(rankingPanel.y + rankingPanel.height - 27.0f),
            11,
            GRAY
        );
        DrawTextFont(
            leaderboard.championRunRef.empty()
                ? "Fantasma campeón: no disponible."
                : "Fantasma campeón: grabación lista.",
            46,
            static_cast<int>(rankingPanel.y + rankingPanel.height - 46.0f),
            11,
            leaderboard.championRunRef.empty() ? GRAY : DARKGREEN
        );
    }

    const int panelX = static_cast<int>(width * 0.52f);
    const int valueX = width - 55;
    DrawTextFont("Puntuación", panelX, 86, 28, BLACK);
    const auto row = [panelX, valueX](
        const std::string& label,
        std::int64_t value,
        int y
    ) {
        DrawTextFont(label.c_str(), panelX, y, 18, DARKGRAY);
        const std::string number = std::to_string(value);
        DrawTextFont(
            number.c_str(),
            valueX - MeasureText(number.c_str(), 18),
            y,
            18,
            value < 0 ? MAROON : BLACK
        );
    };
    row(
        "Pasajeros recogidos (" +
            std::to_string(m_statistics.passengersCollected) + ")",
        m_score.passengerPickup,
        135
    );
    row(
        "Pasajeros entregados (" +
            std::to_string(m_statistics.passengersDelivered) + ")",
        m_score.passengerDelivery,
        171
    );
    row(
        "Piezas entregadas (" +
            std::to_string(m_statistics.partsDelivered) + "/" +
            std::to_string(m_statistics.initialParts) + ")",
        m_score.partDelivery,
        207
    );
    row(
        "Construcción (" +
            std::to_string(m_statistics.initialParts) + " piezas)",
        m_score.constructionPenalty,
        243
    );
    row(
        "Rapidez (" + std::to_string(m_statistics.driverTimeMs) + " ms)",
        m_score.speed,
        279
    );
    DrawLine(panelX, 365, valueX, 365, DARKGRAY);
    DrawTextFont("TOTAL", panelX, 382, 30, BLACK);
    const std::string total = std::to_string(m_score.total);
    DrawTextFont(
        total.c_str(),
        valueX - MeasureText(total.c_str(), 42),
        375,
        42,
        MAROON
    );
    if (leaderboard.personalBest.available) {
        const std::string personal = "Récord personal local: " +
            std::to_string(leaderboard.personalBest.total);
        DrawTextFont(personal.c_str(), panelX, 435, 16, DARKGRAY);
        DrawTextFont(
            leaderboard.personalBest.currentAttempt
                ? "Nueva mejor marca local"
                : "Tu mejor marca local anterior sigue vigente",
            panelX,
            460,
            13,
            leaderboard.personalBest.currentAttempt ? DARKGREEN : GRAY
        );
    }

    UiButton::Draw(
        UiButton::CenteredAtBottom(width, height, 220.0f, 42.0f, 30.0f),
        "Volver a niveles",
        SKYBLUE
    );
    DrawTextFont("ENTER", width / 2 + 72, height - 57, 10, DARKGRAY);
}

std::optional<std::string> MainGameScene::PersonalRecordOwner() const {
    return m_appContext->identity.ResultOwner(m_runIdentity);
}
