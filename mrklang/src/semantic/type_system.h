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
    Type(TypeKind kind) : kind_(kind), name_(getDefaultNameForKind(kind)) {}

    /// Composite constructor
    Type(TypeKind kind, const Str& name) : kind_(kind), name_(name) {}

    TypeKind getKind() const { return kind_; }
    const Str& getName() const { return name_; }

    // Simple compatibility check for now
    bool isCompatibleWith(const Type& other) const {
        // Same types are compatible
        if (kind_ == other.kind_)
            return true;

        // Numeric type compatibility (int/float)
        if (kind_ == TypeKind::FLOAT && other.kind_ == TypeKind::INT)
            return true;

        return false;
    }

    Str toString() const {
        return name_;
    }

private:
    TypeKind kind_;
    Str name_;

    static Str getDefaultNameForKind(TypeKind kind) {
        switch (kind) {
            case TypeKind::VOID: return "void";
            case TypeKind::BOOL: return "bool";
            case TypeKind::INT: return "int";
            case TypeKind::FLOAT: return "float";
            case TypeKind::STRING: return "string";
            case TypeKind::CHAR: return "char";
            case TypeKind::ARRAY: return "array";
            case TypeKind::ENUM: return "enum";
            case TypeKind::STRUCT: return "struct";
            case TypeKind::CLASS: return "class";
            case TypeKind::INTERFACE: return "interface";
            default: return "unknown";
        }
    }
};

MRK_NS_END