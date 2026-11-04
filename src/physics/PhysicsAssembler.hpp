#ifndef PHYSICS_ASSEMBLER_HPP
#define PHYSICS_ASSEMBLER_HPP

#include "game/BuildModel.hpp"
#include "physics/I_Physics.hpp"

#include <memory>
#include <vector>

class PhysicsAssembler {
private:
    std::vector<std::shared_ptr<I_Joint>> m_joints;

public:
    void Clear();
    void Assemble(BuildModel& model, std::unique_ptr<I_PhysicsWorld>& physics);
    std::size_t GetJointCount() const { return m_joints.size(); }
};

#endif
