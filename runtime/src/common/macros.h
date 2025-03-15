#pragma once

#ifndef MRKLANG_COMMON_MACROS_H
#define MRKLANG_COMMON_MACROS_H

// No more std::3shn Ali
//#define MRK_STD ::std::

#define MRK_NS mrklang
#define MRK_NS_BEGIN namespace MRK_NS {
#define MRK_NS_BEGIN_MODULE(mod) namespace MRK_NS :: ##mod {
#define MRK_NS_END }

#define IMPLEMENT_FLAGS_OPERATORS_INLINE(name) \
	inline name operator|(name lhs, name rhs) { \
		return static_cast<name>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)); \
	} \
	inline name operator&(name lhs, name rhs) { \
		return static_cast<name>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)); \
	} \
	inline name operator~(name modifier) { \
		return static_cast<name>(~static_cast<uint32_t>(modifier)); \
	} \
	inline name& operator|=(name& lhs, name rhs) { \
		lhs = lhs | rhs; \
		return lhs; \
	} \
	inline name& operator&=(name& lhs, name rhs) { \
		lhs = lhs & rhs; \
		return lhs; \
	} \
	namespace detail { \
		inline bool hasFlag(name value, name flag) { \
			return (value & flag) != name::NONE; \
		} \
	}

#endif // MRKLANG_COMMON_MACROS_H
