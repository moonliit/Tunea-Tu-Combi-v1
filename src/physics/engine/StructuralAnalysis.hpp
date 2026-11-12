#ifndef PHYSICS_STRUCTURAL_GRAPH_HPP
#define PHYSICS_STRUCTURAL_GRAPH_HPP

#include "math/Vector2D.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>

namespace Physics {
    class BaseRigidBody;
    class BaseJoint;
}

namespace Physics::Structural {
    static constexpr Math::RealNum referenceMass = 10.0f;
    static constexpr Math::RealNum massFactor = 0.75f;
    static constexpr Math::RealNum maxMassDamping = 4.0f;
    static constexpr Math::RealNum redundancyDamping = 0.25f;
    static constexpr Math::RealNum maxTorqueDamping = 2.0f;

    using BodyList = std::vector<std::shared_ptr<BaseRigidBody>>;
    using JointList = std::list<std::shared_ptr<BaseJoint>>;
    using BodySet = std::unordered_set<const BaseRigidBody*>;
    using AdjacencyMap = std::unordered_map<const BaseRigidBody*, std::vector<BaseRigidBody*>>;

    struct ComponentLoad {
        BodySet bodies;
        Math::RealNum mass = 0.0f;
        Math::Vector2D centerOfMass = Math::Vector2D::Zero();

        static ComponentLoad Collect(
            const BaseRigidBody& root,
            const AdjacencyMap& adjacency
        );
        static ComponentLoad SelectDependent(
            const ComponentLoad& componentA,
            const ComponentLoad& componentB,
            Math::Vector2D gravity
        );
        Math::RealNum CalculateHeight(Math::Vector2D gravity) const;
    };

    struct LoadData {
        Math::RealNum dependentMass;
        Math::Vector2D dependentCOM;
        Math::Vector2D leverArm;
        Math::RealNum redundancy;
        Math::RealNum massDamping;
        Math::RealNum leverArmDamping;
        bool isBridge = true;

        static LoadData Default();
        static LoadData FromJointDependent(
            const BaseJoint& joint,
            const ComponentLoad& dependent,
            bool isBridge
        );
    };

    class Graph {
    public:
        Graph(Math::Vector2D gravity) : m_gravity(gravity) {}
        void Rebuild(const BodyList& bodies, const JointList& joints);
        LoadData Evaluate(
            const BaseJoint& joint,
            const BaseRigidBody& bodyA,
            const BaseRigidBody& bodyB
        ) const;

    private:
        BodyList m_bodies;
        JointList m_joints;
        Math::Vector2D m_gravity;

        AdjacencyMap BuildAdjacency(const BaseJoint* excludedJoint) const;
    };
}

#endif
