#ifndef PHYSICS_VELOCITIES_RESOLUTION_HPP
#define PHYSICS_VELOCITIES_RESOLUTION_HPP

#include "physics/engine/common/Common.hpp"
#include "physics/engine/Joint.hpp"
#include <list>

namespace Physics {
    void ResolveJointVelocities(std::list<std::shared_ptr<BaseJoint>>& joints, float dt);
    Math::RealNum ResolveRevoluteJointVelocity(RevoluteJoint& joint, float dt);
    Math::RealNum ResolveWeldJointVelocity(WeldJoint& joint, float dt);
    void ResolveManifoldVelocities(std::vector<Manifold>& manifolds, float dt);
}

#endif
