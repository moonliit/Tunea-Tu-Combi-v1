#ifndef PHYSICS_WORLD_HPP
#define PHYSICS_WORLD_HPP

#include "physics/I_Physics.hpp"
#include "physics/engine/Common.hpp"
#include "physics/engine/BaseRigidBody.hpp"

namespace Physics {

class World : public I_PhysicsWorld {
private:
    std::vector<std::shared_ptr<BaseRigidBody>> m_bodies;
    std::vector<Manifold> m_manifolds;
    Math::Vector2D m_gravity;
    int m_subStepCount;

public:
    explicit World(Math::Vector2D gravity, int subStepCount = 4);
    ~World() override = default;
    void Step(float deltaTime) override;

    std::shared_ptr<I_RigidBody> CreateBox(
        Math::Vector2D position,
        float rotation,
        Math::Vector2D size,
        float mass,
        BodyType type
    ) override;

    std::shared_ptr<I_RigidBody> CreateCircle(
        Math::Vector2D position,
        float radius,
        float mass,
        BodyType type
    ) override;

    std::shared_ptr<I_RigidBody> CreateCapsule(
        Math::Vector2D position,
        float rotation,
        float height,
        float radius,
        float mass,
        BodyType type
    ) override;

    std::shared_ptr<I_RigidBody> CreateTriangle(
        Math::Vector2D position,
        float rotation,
        Math::Vector2D size,
        float mass,
        BodyType type
    ) override;

    std::vector<Math::Vector2D> CreateTerrainChain(
        const std::vector<Math::Vector2D>& controlPoints,
        bool isLoop = false
    ) override;

    std::shared_ptr<I_Joint> CreateRevoluteJoint(
        std::shared_ptr<I_RigidBody> bodyA,
        std::shared_ptr<I_RigidBody> bodyB,
        Math::Vector2D anchor
    ) override;

    std::shared_ptr<I_Joint> CreateWeldJoint(
        std::shared_ptr<I_RigidBody> bodyA,
        std::shared_ptr<I_RigidBody> bodyB,
        Math::Vector2D anchor
    ) override;
};

}

#endif
