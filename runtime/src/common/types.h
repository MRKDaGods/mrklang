#pragma once

#ifndef MRKLANG_COMMON_TYPES_H // Since we have the same header in runtime too
#define MRKLANG_COMMON_TYPES_H

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

template <typename T>
constexpr auto MakeUnique() {
    auto ptr = std::make_unique<T>();
    if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_pointer_v<T>) {
        *ptr = static_cast<T>(0);
    }
    else if constexpr (std::is_class_v<T> && !std::is_trivially_constructible_v<T>) {
        // For non-trivial classes, attempt to zero-initialize if possible
        std::memset(ptr.get(), 0, sizeof(T));
    }
    return ptr;
}

MRK_NS_END

#endif // MRKLANG_COMMON_TYPES_H