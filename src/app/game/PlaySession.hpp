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
    std::vector<Math::Vector2D> m_terrainVertices;
    Math::TerrainDefinition m_terrainDefinition;
    std::vector<Math::TerrainMesh> m_terrainMeshes;
    Math::TerrainVisualDefinition m_terrainVisual;
    std::unique_ptr<I_PhysicsWorld> m_physics;
    PhysicsAssembler m_assembler;
    ActivationRegistry m_activationRegistry;
    Math::Vector2D m_gravity;

    void CreatePhysicsWorld(const std::vector<Math::Vector2D>& terrainPoints);
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
    void Reset(const std::vector<Math::Vector2D>& terrainPoints);
    void Reset(
        const std::vector<std::vector<Math::Vector2D>>& terrainSections
    );
    void Reset(const Math::TerrainDefinition& terrain);
    void Update(float deltaTime);
    void ActivateAction(ActivationAction action);
    const ActivationRegistry& GetActivationRegistry() const {
        return m_activationRegistry;
    }

private:
    void BuildActivationSubscriptions();
};

#endif
