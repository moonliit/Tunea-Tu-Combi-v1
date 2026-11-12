#ifndef GAME_PLAY_SESSION_HPP
#define GAME_PLAY_SESSION_HPP

#include "app/game/BuildModel.hpp"
#include "app/game/ActivationRegistry.hpp"
#include "math/Terrain.hpp"
#include "math/Vector2D.hpp"
#include "physics/I_Physics.hpp"
#include "physics/PhysicsAssembler.hpp"
#include <memory>
#include <vector>

class PlaySession {
private:
    BuildModel& m_model;
    BuildModel m_runtimeModel;
    std::vector<Math::Vector2D> m_terrainVertices;
    Math::TerrainDefinition m_terrainDefinition;
    std::vector<Math::TerrainMesh> m_terrainMeshes;
    Math::TerrainVisualDefinition m_terrainVisual;
    std::unique_ptr<I_PhysicsWorld> m_physics;
    PhysicsAssembler m_assembler;
    ActivationRegistry m_activationRegistry;
    Math::Vector2D m_gravity;
    int m_tntConsumedAmount = 0;
    bool m_damageConsumed = false;
    bool m_breakageConsumed = false;

    void CreatePhysicsWorld(
        const std::vector<std::vector<Math::Vector2D>>& terrainSections,
        bool closed
    );

public:
    PlaySession(
        BuildModel& model,
        const std::vector<Math::Vector2D>& terrainPoints,
        Math::Vector2D gravity
    );
    PlaySession(
        BuildModel& model,
        const std::vector<std::vector<Math::Vector2D>>& terrainSections,
        Math::Vector2D gravity
    );
    PlaySession(
        BuildModel& model,
        const Math::TerrainDefinition& terrain,
        Math::Vector2D gravity
    );

    I_PhysicsWorld& GetPhysics() { return *m_physics; }
    const std::vector<Math::Vector2D>& GetTerrainVertices() const {
        return m_terrainVertices;
    }
    const std::vector<Math::TerrainMesh>& GetTerrainMeshes() const {
        return m_terrainMeshes;
    }
    const Math::TerrainVisualDefinition& GetTerrainVisual() const {
        return m_terrainVisual;
    }
    std::size_t GetJointCount() const {
        return m_assembler.GetJointCount();
    }

    void Start();
    void Reset(const Math::TerrainDefinition& terrain);
    void Update(float deltaTime);
    void ActivateAction(ActivationAction action);
    const ActivationRegistry& GetActivationRegistry() const {
        return m_activationRegistry;
    }
    const EditorGrid& GetRuntimeGrid() const {
        return m_runtimeModel.GetGrid();
    }
    int& GetTntConsumedAmount() { return m_tntConsumedAmount; }
    int GetActiveTntAmount() {
        int count = 0;
        for (const Part* part : m_runtimeModel.GetGrid().GetActiveParts()) {
            if (part->GetType() == PartType::TNTBarrel) {
                count++;
            }
            if (part->HasContainedPart() &&
                part->GetContainedPart()->GetType() == PartType::TNTBarrel) {
                count++;
            }
        }
        return count;
    }
    bool& GetDamageConsumed() { return m_damageConsumed; }
    bool& GetBreakageConsumed() { return m_breakageConsumed; }
    std::vector<std::pair<PartId, PartId>> ConsumeBrokenConnections() {
        return m_assembler.ConsumeBrokenConnections();
    }

private:
    void BuildActivationSubscriptions();
    void ProcessImpacts();
    void RemoveConsumedTnt();
};

#endif
