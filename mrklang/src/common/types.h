#pragma once

#include "macros.h"

#include <utility>
#include <memory>
#include <vector>
#include <string>

MRK_NS_BEGIN

// Common type aliases
template<typename T>
using UniquePtr = MRK_STD unique_ptr<T>;

template<typename T>
using Vec = MRK_STD vector<T>;
using Str = MRK_STD string;

// Common method aliases
template <typename T>
constexpr MRK_STD remove_reference_t<T>&& Move(T&& value) noexcept {
    return MRK_STD move(value);
}

template <typename T, typename... Args>
constexpr auto MakeUnique(Args&&... args) {
    return MRK_STD make_unique<T>(MRK_STD forward<Args>(args)...);
}

MRK_NS_END