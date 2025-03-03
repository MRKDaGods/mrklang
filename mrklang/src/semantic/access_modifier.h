#pragma once

#include "common/macros.h"

#include <cstdint>
#include <array>
#include <cctype>
#include <string_view>

MRK_NS_BEGIN_MODULE(semantic)

#define ACCESS_MODIFIERS \
	X(PUBLIC, 0) \
	X(PROTECTED, 1) \
	X(PRIVATE, 2) \
	X(INTERNAL, 3) \
	X(STATIC, 4) \
	X(ABSTRACT, 5) \
	X(SEALED, 6) \
	X(VIRTUAL, 7) \
	X(OVERRIDE, 8) \
	X(CONST, 9) \
	X(READONLY, 10) \
	X(EXTERN, 11) \
	X(IMPLICIT, 12) \
	X(EXPLICIT, 13) \
	X(NEW, 14) \
	X(ASYNC, 15)

/// Access modifiers for symbols, only these for now
enum class AccessModifier : uint32_t {
	NONE = 0,

	#define X(x, y) x = 1 << y,
	ACCESS_MODIFIERS
	#undef X
};

inline AccessModifier operator|(AccessModifier lhs, AccessModifier rhs) {
	return static_cast<AccessModifier>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline AccessModifier operator&(AccessModifier lhs, AccessModifier rhs) {
	return static_cast<AccessModifier>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

inline AccessModifier operator~(AccessModifier modifier) {
	return static_cast<AccessModifier>(~static_cast<uint32_t>(modifier));
}

inline AccessModifier& operator|=(AccessModifier& lhs, AccessModifier rhs) {
	lhs = lhs | rhs;
	return lhs;
}

inline AccessModifier& operator&=(AccessModifier& lhs, AccessModifier rhs) {
	lhs = lhs & rhs;
	return lhs;
}

// Utility functions to check for access modifiers

namespace detail {
	#define X(x, y) \
	inline bool is##x(AccessModifier modifier) { \
		return (modifier & AccessModifier::x) != AccessModifier::NONE; \
	}

	ACCESS_MODIFIERS
	#undef X

	constexpr char toLowerChar(char c) {
		return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
	}

	template<size_t N>
	constexpr std::array<char, N> toLowerStr(const char(&str)[N]) {
		std::array<char, N> result{};
		for (size_t i = 0; i < N - 1; i++) {
			result[i] = toLowerChar(str[i]);
		}

		result[N - 1] = '\0';
		return result;
	}

	inline AccessModifier parseAccessModifier(Str modifier) {
		#define X(x, y) if (modifier == std::string_view(toLowerStr(#x).data())) return AccessModifier::x;
		ACCESS_MODIFIERS
		#undef X
			
		return AccessModifier::NONE;
	}

	inline Str formatAccessModifier(AccessModifier modifier) {
		Str result;
		bool first = true;
		#define X(x, y) \
			if (is##x(modifier)) { \
				if (!first) { \
					result += " "; \
				} \
				result += std::string_view(toLowerStr(#x).data()); \
				first = false; \
			}

		ACCESS_MODIFIERS
		#undef X
		return result;
	}
}

#undef ACCESS_MODIFIERS

MRK_NS_END