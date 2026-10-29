#ifndef MATH_SAFE_REAL_HPP
#define MATH_SAFE_REAL_HPP

#include <concepts>

namespace Math {

// concept for an arithmetic/number type
template <typename T>
concept Arithmetic = std::integral<T> || std::floating_point<T>;

// SafeReal handles real number computations safely
// U is the underlying arithmetic primitive type
template <Arithmetic U>
struct SafeReal;

// sfinae thing for detecting if type is of type SafeReal
template <typename T>
struct is_safe_real : std::false_type {};
template <Arithmetic U>
struct is_safe_real<SafeReal<U>> : std::true_type {};

// concept for arithmetic and SafeReal types
template <typename T>
concept ArithmeticOrSafe = Arithmetic<T> || is_safe_real<T>::value;

// macro for simplifying arithmetic memeber function declaration on SafeReal
#define arithmetic(T) template <ArithmeticOrSafe T> inline

template <Arithmetic U>
struct SafeReal {
public:
    static constexpr U epsilon = 1e-8;

    // constructors / destructor / conversions
    constexpr SafeReal(): m_value(static_cast<U>(0)) {}
    arithmetic(T) constexpr SafeReal(T value): m_value(value) {}
    constexpr SafeReal(const SafeReal& other) = default;
    inline operator U() const { return m_value; }
    ~SafeReal() = default;

    // arithmetic
    arithmetic(T) SafeReal operator+(T value) const;
    arithmetic(T) SafeReal operator-(T value) const;
    arithmetic(T) SafeReal operator*(T value) const;
    arithmetic(T) SafeReal operator/(T value) const;

    // assigning
    arithmetic(T) SafeReal& operator=(T value);
    arithmetic(T) SafeReal& operator+=(T value);
    arithmetic(T) SafeReal& operator-=(T value);
    arithmetic(T) SafeReal& operator*=(T value);
    arithmetic(T) SafeReal& operator/=(T value);

    // comparison
    arithmetic(T) bool operator==(T other) const;
    arithmetic(T) bool operator!=(T other) const;
    arithmetic(T) bool operator<(T other) const;
    arithmetic(T) bool operator<=(T other) const;
    arithmetic(T) bool operator>(T other) const;
    arithmetic(T) bool operator>=(T other) const;

    // extras
    inline SafeReal Abs() const;
    inline bool IsZero() const;
    inline int Sign() const;

private:
    U m_value;
};

using RealNum = SafeReal<float>;
}

#include "math/SafeReal.tpp"

#endif
