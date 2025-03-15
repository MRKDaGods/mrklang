#pragma once

#include "common/types.h"
#include <utility>
#include <span>

MRK_NS_BEGIN_MODULE(runtime::type_system)

class Type;

class Method {
public:
	Method(const Str& name, Type* returnType)
		: name_(name), returnType_(returnType) {}

	const Str& getName() const { return name_; }
	Type* getReturnType() const { return returnType_; }

	void addParameter(const Str& name, Type* paramType) {
		parameters_.emplace_back(name, paramType);
	}

	// Why not return a vector directly?
	// Encapsulation.
	std::span<const std::pair<Str, Type*>> getParameters() const {
		return std::span<const std::pair<Str, Type*>>(parameters_.data(), parameters_.size());
	}

private:
	Str name_;
	Type* returnType_;
	Vec<std::pair<Str, Type*>> parameters_;
};

MRK_NS_END
