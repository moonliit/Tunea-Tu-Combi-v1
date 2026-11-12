#ifndef MATH_INTERVAL_HPP
#define MATH_INTERVAL_HPP

#include "math/SafeReal.hpp"
#include <algorithm>
#include <optional>

namespace Math {

    struct Interval {
        RealNum min;
        RealNum max;

        Interval(): min(RealNum()), max(RealNum()) {}
        Interval(const RealNum& min, const RealNum& max): min(min), max(max) {}

        constexpr inline bool Contains(const RealNum& value) const {
            return (
                min <= value && value <= max
            );
        }
        constexpr inline bool Overlaps(const Interval& other) const {
            return (
                min <= other.max && other.min <= max
            );
        }
        constexpr inline std::optional<RealNum> GetOverlap(const Interval& other) const {
            if (!Overlaps(other)) return std::nullopt;
            return std::min(max - other.min, other.max - min);
        }
    };

}

#endif
