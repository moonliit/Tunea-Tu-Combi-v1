#ifndef PHYSICS_CONTACT_CANDIDATE_HPP
#define PHYSICS_CONTACT_CANDIDATE_HPP

#include "math/Vector2D.hpp"

namespace Physics {
    struct ContactCandidate {
        Math::Vector2D position;
        Math::RealNum depth;
        Math::RealNum tangentPos;
    };

    // evaluates a point against terrain edge and collect valid candidates
    inline void EvaluatePoint(
        const Math::Vector2D& pt,
        const Math::Vector2D& p1,
        const Math::Vector2D& p2,
        const Math::Vector2D& edgeNormal,
        const Math::Vector2D& edgeDir,
        Math::RealNum edgeLength,
        std::vector<Physics::ContactCandidate>& candidates
    ) {
        constexpr Math::RealNum contactTolerance = 2.5f;
        constexpr Math::RealNum edgeDistThreshold = 0.1f;

        Math::RealNum signedDist = (pt - p1).Dot(edgeNormal);
        Math::RealNum depth = -signedDist;
        if (depth < -contactTolerance) return;

        Math::RealNum tanPos = (pt - p1).Dot(edgeDir);
        if (tanPos < -edgeDistThreshold || tanPos > edgeLength + edgeDistThreshold) return;

        bool duplicate = false;
        for (const auto& c: candidates) {
            if ((c.position - pt).LengthSquared() < Math::RealNum::epsilon) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            candidates.push_back({
                pt,
                std::max(Math::RealNum(0), depth),
                tanPos
            });
        }
    }
}

#endif
