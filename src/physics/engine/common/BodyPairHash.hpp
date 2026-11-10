#ifndef BODY_PAIR_HASH_HPP
#define BODY_PAIR_HASH_HPP

#include "physics/I_Physics.hpp"
#include <utility>

using RigidBodyPair = std::pair<const I_RigidBody*, const I_RigidBody*>;

// Custom hash for an unordered pair of body pointers
struct BodyPairHash {
    template <typename T1, typename T2>
    size_t operator()(const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

// Canonical pair order so (A, B) and (B, A) generate the same key
inline RigidBodyPair MakeBodyPairKey(
    const std::shared_ptr<I_RigidBody>& a,
    const std::shared_ptr<I_RigidBody>& b
) {
    if (a.get() < b.get()) {
        return {a.get(), b.get()};
    }
    return {b.get(), a.get()};
}

#endif
