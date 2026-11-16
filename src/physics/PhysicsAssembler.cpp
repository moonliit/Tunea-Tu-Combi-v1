#include "physics/PhysicsAssembler.hpp"

#include <unordered_map>
#include <unordered_set>

namespace {

bool ContainsWheel(const PartType first, const PartType second) {
    return first == PartType::CircleWheel ||
        second == PartType::CircleWheel;
}

void AttachContainedPart(
    Part& part,
    I_PhysicsWorld* physicsWorld,
    EditorGrid& grid
) {
    if (!part.HasContainedPart()) return;
    Part* contained = part.GetContainedPart();
    contained->ResetPhysicsBody();
    contained->SetContainedBody(part.GetPhysicsBody(), physicsWorld, &grid);
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
        AttachContainedPart(*part, physics.get(), model.GetGrid());
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

bool PhysicsAssembler::ArePartsConnected(PartId first, PartId second) const {
    if (first == second) return true;

    std::unordered_map<PartId, std::vector<PartId>> graph;
    for (std::size_t index = 0; index < m_jointParts.size(); ++index) {
        if (index < m_joints.size() && m_joints[index] &&
            m_joints[index]->IsBroken()) {
            continue;
        }
        const auto [left, right] = m_jointParts[index];
        graph[left].push_back(right);
        graph[right].push_back(left);
    }

    std::vector<PartId> pending{first};
    std::unordered_set<PartId> visited{first};
    while (!pending.empty()) {
        const PartId current = pending.back();
        pending.pop_back();
        const auto iterator = graph.find(current);
        if (iterator == graph.end()) continue;
        for (const PartId neighbor : iterator->second) {
            if (neighbor == second) return true;
            if (visited.insert(neighbor).second) {
                pending.push_back(neighbor);
            }
        }
    }
    return false;
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
