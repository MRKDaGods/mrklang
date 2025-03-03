#pragma once

#include "macros.h"

#include <utility>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

MRK_NS_BEGIN

// Common type aliases
template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using Vec = std::vector<T>;

template<typename K, typename V>
using Dict = std::unordered_map<K, V>;
using Str = std::string;

// Common method aliases
template <typename T>
constexpr std::remove_reference_t<T>&& Move(T&& value) noexcept {
    return std::move(value);
}

template <typename T, typename... Args>
constexpr auto MakeUnique(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

MRK_NS_END