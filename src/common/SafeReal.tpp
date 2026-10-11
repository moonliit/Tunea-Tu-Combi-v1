#include "common/SafeReal.hpp"

// arithmetic

template <Arithmetic U>
arithmetic(T) SafeReal<U> SafeReal<U>::operator+(T value) const {
    return SafeReal<U>(m_value + static_cast<U>(value));
}

template <Arithmetic U>
arithmetic(T) SafeReal<U> SafeReal<U>::operator-(T value) const {
    return SafeReal<U>(m_value - static_cast<U>(value));
}

template <Arithmetic U>
arithmetic(T) SafeReal<U> SafeReal<U>::operator*(T value) const {
    return SafeReal<U>(m_value * static_cast<U>(value));
}

template <Arithmetic U>
arithmetic(T) SafeReal<U> SafeReal<U>::operator/(T value) const {
    const U uValue = static_cast<U>(value);
    const SafeReal<U> uSafe = uValue;
    const int uSign = uSafe.Sign();
    const U denom = uValue + epsilon * uSign;
    return SafeReal<U>(m_value / denom);
}

// assigning

template <Arithmetic U>
arithmetic(T) SafeReal<U>& SafeReal<U>::operator=(T value) {
    m_value = static_cast<U>(value);
    return *this;
}

template <Arithmetic U>
arithmetic(T) SafeReal<U>& SafeReal<U>::operator+=(T value) {
    m_value = m_value + static_cast<U>(value);
    return *this;
}

template <Arithmetic U>
arithmetic(T) SafeReal<U>& SafeReal<U>::operator-=(T value) {
    m_value = m_value - static_cast<U>(value);
    return *this;
}

template <Arithmetic U>
arithmetic(T) SafeReal<U>& SafeReal<U>::operator*=(T value) {
    m_value = m_value * static_cast<U>(value);
    return *this;
}

template <Arithmetic U>
arithmetic(T) SafeReal<U>& SafeReal<U>::operator/=(T value) {
    m_value = *this / static_cast<U>(value);
    return *this;
}

// comparison

template <Arithmetic U>
arithmetic(T) bool SafeReal<U>::operator==(T value) const {
    const U uValue = static_cast<U>(value);
    const SafeReal<U> diff = m_value - uValue;
    return diff.Abs().IsZero();
}

template <Arithmetic U>
arithmetic(T) bool SafeReal<U>::operator!=(T value) const {
    return !(*this == value);
}

template <Arithmetic U>
arithmetic(T) bool SafeReal<U>::operator<(T value) const {
    const U uValue = static_cast<U>(value);
    return m_value < uValue - epsilon;
}

template <Arithmetic U>
arithmetic(T) bool SafeReal<U>::operator<=(T value) const {
    const U uValue = static_cast<U>(value);
    return m_value <= uValue + epsilon;
}

template <Arithmetic U>
arithmetic(T) bool SafeReal<U>::operator>(T value) const {
    const U uValue = static_cast<U>(value);
    return m_value > uValue + epsilon;
}

template <Arithmetic U>
arithmetic(T) bool SafeReal<U>::operator>=(T value) const {
    const U uValue = static_cast<U>(value);
    return m_value >= uValue - epsilon;
}

// extras

template <Arithmetic U>
inline SafeReal<U> SafeReal<U>::Abs() const {
    U abs = m_value >= 0 ? m_value : (m_value * -1);
    return SafeReal<U>(abs);
}

template <Arithmetic U>
inline bool SafeReal<U>::IsZero() const {
    return Abs() < epsilon;
}

template <Arithmetic U>
inline int SafeReal<U>::Sign() const {
    if (IsZero()) return 0;
    if (m_value < 0) return -1;
    return 1;
}
