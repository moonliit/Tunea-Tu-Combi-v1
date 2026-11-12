#include "physics/PhysicsAssembler.hpp"

#include <unordered_map>

namespace {

bool ContainsWheel(const PartType first, const PartType second) {
    return first == PartType::CircleWheel ||
        second == PartType::CircleWheel;
}

void AttachContainedPart(
    Part& part,
    I_PhysicsWorld* physicsWorld
) {
    if (!part.HasContainedPart()) return;
    Part* contained = part.GetContainedPart();
    contained->ResetPhysicsBody();
    contained->SetContainedBody(part.GetPhysicsBody(), physicsWorld);
}

}

void PhysicsAssembler::Clear() {
    m_joints.clear();
    m_jointParts.clear();
    m_reportedBroken.clear();
}

void PhysicsAssembler::Assemble(
    BuildModel& model,
    std::unique_ptr<I_PhysicsWorld>& physics
) {
    Clear();

    std::unordered_map<PartId, std::shared_ptr<I_RigidBody>> bodies;
    std::unordered_map<PartId, Part*> parts;
    for (Part* part : model.GetGrid().GetActiveParts()) {
        part->SetPhysicsBody(model.GetGrid(), physics);
        AttachContainedPart(*part, physics.get());
        bodies.emplace(part->GetId(), part->GetPhysicsBody());
        parts.emplace(part->GetId(), part);
    }

    for (const PartConnection& connection : model.GetConnections()) {
        const auto first = bodies.find(connection.first);
        const auto second = bodies.find(connection.second);
        if (first == bodies.end() || second == bodies.end()) {
            continue;
        }

        std::shared_ptr<I_Joint> joint;
        const auto firstPart = parts.find(connection.first);
        const auto secondPart = parts.find(connection.second);
        if (firstPart == parts.end() || secondPart == parts.end()) {
            continue;
        }

        if (ContainsWheel(
                firstPart->second->GetType(),
                secondPart->second->GetType()
            )) {
            if (firstPart->second->GetType() == PartType::CircleWheel) {
                firstPart->second->SetAttachmentBody(second->second);
            } else if (secondPart->second->GetType() ==
                       PartType::CircleWheel) {
                secondPart->second->SetAttachmentBody(first->second);
            }
            Math::Vector2D wheelAnchor = connection.anchor;
            if (firstPart->second->GetType() == PartType::CircleWheel) {
                wheelAnchor = first->second->GetPosition();
            } else if (secondPart->second->GetType() ==
                       PartType::CircleWheel) {
                wheelAnchor = second->second->GetPosition();
            }
            joint = physics->CreateRevoluteJoint(
                first->second,
                second->second,
                wheelAnchor
            );
        } else {
            joint = physics->CreateWeldJoint(
                first->second,
                second->second,
                connection.anchor
            );
        }

        if (joint != nullptr) {
            m_joints.push_back(std::move(joint));
            m_jointParts.emplace_back(connection.first, connection.second);
            m_reportedBroken.push_back(false);
        }
    }
}

std::vector<std::pair<PartId, PartId>>
PhysicsAssembler::ConsumeBrokenConnections() {
    std::vector<std::pair<PartId, PartId>> broken;
    for (std::size_t index = 0; index < m_joints.size(); ++index) {
        if (!m_reportedBroken[index] && m_joints[index]->IsBroken()) {
            m_reportedBroken[index] = true;
            broken.push_back(m_jointParts[index]);
        }
    }
    return broken;
}
