#include "game/PlaySession.hpp"
#include "physics/Box2D_Physics.hpp"

PlaySession::PlaySession(
    BuildModel& model,
    const std::vector<Math::Vector2D>& terrainPoints,
    Math::Vector2D gravity
):
    m_model(model),
    m_gravity(gravity)
{
    CreatePhysicsWorld(terrainPoints);
}

void PlaySession::CreatePhysicsWorld(
    const std::vector<Math::Vector2D>& terrainPoints
) {
    m_physics = std::make_unique<Box2D_PhysicsWorld>(m_gravity);
    m_terrainVertices = m_physics->CreateTerrainChain(terrainPoints);
}

void PlaySession::Start() {
    m_assembler.Assemble(m_model, m_physics);
    BuildActivationSubscriptions();
}

void PlaySession::Reset(const std::vector<Math::Vector2D>& terrainPoints) {
    m_assembler.Clear();
    for (Part* part : m_model.GetGrid().GetActiveParts()) {
        part->ResetPhysicsBody();
    }

    CreatePhysicsWorld(terrainPoints);
    m_activationRegistry.Clear();
}

void PlaySession::Update(float deltaTime) {
    m_activationRegistry.Update(deltaTime);
    m_physics->Step(deltaTime);
}

void PlaySession::BuildActivationSubscriptions() {
    m_activationRegistry.Clear();
    for (Part* part : m_model.GetGrid().GetActiveParts()) {
        switch (part->GetType()) {
            case PartType::Fan:
                m_activationRegistry.Subscribe(
                    GetFanAction(part->GetOrientation()),
                    *part
                );
                break;
            case PartType::WindSail:
                m_activationRegistry.Subscribe(
                    GetWindSailAction(part->GetOrientation()),
                    *part
                );
                break;
            case PartType::TNTBarrel:
                m_activationRegistry.Subscribe(
                    ActivationAction::TNT,
                    *part
                );
                break;
            default:
                break;
        }
    }
}

void PlaySession::ActivateAction(ActivationAction action) {
    m_activationRegistry.Publish(action);
}
