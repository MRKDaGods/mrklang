#pragma once

#include "common/types.h"

MRK_NS_BEGIN

enum class TypeKind {
	// Primitive types
	VOID,
	BOOL,
	INT,
	FLOAT,
	STRING,
	CHAR,

	// Composite types
	ARRAY,
	ENUM,
	STRUCT,
	CLASS,
	INTERFACE
};

class Type {
public:
	/// Primitive constructor
	Type(TypeKind kind) : kind_(kind) {}

	/// Composite constructor
	Type(TypeKind kind, const Str& name) : kind_(kind), name_(name) {}
	TypeKind getKind() const { return kind_; }
	bool isCompatibleWith(const Type& other) const;
	Str toString() const;

private:
	TypeKind kind_;
	Str name_;
};

MRK_NS_END