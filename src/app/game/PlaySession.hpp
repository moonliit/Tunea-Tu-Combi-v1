#ifndef GAME_PLAY_SESSION_HPP
#define GAME_PLAY_SESSION_HPP

#include "app/game/BuildModel.hpp"
#include "app/game/Level.hpp"
#include "app/game/ActivationRegistry.hpp"
#include "math/Terrain.hpp"
#include "math/Vector2D.hpp"
#include "physics/I_Physics.hpp"
#include "physics/PhysicsAssembler.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct EjectedPassenger {
    std::size_t id = 0;
    bool hasSkirt = false;
    std::string face;
    std::shared_ptr<I_RigidBody> body;
};

struct CarriedPassengerInfo {
    std::size_t id = 0;
    bool hasSkirt = false;
    std::string face;
};

struct ExplosionVisual {
    Math::Vector2D position{0.0f, 0.0f};
    float radius = 1.0f;
};

struct DamageVisual {
    Math::Vector2D position{0.0f, 0.0f};
};

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
    std::vector<std::unique_ptr<Part>> m_ejectedParts;
    std::vector<EjectedPassenger> m_ejectedPassengers;
    std::unordered_map<PartId, CarriedPassengerInfo> m_carriedPassengersByHost;
    std::vector<std::size_t> m_ejectedPassengerIds;
    std::vector<ExplosionVisual> m_explosionVisuals;
    std::vector<DamageVisual> m_damageVisuals;
    std::vector<DamageVisual> m_breakageVisuals;
    std::vector<TntObstacleDefinition> m_tntObstacles;
    std::vector<TntObstacleDefinition> m_initialTntObstacles;
    std::vector<std::unique_ptr<Part>> m_tntObstacleParts;
    bool m_tntObstacleLandingProtection = true;
    float m_tntObstacleArmingRemaining = 0.5f;
    int m_tntConsumedAmount = 0;
    bool m_damageConsumed = false;
    bool m_breakageConsumed = false;
    bool m_screamingConsumed = false;

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
    PlaySession(
        BuildModel& model,
        const Math::TerrainDefinition& terrain,
        const std::vector<TntObstacleDefinition>& tntObstacles,
        Math::Vector2D gravity
    );

    I_PhysicsWorld& GetPhysics() { return *m_physics; }
    const std::vector<Math::Vector2D>& GetTerrainVertices() const {
        return m_terrainVertices;
    }
    const std::vector<Math::TerrainMesh>& GetTerrainMeshes() const {
        return m_terrainMeshes;
    }
    const std::vector<TntObstacleDefinition>& GetTntObstacles() const {
        return m_tntObstacles;
    }
    const Math::TerrainVisualDefinition& GetTerrainVisual() const {
        return m_terrainVisual;
    }
    const std::vector<std::unique_ptr<Part>>& GetEjectedParts() const {
        return m_ejectedParts;
    }
    const std::vector<EjectedPassenger>& GetEjectedPassengers() const {
        return m_ejectedPassengers;
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
    bool& GetScreamingConsumed() { return m_screamingConsumed; }
    std::vector<std::pair<PartId, PartId>> ConsumeBrokenConnections() {
        return m_assembler.ConsumeBrokenConnections();
    }
    std::vector<ExplosionVisual> ConsumeExplosionVisuals() {
        std::vector<ExplosionVisual> visuals = std::move(m_explosionVisuals);
        m_explosionVisuals.clear();
        return visuals;
    }
    std::vector<DamageVisual> ConsumeDamageVisuals() {
        std::vector<DamageVisual> visuals = std::move(m_damageVisuals);
        m_damageVisuals.clear();
        return visuals;
    }
    std::vector<DamageVisual> ConsumeBreakageVisuals() {
        std::vector<DamageVisual> visuals = std::move(m_breakageVisuals);
        m_breakageVisuals.clear();
        return visuals;
    }
    void RegisterCarriedPassenger(PartId hostId, const CarriedPassengerInfo& passenger);
    void UnregisterCarriedPassenger(PartId hostId);
    void ClearCarriedPassengers();
    std::vector<std::size_t> ConsumeEjectedPassengerIds();



private:
    void BuildActivationSubscriptions();
    void ProcessImpacts();
    void RemoveConsumedTnt();
    void EjectContainedPart(Part& container, float impactImpulse);
    bool EjectCarriedPassenger(Part& host);
};

#endif
