#ifndef PHYSICS_JOINT_BREAKAGE_CLEANUP_HPP
#define PHYSICS_JOINT_BREAKAGE_CLEANUP_HPP

#include "physics/engine/common/BodyPairHash.hpp"
#include "physics/engine/Joint.hpp"
#include <unordered_set>
#include <queue>
#include <list>

namespace Physics {
    inline void CleanupBrokenJoints(
        std::list<std::shared_ptr<BaseJoint>>& joints,
        std::unordered_set<RigidBodyPair, BodyPairHash>& jointConnectedCache
    ) {
        std::queue<std::list<std::shared_ptr<BaseJoint>>::iterator> brokenJointIters;

        // clear accumulated stress for all joints after the step
        for (auto it = joints.begin(); it != joints.end(); ++it) {
            auto& joint = *it;
            if (!joint) continue;

            if (joint->ShouldBreak()) {
                auto cacheKey = MakeBodyPairKey(joint->GetBodyA(), joint->GetBodyB());
                auto cacheIt = jointConnectedCache.find(cacheKey);
                if (cacheIt != jointConnectedCache.end()) {
                    jointConnectedCache.erase(cacheIt);
                }
                joint->SetIsBroken(true);
                // mark ith joint for removal
                brokenJointIters.push(it);
            }
            joint->ResetStress();
        }

        // remove all marked joints from the vector via pop-swap
        while (!brokenJointIters.empty()) {
            auto popIter = brokenJointIters.front();
            brokenJointIters.pop();
            joints.erase(popIter);
        }
    }
}

#endif
