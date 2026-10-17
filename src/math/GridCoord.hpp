#ifndef MATH_GRID_COORD_HPP
#define MATH_GRID_COORD_HPP

#include "math/Vector2D.hpp"

namespace Math {
    struct GridCoord {
        int x, y;

        bool operator<(const GridCoord& other) const {
            if (x != other.x) return x < other.x;
            return y < other.y;
        }

        Vector2D ToWorldSpace(float cellSize) const {
            return {
                x * cellSize + cellSize / 2.0f,
                y * cellSize + cellSize / 2.0f,
            };
        }
    };
}

#endif
