#include "app/game/PlaySession.hpp"
#include "app/game/PartDefinition.hpp"
#include "physics/engine/World.hpp"
#include "physics/engine/BaseRigidBody.hpp"

#include <unordered_set>
#include <algorithm>
#include <cmath>

namespace {

constexpr float TntObstacleArmingSeconds = 0.5f;
constexpr float DamageImpulseThreshold = 150.0f;
constexpr float PassengerHurtImpulseThreshold = 80.0f;
constexpr float TntImpactImpulseThreshold = 10.0f;
constexpr float ContainerEjectDamageThreshold = 150.0f;

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
    PlaySession(model, terrain, std::vector<TntObstacleDefinition>{}, gravity) {}

PlaySession::PlaySession(
    BuildModel& model,
    const Math::TerrainDefinition& terrain,
    const std::vector<TntObstacleDefinition>& tntObstacles,
    Math::Vector2D gravity
):
    m_model(model),
    m_runtimeModel(model),
    m_terrainDefinition(terrain),
    m_tntObstacles(tntObstacles),
    m_initialTntObstacles(tntObstacles),
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

    m_tntObstacleParts.clear();
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
    for (const auto& obstacle : m_tntObstacles) {
        auto part = std::make_unique<Part>(
            Math::GridCoord{0, 0}, PartType::TNTBarrel,
            PartOrientation::Up, static_cast<PartId>(1000000 + m_tntObstacleParts.size())
        );
        auto body = m_physics->CreateBox(
            obstacle.position,
            0.0f,
            Math::Vector2D{
                m_model.GetGrid().GetCellSize(),
                m_model.GetGrid().GetCellSize()
            },
            1.0f,
            BodyType::Dynamic
        );
        part->AdoptPhysicsBody(body, m_physics.get(), nullptr);
        part->SetActivationEnabled(false);
        m_tntObstacleParts.push_back(std::move(part));
    }
    m_terrainVertices = terrainSections.empty()
        ? std::vector<Math::Vector2D>{}
        : terrainSections.front();
}

void PlaySession::Start() {
    m_ejectedParts.clear();
    m_tntObstacleLandingProtection = true;
    m_tntObstacleArmingRemaining = TntObstacleArmingSeconds;
    for (const auto& obstacle : m_tntObstacleParts) {
        if (obstacle) obstacle->SetActivationEnabled(false);
    }
    if (m_physics) {
        for (const auto& passenger : m_ejectedPassengers) {
            if (passenger.body) m_physics->RemoveBody(passenger.body);
        }
    }
    m_ejectedPassengers.clear();
    m_carriedPassengersByHost.clear();
    m_ejectedPassengerIds.clear();
    m_explosionVisuals.clear();
    m_damageVisuals.clear();
    m_breakageVisuals.clear();
    m_tntConsumedAmount = 0;
    m_damageConsumed = false;
    m_breakageConsumed = false;
    m_screamingConsumed = false;
    m_runtimeModel = m_model;
    m_assembler.Assemble(m_runtimeModel, m_physics);
    BuildActivationSubscriptions();
}

void PlaySession::Reset(const Math::TerrainDefinition& terrain) {
    m_terrainDefinition = terrain;
    m_tntObstacles = m_initialTntObstacles;
    m_tntObstacleLandingProtection = true;
    m_tntObstacleArmingRemaining = TntObstacleArmingSeconds;
    m_assembler.Clear();
    for (Part* part : m_runtimeModel.GetGrid().GetActiveParts()) {
        part->ResetPhysicsBody();
    }
    m_ejectedParts.clear();
    if (m_physics) {
        for (const auto& passenger : m_ejectedPassengers) {
            if (passenger.body) m_physics->RemoveBody(passenger.body);
        }
    }
    m_ejectedPassengers.clear();
    m_carriedPassengersByHost.clear();
    m_ejectedPassengerIds.clear();
    m_explosionVisuals.clear();
    m_damageVisuals.clear();
    m_breakageVisuals.clear();
    m_tntConsumedAmount = 0;
    m_damageConsumed = false;
    m_breakageConsumed = false;
    m_screamingConsumed = false;

    CreatePhysicsWorld(SampleTerrain(terrain), true);
    m_activationRegistry.Clear();
}

void PlaySession::Update(float deltaTime) {
    m_activationRegistry.Update(deltaTime);
    if (m_tntObstacleArmingRemaining > 0.0f) {
        m_tntObstacleArmingRemaining = std::max(
            0.0f, m_tntObstacleArmingRemaining - deltaTime
        );
        if (m_tntObstacleArmingRemaining <= 0.0f) {
            for (const auto& obstacle : m_tntObstacleParts) {
                if (obstacle) obstacle->SetActivationEnabled(true);
            }
        }
    }
    for (const auto& obstacle : m_tntObstacleParts) {
        if (obstacle) obstacle->UpdateActivation(deltaTime);
    }
    m_physics->Step(deltaTime);
    for (std::size_t index = 0; index < m_tntObstacleParts.size(); ++index) {
        const auto& body = m_tntObstacleParts[index]->GetPhysicsBody();
        if (body && index < m_tntObstacles.size()) {
            m_tntObstacles[index].position = body->GetPosition();
        }
    }
    ProcessImpacts();
    RemoveConsumedTnt();
}

void PlaySession::ProcessImpacts() {
    std::unordered_set<Part*> impactedPartsForTnt;
    std::unordered_set<Part*> ejectedContainers;
    std::unordered_set<Part*> passengerEjectedHosts;
    std::vector<Part*> containersToEject;
    std::vector<Part*> passengerHostsToEject;
    const bool landingProtectionActive =
        m_physics && m_physics->IsLandingImpactProtectionActive();

    const auto impacts = m_physics->ConsumeImpactEvents();
    const auto isLevelObstacle = [this](Part* part) {
        return std::any_of(
            m_tntObstacleParts.begin(), m_tntObstacleParts.end(),
            [part](const std::unique_ptr<Part>& obstacle) {
                return obstacle.get() == part;
            }
        );
    };

    for (const PhysicsImpact& impact : impacts) {
        if (impact.part == nullptr) continue;

        // cualquier golpe fuerte asusta a los pasajeros a bordo
        if (!m_carriedPassengersByHost.empty() &&
            impact.impulse >= DamageImpulseThreshold) {
            m_screamingConsumed = true;
        }

        // container hit: explode contained tnt or eject contained part on strong impact
        if (GetPartDefinition(impact.part->GetType()).role == PartRole::Container &&
            impact.part->HasContainedPart() &&
            impact.impulse >= ContainerEjectDamageThreshold &&
            ejectedContainers.find(impact.part) == ejectedContainers.end()) {
            Part* contained = impact.part->GetContainedPart();
            if (contained != nullptr && contained->GetType() == PartType::TNTBarrel) {
                if (!landingProtectionActive) {
                    if (impactedPartsForTnt.find(contained) == impactedPartsForTnt.end()) {
                        contained->Activate();
                        impactedPartsForTnt.insert(contained);
                    }
                    m_damageConsumed = true;
                    ejectedContainers.insert(impact.part);
                }
            } else if (!landingProtectionActive) {
                containersToEject.push_back(impact.part);
                m_damageConsumed = true;
                ejectedContainers.insert(impact.part);
            }
            continue;
        }

        // carried passenger hit: hurt (scream + damage) and eject on strong impact
        if (!landingProtectionActive &&
            impact.part->GetType() == PartType::SquareBlock &&
            passengerEjectedHosts.find(impact.part) == passengerEjectedHosts.end() &&
            m_carriedPassengersByHost.find(impact.part->GetId()) != m_carriedPassengersByHost.end()) {
            if (impact.impulse >= ContainerEjectDamageThreshold) {
                passengerHostsToEject.push_back(impact.part);
                m_damageConsumed = true;
                m_screamingConsumed = true;
                m_breakageConsumed = true;
                passengerEjectedHosts.insert(impact.part);
                continue;
            }
            if (impact.impulse >= PassengerHurtImpulseThreshold) {
                m_damageConsumed = true;
                m_screamingConsumed = true;
                continue;
            }
        }

        if (
            impact.part->GetType() == PartType::TNTBarrel &&
            (!isLevelObstacle(impact.part) || !m_tntObstacleLandingProtection) &&
            impact.impulse >= TntImpactImpulseThreshold
        ) {
            if (impactedPartsForTnt.find(impact.part) == impactedPartsForTnt.end()) {
                impact.part->Activate();
                impactedPartsForTnt.insert(impact.part);
            }
        } else if (impact.impulse >= DamageImpulseThreshold) {
            Math::Vector2D damagePosition{0.0f, 0.0f};
            if (impact.part != nullptr) {
                if (auto body = impact.part->GetPhysicsBody()) {
                    damagePosition = body->GetPosition();
                } else {
                    damagePosition =
                        impact.part->GetRenderState(m_runtimeModel.GetGrid()).position;
                }
            }
            m_damageVisuals.push_back(DamageVisual{.position = damagePosition});
            m_damageConsumed = true;
        }
    }

    for (Part* container : containersToEject) {
        if (container != nullptr && container->HasContainedPart()) {
            EjectContainedPart(*container, 0.0f);
        }
    }
    if (!containersToEject.empty()) {
        BuildActivationSubscriptions();
    }

    for (Part* host : passengerHostsToEject) {
        if (host != nullptr) {
            EjectCarriedPassenger(*host);
        }
    }

    for (const Math::Vector2D& breakagePos : m_physics->ConsumeBreakagePositions()) {
        m_breakageVisuals.push_back(DamageVisual{.position = breakagePos});
        m_breakageConsumed = true;
    }
    m_tntObstacleLandingProtection = false;
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

    const float cellSize = m_runtimeModel.GetGrid().GetCellSize();
    for (Part* root : containedToRemove) {
        const PartRenderState state =
            root->GetRenderState(m_runtimeModel.GetGrid());
        m_tntConsumedAmount++;
        m_explosionVisuals.push_back(ExplosionVisual{
            .position = state.position,
            .radius = cellSize * 4.0f
        });
        std::unique_ptr<Part> contained = root->RemoveContainedPart();
        if (contained == nullptr) continue;
        m_physics->RemoveBody(contained->GetPhysicsBody());
        contained->ResetPhysicsBody();
    }
    for (const Math::GridCoord coord : rootsToRemove) {
        const auto& part = m_runtimeModel.GetGrid().GetPart(coord);
        if (!part.has_value()) continue;
        m_tntConsumedAmount++;
        m_explosionVisuals.push_back(ExplosionVisual{
            .position = part->GetRenderState(m_runtimeModel.GetGrid()).position,
            .radius = cellSize * 3.0f
        });
        m_physics->RemoveBody(part->GetPhysicsBody());
        m_runtimeModel.GetGrid().RemovePart(coord);
    }
    for (std::size_t index = 0; index < m_tntObstacleParts.size();) {
        Part& obstacle = *m_tntObstacleParts[index];
        if (!obstacle.IsRemovalRequested()) {
            ++index;
            continue;
        }
        const Math::Vector2D position = obstacle.GetPhysicsBody()
            ? obstacle.GetPhysicsBody()->GetPosition()
            : Math::Vector2D{0.0f, 0.0f};
        m_tntConsumedAmount++;
        m_explosionVisuals.push_back(ExplosionVisual{
            .position = position,
            .radius = m_runtimeModel.GetGrid().GetCellSize() * 3.0f
        });
        m_physics->RemoveBody(obstacle.GetPhysicsBody());
        m_tntObstacleParts.erase(m_tntObstacleParts.begin() + index);
        m_tntObstacles.erase(m_tntObstacles.begin() + index);
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
    for (const auto& ejected : m_ejectedParts) {
        if (ejected) {
            subscribePart(*ejected, subscribePart);
        }
    }
}

void PlaySession::EjectContainedPart(Part& container, float) {
    if (!container.HasContainedPart() || !m_physics) return;
    std::unique_ptr<Part> ejected = container.RemoveContainedPart();
    if (!ejected) return;

    const float cellSize = m_runtimeModel.GetGrid().GetCellSize();
    std::shared_ptr<I_RigidBody> containerBody = container.GetPhysicsBody();
    Math::Vector2D position = containerBody
        ? containerBody->GetPosition()
        : m_runtimeModel.GetGrid().CoordToWorld(container.GetGridCoord());
    Math::RealNum rotation = containerBody
        ? containerBody->GetRotation()
        : Math::RealNum{PartOrientationRadians(container.GetOrientation())};

    Math::Vector2D linearVelocity{0.0f, 0.0f};
    Math::RealNum angularVelocity = 0.0f;
    if (auto baseBody = std::dynamic_pointer_cast<Physics::BaseRigidBody>(containerBody)) {
        linearVelocity = baseBody->GetLinearVelocity();
        angularVelocity = baseBody->GetAngularVelocity();
    }

    // spawn outside container to reduce overlap/collision glitches
    const Math::Vector2D outDir = Math::Vector2D::FromAngle(rotation - 1.5707963f);
    const float radius = std::max(4.0f, cellSize * 0.22f);
    const float offsetDist = cellSize * 0.55f;
    Math::Vector2D spawnPos = position + outDir * (offsetDist + radius);
    Math::Vector2D spawnVelocity = (linearVelocity + (outDir * offsetDist * angularVelocity)) * 1.05f;

    float mass = GetPartDefinition(ejected->GetType()).mass;
    if (mass <= 0.0f) mass = 0.5f;

    std::shared_ptr<I_RigidBody> body = m_physics->CreateCircle(
        spawnPos,
        radius,
        mass,
        BodyType::Dynamic
    );
    if (!body) return;

    ejected->AdoptPhysicsBody(body, m_physics.get(), &m_runtimeModel.GetGrid());
    if (auto ejectedBase = std::dynamic_pointer_cast<Physics::BaseRigidBody>(body)) {
        ejectedBase->GetLinearVelocityMut() = spawnVelocity;
        ejectedBase->GetAngularVelocityMut() = angularVelocity;
    }

    m_ejectedParts.push_back(std::move(ejected));
}

void PlaySession::RegisterCarriedPassenger(
    PartId hostId,
    const CarriedPassengerInfo& passenger
) {
    m_carriedPassengersByHost[hostId] = passenger;
}

void PlaySession::UnregisterCarriedPassenger(PartId hostId) {
    m_carriedPassengersByHost.erase(hostId);
}

void PlaySession::ClearCarriedPassengers() {
    m_carriedPassengersByHost.clear();
    m_ejectedPassengerIds.clear();
}

std::vector<std::size_t> PlaySession::ConsumeEjectedPassengerIds() {
    std::vector<std::size_t> ids = std::move(m_ejectedPassengerIds);
    m_ejectedPassengerIds.clear();
    return ids;
}

bool PlaySession::EjectCarriedPassenger(Part& host) {
    if (!m_physics) return false;
    auto it = m_carriedPassengersByHost.find(host.GetId());
    if (it == m_carriedPassengersByHost.end()) return false;

    std::shared_ptr<I_RigidBody> hostBody = host.GetPhysicsBody();
    if (!hostBody) return false;

    const CarriedPassengerInfo passenger = it->second;
    const float cellSize = m_runtimeModel.GetGrid().GetCellSize();
    Math::Vector2D position = hostBody->GetPosition();
    Math::RealNum rotation = hostBody->GetRotation();
    Math::Vector2D linearVelocity{0.0f, 0.0f};
    Math::RealNum angularVelocity = 0.0f;
    if (auto baseBody = std::dynamic_pointer_cast<Physics::BaseRigidBody>(hostBody)) {
        linearVelocity = baseBody->GetLinearVelocity();
        angularVelocity = baseBody->GetAngularVelocity();
    }

    // match contained-part ejection feel/kinematics
    const Math::Vector2D outDir = Math::Vector2D::FromAngle(rotation - 1.5707963f);
    const float radius = std::max(4.0f, cellSize * 0.18f);
    const float offsetDist = cellSize * 0.55f;
    Math::Vector2D spawnPos = position + outDir * (offsetDist + radius);
    Math::Vector2D spawnVelocity =
        (linearVelocity + (outDir * offsetDist * angularVelocity)) * 1.05f;

    const float passengerMass = GetPartDefinition(PartType::Fan).mass;
    std::shared_ptr<I_RigidBody> body = m_physics->CreateCircle(
        spawnPos,
        radius,
        passengerMass,
        BodyType::Dynamic
    );
    if (!body) return false;

    if (auto ejectedBase = std::dynamic_pointer_cast<Physics::BaseRigidBody>(body)) {
        ejectedBase->GetLinearVelocityMut() = spawnVelocity;
        ejectedBase->GetAngularVelocityMut() = angularVelocity;
    }

    hostBody->AddMass(-passengerMass);
    m_carriedPassengersByHost.erase(it);
    m_ejectedPassengerIds.push_back(passenger.id);
    m_ejectedPassengers.push_back(EjectedPassenger{
        .id = passenger.id,
        .hasSkirt = passenger.hasSkirt,
        .face = passenger.face,
        .body = std::move(body)
    });
    return true;
}

void PlaySession::ActivateAction(ActivationAction action) {
    m_activationRegistry.Publish(action);
}
