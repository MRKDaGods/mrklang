#pragma once

#include "common/types.h"
#include "parameter.h"
#include <utility>
#include <span>

MRK_NS_BEGIN_MODULE(runtime::type_system)

class Type;

using MethodPtr = void*;

class Method {
public:
	Method(const Str& name, Type* returnType, MemberFlags flags = {}, Vec<Parameter> parameters = {})
		: name_(name), returnType_(returnType), parameters_(parameters), nativeMethod_(nullptr) {}

	const Str& getName() const { return name_; }
	Type* getReturnType() const { return returnType_; }
	bool isStatic() const { return flags_.STATIC; }

	void addParameter(const Str& name, Type* paramType, uint32_t flags = 0) {
		parameters_.emplace_back(name, paramType, flags);
	}

	const Vec<Parameter>& getParameters() const { return parameters_; }

	MethodPtr getNativeMethod() const {
		return nativeMethod_;
	}

	void setNativeMethod(MethodPtr nativeMethod) {
		nativeMethod_ = nativeMethod;
	}

private:
	Str name_;
	Type* returnType_;
	MemberFlags flags_;
	Vec<Parameter> parameters_;
	MethodPtr nativeMethod_;
};

MRK_NS_END
