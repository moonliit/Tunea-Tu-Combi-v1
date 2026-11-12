#include "app/game/PlaySession.hpp"
#include "physics/engine/World.hpp"

#include <unordered_set>
#include <algorithm>
#include <cmath>

namespace {

constexpr float DamageImpulseThreshold = 150.0f;
constexpr float TntImpactImpulseThreshold = 10.0f;

bool AreSamePoint(
    const Math::Vector2D& first,
    const Math::Vector2D& second
) {
    constexpr float epsilon = 0.0001f;
    return std::fabs(static_cast<float>(first.x - second.x)) <= epsilon &&
        std::fabs(static_cast<float>(first.y - second.y)) <= epsilon;
}

}

PlaySession::PlaySession(
    BuildModel& model,
    const std::vector<Math::Vector2D>& terrainPoints,
    Math::Vector2D gravity
):
    m_model(model),
    m_runtimeModel(model),
    m_gravity(gravity)
{
    CreatePhysicsWorld(
        std::vector<std::vector<Math::Vector2D>>{terrainPoints},
        false
    );
}

PlaySession::PlaySession(
    BuildModel& model,
    const std::vector<std::vector<Math::Vector2D>>& terrainSections,
    Math::Vector2D gravity
):
    m_model(model),
    m_runtimeModel(model),
    m_gravity(gravity)
{
    Math::TerrainDefinition definition;
    for (const auto& section : terrainSections) {
        definition.polygons.push_back(Math::TerrainPolygon{
            .segments = {
                Math::TerrainSegment{
                    .type = Math::TerrainSegmentType::Line,
                    .points = section
                }
            }
        });
    }
    m_terrainDefinition = definition;
    CreatePhysicsWorld(terrainSections, false);
}

PlaySession::PlaySession(
    BuildModel& model,
    const Math::TerrainDefinition& terrain,
    Math::Vector2D gravity
):
    m_model(model),
    m_runtimeModel(model),
    m_terrainDefinition(terrain),
    m_gravity(gravity)
{
    CreatePhysicsWorld(SampleTerrain(terrain), true);
}

void PlaySession::CreatePhysicsWorld(
    const std::vector<std::vector<Math::Vector2D>>& terrainSections,
    bool closed
) {
    constexpr int subStepCount = 10;
    constexpr int velocityIterations = 8;
    constexpr int positionIterations = 3;

    m_physics = std::make_unique<Physics::World>(
        m_gravity, subStepCount, velocityIterations, positionIterations
    );

    m_terrainMeshes.clear();
    for (const auto& section : terrainSections) {
        std::vector<Math::Vector2D> physicsPoints = section;
        physicsPoints.erase(
            std::unique(
                physicsPoints.begin(),
                physicsPoints.end(),
                [](const Math::Vector2D& first, const Math::Vector2D& second) {
                    return AreSamePoint(first, second);
                }
            ),
            physicsPoints.end()
        );
        if (physicsPoints.size() >= 3 &&
            AreSamePoint(physicsPoints.front(), physicsPoints.back())) {
            physicsPoints.pop_back();
        }
        const bool sectionIsClosed =
            closed &&
            physicsPoints.size() >= 3 &&
            AreSamePoint(section.front(), section.back());
        const std::vector<Math::Vector2D> vertices = sectionIsClosed &&
                m_physics->CreateTerrainPolygon(section)
            ? section
            : m_physics->CreateTerrainChain(physicsPoints, sectionIsClosed);
        if (vertices.size() >= 2) {
            Math::TerrainMesh mesh = BuildTerrainPolygonMesh(section);
            if (mesh.top.empty() && !closed) {
                mesh.top = section;
            }
            if (!mesh.indices.empty()) {
                m_terrainMeshes.push_back(std::move(mesh));
            } else if (!mesh.top.empty()) {
                m_terrainMeshes.push_back(std::move(mesh));
            }
        }
    }
    m_terrainVertices = terrainSections.empty()
        ? std::vector<Math::Vector2D>{}
        : terrainSections.front();
}

void PlaySession::Start() {
    m_runtimeModel = m_model;
    m_assembler.Assemble(m_runtimeModel, m_physics);
    BuildActivationSubscriptions();
}

void PlaySession::Reset(const Math::TerrainDefinition& terrain) {
    m_terrainDefinition = terrain;
    m_assembler.Clear();
    for (Part* part : m_runtimeModel.GetGrid().GetActiveParts()) {
        part->ResetPhysicsBody();
    }

    CreatePhysicsWorld(SampleTerrain(terrain), true);
    m_activationRegistry.Clear();
}

void PlaySession::Update(float deltaTime) {
    m_activationRegistry.Update(deltaTime);
    m_physics->Step(deltaTime);
    ProcessImpacts();
    RemoveConsumedTnt();
}

void PlaySession::ProcessImpacts() {
    std::unordered_set<Part*> impactedPartsForTnt;
    for (const PhysicsImpact& impact : m_physics->ConsumeImpactEvents()) {
        if (
            impact.part != nullptr &&
            impact.part->GetType() == PartType::TNTBarrel &&
            impact.impulse >= TntImpactImpulseThreshold
        ) {
            // todo: draw explosion effect
            impact.part->Activate();
            if (impactedPartsForTnt.find(impact.part) == impactedPartsForTnt.end()) {
                m_tntConsumedAmount++;
                impactedPartsForTnt.insert(impact.part);
            }
        } else if (impact.impulse >= DamageImpulseThreshold) {
            // todo: draw damage effect
            m_damageConsumed = true;
        }
    }
    for (const Math::Vector2D& breakagePos : m_physics->ConsumeBreakagePositions()) {
        // todo: draw breakage effect
        m_breakageConsumed = true;
    }
}

void PlaySession::RemoveConsumedTnt() {
    std::vector<Math::GridCoord> rootsToRemove;
    std::vector<Part*> containedToRemove;

    for (Part* root : m_runtimeModel.GetGrid().GetActiveParts()) {
        if (root->GetType() == PartType::TNTBarrel &&
            root->IsRemovalRequested()) {
            rootsToRemove.push_back(root->GetGridCoord());
            continue;
        }
        if (root->HasContainedPart()) {
            Part* contained = root->GetContainedPart();
            if (contained->GetType() == PartType::TNTBarrel &&
                contained->IsRemovalRequested()) {
                containedToRemove.push_back(root);
            }
        }
    }

    for (Part* root : containedToRemove) {
        std::unique_ptr<Part> contained = root->RemoveContainedPart();
        if (contained == nullptr) continue;
        m_physics->RemoveBody(contained->GetPhysicsBody());
        contained->ResetPhysicsBody();
    }
    for (const Math::GridCoord coord : rootsToRemove) {
        const auto& part = m_runtimeModel.GetGrid().GetPart(coord);
        if (!part.has_value()) continue;
        m_physics->RemoveBody(part->GetPhysicsBody());
        m_runtimeModel.GetGrid().RemovePart(coord);
    }
    if (!rootsToRemove.empty() || !containedToRemove.empty()) {
        BuildActivationSubscriptions();
    }
}

void PlaySession::BuildActivationSubscriptions() {
    m_activationRegistry.Clear();
    const auto subscribePart = [this](Part& part, const auto& self) -> void {
        switch (part.GetType()) {
            case PartType::Fan:
                m_activationRegistry.Subscribe(
                    GetFanAction(part.GetOrientation()),
                    part
                );
                break;
            case PartType::WindSail:
                m_activationRegistry.Subscribe(
                    GetWindSailAction(part.GetOrientation()),
                    part
                );
                break;
            case PartType::TNTBarrel:
                m_activationRegistry.Subscribe(
                    ActivationAction::TNT,
                    part
                );
                break;
            case PartType::Driver:
                m_activationRegistry.Subscribe(
                    ActivationAction::DriverHonk,
                    part
                );
                break;
            default:
                break;
        }
        if (part.HasContainedPart()) {
            self(*part.GetContainedPart(), self);
        }
    };
    for (Part* part : m_runtimeModel.GetGrid().GetActiveParts()) {
        subscribePart(*part, subscribePart);
    }
}

void PlaySession::ActivateAction(ActivationAction action) {
    m_activationRegistry.Publish(action);
}
