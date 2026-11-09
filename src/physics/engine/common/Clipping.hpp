#ifndef PHYSICS_COLLISION_CLIPPING_HPP
#define PHYSICS_COLLISION_CLIPPING_HPP

#include "math/Vector2D.hpp"
#include "math/SafeReal.hpp"

// helper function to clip a polygon edge against a plane (Sutherland-Hodgman style step)
inline int ClipSegmentToLine(
    const Math::Vector2D vIn[2], Math::Vector2D vOut[2],
    const Math::Vector2D& normal, Math::RealNum offset
) {
    int numOut = 0;

    Math::RealNum distance0 = vIn[0].Dot(normal) - offset;
    Math::RealNum distance1 = vIn[1].Dot(normal) - offset;

    if (distance0 <= 0.0f) vOut[numOut++] = vIn[0];
    if (distance1 <= 0.0f) vOut[numOut++] = vIn[1];

    if (distance0 * distance1 < 0.0f) {
        Math::Vector2D interpolate = vIn[0] + (vIn[1] - vIn[0]) * (distance0 / (distance0 - distance1));
        vOut[numOut++] = interpolate;
    }

    return numOut;
}


#endif
