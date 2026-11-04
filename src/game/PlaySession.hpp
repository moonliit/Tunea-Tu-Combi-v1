#ifndef GAME_PLAY_SESSION_HPP
#define GAME_PLAY_SESSION_HPP

#include "game/BuildModel.hpp"
#include "game/ActivationRegistry.hpp"
#include "math/Vector2D.hpp"
#include "physics/I_Physics.hpp"
#include "physics/PhysicsAssembler.hpp"

#include <memory>
#include <vector>

class PlaySession {
private:
    BuildModel& m_model;
    std::vector<Math::Vector2D> m_terrainVertices;
    std::unique_ptr<I_PhysicsWorld> m_physics;
    PhysicsAssembler m_assembler;
    ActivationRegistry m_activationRegistry;
    Math::Vector2D m_gravity;

    void CreatePhysicsWorld(const std::vector<Math::Vector2D>& terrainPoints);

public:
    PlaySession(
        BuildModel& model,
        const std::vector<Math::Vector2D>& terrainPoints,
        Math::Vector2D gravity
    );

    I_PhysicsWorld& GetPhysics() { return *m_physics; }
    const std::vector<Math::Vector2D>& GetTerrainVertices() const {
        return m_terrainVertices;
    }
    std::size_t GetJointCount() const {
        return m_assembler.GetJointCount();
    }

    void Start();
    void Reset(const std::vector<Math::Vector2D>& terrainPoints);
    void Update(float deltaTime);
    void ActivateAction(ActivationAction action);
    const ActivationRegistry& GetActivationRegistry() const {
        return m_activationRegistry;
    }

private:
    void BuildActivationSubscriptions();
};

#endif
