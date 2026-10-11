#ifndef TYPES_HPP
#define TYPES_HPP

#include "common/SafeReal.hpp"

struct Vector2D {
    RealNum x;
    RealNum y;
};

struct Matrix2x2 {
    RealNum a, b;
    RealNum c, d;
};

#endif
