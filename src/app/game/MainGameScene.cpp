#include "app/game/MainGameScene.hpp"
#include "ui/UiButton.hpp"
#include "app/game/Level.hpp"
#include "app/game/PartDefinition.hpp"
#include "math/GridCoord.hpp"
#include "math/Vector2D.hpp"
#include "ui/UiControl.hpp"
#include "app/AppContext.hpp"
#include "app/AudioController.hpp"
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

void DrawPassenger(float x, float y, bool hasSkirt, float scale = 1.0f) {
    DrawCircleV({x, y - 19.0f * scale}, 6.0f * scale, BLACK);
    DrawLineEx({x, y - 13.0f * scale}, {x, y - 2.0f * scale}, 3.0f * scale, BLACK);
    DrawLineEx({x, y - 9.0f * scale}, {x - 7.0f * scale, y - 4.0f * scale}, 2.0f * scale, BLACK);
    DrawLineEx({x, y - 9.0f * scale}, {x + 7.0f * scale, y - 4.0f * scale}, 2.0f * scale, BLACK);
    if (hasSkirt) {
        DrawTriangle({x - 5.0f * scale, y - 2.0f * scale},
            {x + 5.0f * scale, y - 2.0f * scale},
            {x, y + 4.0f * scale}, PINK);
    }
    DrawLineEx({x, y - 2.0f * scale}, {x - 4.0f * scale, y + 5.0f * scale}, 2.0f * scale, BLACK);
    DrawLineEx({x, y - 2.0f * scale}, {x + 4.0f * scale, y + 5.0f * scale}, 2.0f * scale, BLACK);
}

}

MainGameScene::MainGameScene(
    AppContext* appContext,
    const LevelDefinition& specs
):
    m_appContext(appContext),
    m_gravity(9.8f * 40.0f),
    m_isCustomLevel(specs.levelNumber < 0),
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
            m_appContext->audioController->SetCurrentScenario({Scenario::PlayLevel});
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

        // there can only be a single driver box per build
        // if one is already placed, clear any driver box selection
        if (m_buildSession.GetSelectedPartType() == PartType::DriverBox &&
            m_level.GetGrid().ContainsPartType(PartType::DriverBox)) {
            m_buildSession.ClearSelection();
        }
    }
}

void MainGameScene::Update(float dt) {
    if (m_stateMachine.IsResultsMode()) {
        m_leaderboard.Poll(static_cast<double>(dt) * 1000.0);
        return;
    }
    if (m_stateMachine.IsPlayMode()) {
        m_playSession.Update(dt);
        const double frameMs = static_cast<double>(dt) * 1000.0;
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
                std::llround(m_elapsedMs), passengerIndex
            );
        }
        std::optional<Vector2> driverPosition;
        if (!parts.empty()) {
            Vector2 center{0.0f, 0.0f};
            for (const Part* part : parts) {
                const auto position = part->GetRenderState(runtimeGrid).position;
                center.x += static_cast<float>(position.x);
                center.y += static_cast<float>(position.y);
                if (!driverPosition && IsOrContainsDriver(part)) {
                    driverPosition = Vector2{
                        static_cast<float>(position.x),
                        static_cast<float>(position.y)
                    };
                }
            }
            // Follow the driver or the vehicle center, but not outside the map
            m_camera.target = ClampToBounds(
                driverPosition.value_or(Vector2{
                    center.x / parts.size(),
                    center.y / parts.size()
                }),
                m_mapBounds
            );
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
}

void MainGameScene::TransitionToPlayMode() {
    if (!m_level.GetGrid().ContainsPartType(PartType::DriverBox)) return;
    m_elapsedMs = 0.0;
    m_levelCompleted = false;
    m_initialPartIds.clear();
    m_previousPartX.clear();
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
    m_capture.Start(
        m_level.GetBuildModel(),
        m_level.GetDefinition().passengers.size(),
        m_levelId
    );
    m_playSession.Start();
    auto scenario = m_appContext->audioController->GetCurrentScenario();
    m_appContext->audioController->SetCurrentScenario({Scenario::PlayLevel, scenario.levelIndex});

    const EditorGrid& runtimeGrid = m_playSession.GetRuntimeGrid();
    const std::vector<TrackedPart> trackedParts = CollectParts(runtimeGrid);
    for (const TrackedPart& tracked : trackedParts) {
        m_initialPartIds.insert(tracked.part->GetId());
        m_previousPartX.emplace(
            tracked.part->GetId(),
            static_cast<double>(tracked.state.position.x)
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
        for (const Part* part : runtimeParts) {
            const Math::Vector2D position =
                part->GetRenderState(runtimeGrid).position;
            center.x += static_cast<float>(position.x);
            center.y += static_cast<float>(position.y);
            if (!driverPosition && IsOrContainsDriver(part)) {
                driverPosition = Vector2{
                    static_cast<float>(position.x),
                    static_cast<float>(position.y)
                };
            }
        }
        m_camera.target = ClampToBounds(
            driverPosition.value_or(Vector2{
                center.x / runtimeParts.size(),
                center.y / runtimeParts.size()
            }),
            m_mapBounds
        );
    }
}

void MainGameScene::UpdateGoalWindow(double frameMs) {
    const EditorGrid& grid = m_playSession.GetRuntimeGrid();
    const std::vector<TrackedPart> parts = CollectParts(grid);
    std::vector<GoalPartSample> partSamples;
    partSamples.reserve(parts.size());
    for (const TrackedPart& tracked : parts) {
        const PartId id = tracked.part->GetId();
        const double currentX = static_cast<double>(tracked.state.position.x);
        const auto previous = m_previousPartX.find(id);
        partSamples.push_back({
            .id = id,
            .previousX = previous == m_previousPartX.end()
                ? currentX
                : previous->second,
            .currentX = currentX,
            .horizontalRadius = HorizontalRadius(tracked.state),
            .initial = m_initialPartIds.contains(id),
            .driver = tracked.part->GetType() == PartType::Driver
        });
    }
    std::vector<GoalPassengerSample> passengerSamples;
    passengerSamples.reserve(m_carriedPassengers.size());
    for (const CarriedPassenger& passenger : m_carriedPassengers) {
        passengerSamples.push_back({passenger.id, passenger.hostId});
    }
    const double goalX = static_cast<double>(
        m_level.GetDefinition().goal.position.x
    );
    const GoalWindowUpdate update = m_goalWindow.Update(
        m_elapsedMs - frameMs,
        m_elapsedMs,
        goalX,
        partSamples,
        passengerSamples
    );
    if (update.completed && !m_levelCompleted) {
        CompleteRun();
    }

    m_previousPartX.clear();
    for (const TrackedPart& tracked : parts) {
        m_previousPartX.emplace(
            tracked.part->GetId(),
            static_cast<double>(tracked.state.position.x)
        );
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
    m_leaderboard.Submit(
        m_levelId,
        m_capture.GetAttemptId(),
        m_runIdentity,
        m_statistics,
        m_score
    );

    m_appContext->audioController->SetFanActive(false);
    m_appContext->audioController->SetWheelActive(false);
    if (!m_isCustomLevel) {
        m_appContext->completedLevel = m_level.GetDefinition().levelNumber;
    }
    m_stateMachine.EnterResults();
}

void MainGameScene::ResetToBuildMode() {
    m_elapsedMs = 0.0;
    m_goalWindow.Reset();
    m_previousPartX.clear();
    m_capture.Clear();
    m_leaderboard.Reset();
    m_appContext->audioController->SetFanActive(false);
    m_appContext->audioController->SetWheelActive(false);
    if (!m_stateMachine.IsBuildMode()) {
        m_stateMachine.EnterBuild();
    }
    ResetEditorCamera();

    m_playSession.Reset(m_level.GetTerrain());
    auto scenario = m_appContext->audioController->GetCurrentScenario();
    m_appContext->audioController->SetCurrentScenario({Scenario::VehicleBuild, scenario.levelIndex});
    std::fill(m_passengerCollected.begin(), m_passengerCollected.end(), false);
    m_carriedPassengers.clear();
}

void MainGameScene::ResetEditorCamera() {
    m_camera.target = m_editorCameraTarget;
    m_camera.zoom = m_editorCameraZoom;
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
        m_playSession.GetActivationRegistry()
    );
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
        const char* label = "¡Ya llegó el chofer, causa!";
        DrawText(
            label,
            static_cast<int>(centerX - MeasureText(label, 30) / 2),
            static_cast<int>(centerY - 90.0f),
            30,
            WHITE
        );
        const char* number = TextFormat("%d", count);
        DrawText(
            number,
            static_cast<int>(centerX - MeasureText(number, 120) / 2),
            static_cast<int>(centerY - 35.0f),
            120,
            WHITE
        );
    }
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
                100.0f,
                32.0f,
                8.0f
            ),
            "DISEÑO",
            DARKGRAY,
            WHITE,
            14
        );
    }

    EndDrawing();
}

void MainGameScene::RenderResults() const {
    const int width = m_appContext->windowWidth;
    const int height = m_appContext->windowHeight;
    DrawText("Resultados", 36, 28, 34, BLACK);

    const Rectangle rankingPanel{30.0f, 82.0f, width * 0.44f, height - 165.0f};
    DrawRectangleRec(rankingPanel, Fade(LIGHTGRAY, 0.55f));
    DrawRectangleLinesEx(rankingPanel, 2.0f, GRAY);
    DrawText("Ranking", 52, 106, 26, DARKGRAY);
    const LeaderboardSnapshot& leaderboard = m_leaderboard.GetSnapshot();
    if (leaderboard.status == LeaderboardStatus::Loading) {
        DrawText("Esperando confirmación del servidor...", 52, 150, 15, GRAY);
    } else if (leaderboard.status == LeaderboardStatus::Unavailable) {
        DrawText("Ranking global no disponible.", 52, 150, 17, MAROON);
        if (leaderboard.personalBest.available) {
            const std::string local = "Récord personal local: " +
                std::to_string(leaderboard.personalBest.total);
            DrawText(local.c_str(), 52, 183, 16, DARKGRAY);
        }
    } else if (leaderboard.status == LeaderboardStatus::Ready) {
        if (leaderboard.entries.empty()) {
            DrawText("Aún no hay marcas clasificadas.", 52, 150, 16, GRAY);
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
            DrawText(label.c_str(), 50, rowY + 5, 16, own ? BLACK : DARKGRAY);
            const std::string totalValue = std::to_string(entry.total);
            DrawText(
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
            DrawText(detail.c_str(), 50, rowY + 28, 12, own ? MAROON : GRAY);
            rowY += 53;
        }
        DrawText(
            leaderboard.submissionConfirmed
                ? "Puntos recalculados; física no verificada."
                : "Resultado local no enviado al ranking.",
            46,
            static_cast<int>(rankingPanel.y + rankingPanel.height - 27.0f),
            11,
            GRAY
        );
    }

    const int panelX = static_cast<int>(width * 0.52f);
    const int valueX = width - 55;
    DrawText("Puntuación", panelX, 86, 28, BLACK);
    const auto row = [panelX, valueX](
        const std::string& label,
        std::int64_t value,
        int y
    ) {
        DrawText(label.c_str(), panelX, y, 18, DARKGRAY);
        const std::string number = std::to_string(value);
        DrawText(
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
    DrawText("TOTAL", panelX, 382, 30, BLACK);
    const std::string total = std::to_string(m_score.total);
    DrawText(
        total.c_str(),
        valueX - MeasureText(total.c_str(), 42),
        375,
        42,
        MAROON
    );
    if (leaderboard.personalBest.available) {
        const std::string personal = "Récord personal local: " +
            std::to_string(leaderboard.personalBest.total);
        DrawText(personal.c_str(), panelX, 435, 16, DARKGRAY);
        DrawText(
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
    DrawText("ENTER", width / 2 + 72, height - 57, 10, DARKGRAY);
}

std::optional<std::string> MainGameScene::PersonalRecordOwner() const {
    return m_appContext->identity.ResultOwner(m_runIdentity);
}
