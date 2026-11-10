#ifndef PHYSICS_WORLD_HPP
#define PHYSICS_WORLD_HPP

#include "physics/I_Physics.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/BaseRigidBody.hpp"
#include "physics/engine/Joint.hpp"
#include "physics/engine/common/BodyPairHash.hpp"
#include <unordered_set>

namespace Physics {

class World : public I_PhysicsWorld {
private:
    std::vector<std::shared_ptr<BaseRigidBody>> m_bodies;
    std::vector<std::shared_ptr<BaseJoint>> m_joints;
    std::unordered_set<RigidBodyPair, BodyPairHash> m_jointConnectedCache;
    std::vector<Manifold> m_manifolds;
    Math::Vector2D m_gravity;
    int m_subStepCount;
    int m_velocityIterations;
    int m_positionIterations;

public:
    explicit World(
        Math::Vector2D gravity,
        int subStepCount,
        int velocityIterations,
        int positionIterations
    ) :
        m_gravity(gravity),
        m_subStepCount(subStepCount),
        m_velocityIterations(velocityIterations),
        m_positionIterations(positionIterations) {}

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

    bool CreateTerrainPolygon(
        const std::vector<Math::Vector2D>& points
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

    const std::vector<std::shared_ptr<BaseRigidBody>>& GetBodies() const { return m_bodies; }
};

}

#endif
