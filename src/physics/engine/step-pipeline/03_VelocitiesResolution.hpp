#ifndef PHYSICS_VELOCITIES_RESOLUTION_HPP
#define PHYSICS_VELOCITIES_RESOLUTION_HPP

#include "physics/engine/common/Common.hpp"
#include "physics/engine/Joint.hpp"

namespace Physics {
    void ResolveJointVelocities(std::vector<std::shared_ptr<BaseJoint>>& joints, float dt);
    void ResolveRevoluteJointVelocity(RevoluteJoint& joint, float dt);
    void ResolveWeldJointVelocity(WeldJoint& joint, float dt);
    void ResolveManifoldVelocities(std::vector<Manifold>& manifolds, float dt);
}

#endif
