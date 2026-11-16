#ifndef PHYSICS_WORLD_HPP
#define PHYSICS_WORLD_HPP

#include "math/Vector2D.hpp"
#include "physics/I_Physics.hpp"
#include "physics/engine/common/Common.hpp"
#include "physics/engine/BaseRigidBody.hpp"
#include "physics/engine/Joint.hpp"
#include "physics/engine/StructuralAnalysis.hpp"
#include "physics/engine/common/BodyPairHash.hpp"
#include <unordered_set>
#include <list>
#include <vector>

namespace Physics {

class World : public I_PhysicsWorld {
private:
    struct PendingExplosion {
        Math::Vector2D center{0.0f, 0.0f};
        float breakImpulseForce = 0.0f;
        float launchImpulseForce = 0.0f;
        float radius = 0.0f;
    };

    struct PendingLaunchImpulse {
        std::weak_ptr<BaseRigidBody> body;
        Math::Vector2D impulse{0.0f, 0.0f};
    };

    std::vector<std::shared_ptr<BaseRigidBody>> m_bodies;
    std::list<std::shared_ptr<BaseJoint>> m_joints;
    std::unordered_set<RigidBodyPair, BodyPairHash> m_jointConnectedCache;
    std::vector<Manifold> m_manifolds;
    std::vector<PhysicsImpact> m_impactEvents;
    std::vector<Math::Vector2D> m_breakagePositions;
    Math::Vector2D m_gravity;
    Structural::Graph m_structuralGraph;
    int m_subStepCount;
    int m_velocityIterations;
    int m_positionIterations;
    float m_linearDamping = 0.05f;
    float m_angularDamping = 0.65f;
    std::vector<PendingExplosion> m_pendingExplosions;
    bool m_firstTerrainContactSeen = false;
    float m_firstLandingProtectionRemaining = 0.0f;

    std::vector<PendingLaunchImpulse> ProcessPendingExplosions();

public:
    explicit World(
        Math::Vector2D gravity,
        int subStepCount,
        int velocityIterations,
        int positionIterations
    ) :
        m_gravity(gravity),
        m_structuralGraph(gravity),
        m_subStepCount(subStepCount),
        m_velocityIterations(velocityIterations),
        m_positionIterations(positionIterations) {}

    ~World() override = default;
    void Step(float deltaTime) override;
    std::vector<PhysicsImpact> ConsumeImpactEvents() override;
    std::vector<Math::Vector2D> ConsumeBreakagePositions() override;
    void RemoveBody(
        const std::shared_ptr<I_RigidBody>& body
    ) override;
    void ApplyExplosion(
        const Math::Vector2D& center,
        float breakImpulseForce,
        float launchImpulseForce,
        float radius
    ) override;

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
    bool IsLandingImpactProtectionActive() const override {
        return m_firstLandingProtectionRemaining > 0.0f;
    }
};

}

#endif
