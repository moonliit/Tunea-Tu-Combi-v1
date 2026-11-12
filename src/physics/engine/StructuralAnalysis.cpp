#include "physics/engine/StructuralAnalysis.hpp"
#include "physics/engine/BaseRigidBody.hpp"
#include "physics/engine/Joint.hpp"
#include <algorithm>
#include <cmath>
#include <queue>
using namespace Math;
using namespace std;

namespace Physics::Structural {

void Graph::Rebuild(const BodyList& bodies, const JointList& joints) {
    m_bodies = bodies;
    m_joints = joints;
}

AdjacencyMap Graph::BuildAdjacency(const BaseJoint* excludedJoint) const {
    AdjacencyMap adjacency;
    for (const auto& joint : m_joints) {
        if (!joint || joint.get() == excludedJoint) continue;

        const auto bodyA = joint->GetBodyA();
        const auto bodyB = joint->GetBodyB();
        if (!bodyA || !bodyB) continue;

        adjacency[bodyA.get()].push_back(bodyB.get());
        adjacency[bodyB.get()].push_back(bodyA.get());
    }
    return adjacency;
}

RealNum ComponentLoad::CalculateHeight(Vector2D gravity) const {
    if (bodies.empty()) return 0.0f;
    const Vector2D center = mass > 0.0f
        ? centerOfMass
        : (*bodies.begin())->GetTransform().position;
    return center.Dot(gravity);
}

ComponentLoad ComponentLoad::Collect(
    const BaseRigidBody& root,
    const AdjacencyMap& adjacency
) {
    ComponentLoad component;

    queue<const BaseRigidBody*> pending;
    pending.push(&root);

    while (!pending.empty()) {
        const BaseRigidBody* body = pending.front();
        pending.pop();
        if (!component.bodies.insert(body).second) continue;

        const auto neighbors = adjacency.find(body);
        if (neighbors == adjacency.end()) continue;

        for (BaseRigidBody* neighbor : neighbors->second) {
            pending.push(neighbor);
        }
    }

    for (const BaseRigidBody* body : component.bodies) {
        const RealNum mass = body->GetMassData().mass;
        component.centerOfMass += body->GetTransform().position * mass;
        component.mass += mass;
    }
    if (component.mass > 0.0f) {
        component.centerOfMass /= component.mass;
    }
    return component;
}

ComponentLoad ComponentLoad::SelectDependent(
    const ComponentLoad& componentA,
    const ComponentLoad& componentB,
    Vector2D gravity
) {
    const RealNum heightA = componentA.CalculateHeight(gravity);
    const RealNum heightB = componentB.CalculateHeight(gravity);
    if (heightA < heightB) return componentA;
    if (heightB < heightA) return componentB;
    return componentA.mass > componentB.mass ? componentA : componentB;
}

LoadData LoadData::Default() {
    return LoadData {
        .dependentMass = 0.0f,
        .dependentCOM = Math::Vector2D::Zero(),
        .leverArm = Math::Vector2D::Zero(),
        .redundancy = 0.0f,
        .massDamping = 1.0f,
        .leverArmDamping = 1.0f,
        .isBridge = true
    };
}

LoadData LoadData::FromJointDependent(
    const BaseJoint& joint,
    const ComponentLoad& dependent,
    bool isBridge
) {
    const auto bodyA = joint.GetBodyA();
    if (!bodyA) return LoadData::Default();

    const Vector2D anchor = bodyA->GetTransform().ToWorldPoint(joint.GetLocalAnchorA());
    const RealNum gameplayMass = isBridge ? dependent.mass : dependent.mass * 0.35f;
    const Vector2D leverArm = dependent.centerOfMass - anchor;

    return LoadData {
        .dependentMass = dependent.mass,
        .dependentCOM = dependent.centerOfMass,
        .leverArm = leverArm,
        .redundancy = RealNum(!isBridge),
        .massDamping = clamp(
            RealNum(1) + massFactor * sqrt(
                max(RealNum(0), gameplayMass / referenceMass)
            ),
            RealNum(1),
            maxMassDamping
        ),
        .leverArmDamping = clamp(
            RealNum(1 + leverArm.Length() / 8),
            RealNum(1),
            maxTorqueDamping
        ),
        .isBridge = isBridge
    };
}

LoadData Graph::Evaluate(
    const BaseJoint& joint,
    const BaseRigidBody& bodyA,
    const BaseRigidBody& bodyB
) const {
    const AdjacencyMap adjacency = BuildAdjacency(&joint);
    const ComponentLoad componentA = ComponentLoad::Collect(bodyA, adjacency);
    const ComponentLoad componentB = ComponentLoad::Collect(bodyB, adjacency);
    const bool isBridge = componentA.bodies != componentB.bodies;
    const ComponentLoad dependent = isBridge
        ? ComponentLoad::SelectDependent(componentA, componentB, m_gravity)
        : componentB;

    return LoadData::FromJointDependent(joint, dependent, isBridge);
}

}
