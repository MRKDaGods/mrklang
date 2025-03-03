#pragma once

#include "common/types.h"

MRK_NS_BEGIN

template<typename Collection, typename Getter = nullptr_t>
inline Str formatCollection(const Collection& collection, const Str& separator = ", ", Getter&& getter = nullptr) {
	Str result;
	bool first = true;

	for (const auto& item : collection) {
		if (!first) {
			result += separator;
		}

		first = false;

		// Use the custom getter if provided
		if constexpr (!std::is_same_v<std::decay_t<Getter>, std::nullptr_t>) {
			if constexpr (requires(Getter g, decltype(item) i) { { g(i) } -> std::convertible_to<Str>; }) {
				result += std::forward<Getter>(getter)(item);
				continue;
			}
		}

		if constexpr (requires { item->toString(); }) {
			result += item->toString();
		}
		else if constexpr (requires { item.toString(); }) {
			result += item.toString();
		}
		else if constexpr (requires { item.lexeme; }) {
			result += item.lexeme;
		}
		else {
			result += "??";
		}
	}

	return result;
}

template<typename Collection, typename Getter>
inline Str formatCollection(const Collection& collection, Getter&& getter) {
	return formatCollection(collection, ", ", std::forward<Getter>(getter));
}

MRK_NS_END