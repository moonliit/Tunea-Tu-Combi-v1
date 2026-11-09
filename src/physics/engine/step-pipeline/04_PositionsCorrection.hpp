#ifndef PHYSICS_POSITIONS_CORRECTION_HPP
#define PHYSICS_POSITIONS_CORRECTION_HPP

#include "physics/engine/common/Common.hpp"
#include "physics/engine/Joint.hpp"

namespace Physics {
    void CorrectJointPositions(std::vector<std::shared_ptr<BaseJoint>>& manifolds);
    void CorrectRevoluteJointPosition(RevoluteJoint& joint);
    void CorrectWeldJointPosition(WeldJoint& joint);
    void CorrectManifoldPositions(std::vector<Manifold>& manifolds);
}

#endif
