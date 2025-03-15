#pragma once

#include "type.h"

MRK_NS_BEGIN_MODULE(runtime::type_system)

// Built-in primitive types
class PrimitiveType : public Type {
public:
	PrimitiveType(TypeKind kind, const Str& name, size_t size)
		: kind_(kind), name_(name), size_(size) {}

	Str getName() const override { return name_; }
	Str getFullName() const override { return MRK_STL_NAME "::" + name_; }
	TypeKind getTypeKind() const override { return kind_; }
	size_t getSize() const override { return size_; }
	bool isValueType() const override { return true; }
	bool isClass() const override { return false; }
	bool isPrimitive() const override { return true; }
	bool isArray() const override { return false; }

private:
	TypeKind kind_;
	Str name_;
	size_t size_;
};

MRK_NS_END