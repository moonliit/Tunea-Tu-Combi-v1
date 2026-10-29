#ifndef MATH_GRID_COORD_HPP
#define MATH_GRID_COORD_HPP

namespace Math {
    struct GridCoord {
        int x, y;

        bool operator<(const GridCoord& other) const {
            if (x != other.x) return x < other.x;
            return y < other.y;
        }
    };
}

#endif
