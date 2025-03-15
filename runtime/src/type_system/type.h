#pragma once

#include "common/types.h"

MRK_NS_BEGIN_MODULE(runtime::type_system)

#define MRK_STL_NAME "mrkstl"

enum class TypeKind {
    VOID,
    BOOL,
    CHAR,
    I8,
    U8,
    I16,
    U16,
    I32,
    U32,
    I64,
    U64,
    F32,
    F64,
    STRING,
    PTR,
    BYREF,
    VALUE_TYPE,
    CLASS,
	SZ_ARRAY, // Single-dimensional, zero-based array
	ARRAY, // Multi-dimensional, zero-based array
    TYPE_PARAMETER,
    METHOD_TYPE_PARAMETER
};

// Type attributes based on .NET
enum class TypeAttributes {
    // Visibility attributes
    NOT_PUBLIC = 0x00000000,
    PUBLIC = 0x00000001,
    NESTED_PUBLIC = 0x00000002,
    NESTED_PRIVATE = 0x00000003,
    NESTED_FAMILY = 0x00000004,
    NESTED_ASSEMBLY = 0x00000005,
    NESTED_FAM_AND_ASSEM = 0x00000006,
    NESTED_FAM_OR_ASSEM = 0x00000007,
    VISIBILITY_MASK = 0x00000007,

    // Class layout attributes
    AUTO_LAYOUT = 0x00000000,
    SEQUENTIAL_LAYOUT = 0x00000008,
    EXPLICIT_LAYOUT = 0x00000010,
    LAYOUT_MASK = 0x00000018,

    // Class semantics attributes
    CLASS = 0x00000000,
    INTERFACE = 0x00000020,
    ABSTRACT = 0x00000080,
    SEALED = 0x00000100,
    SPECIAL_NAME = 0x00000400,

    // Implementation attributes
    IMPORT = 0x00001000,
    SERIALIZABLE = 0x00002000,
    BEFORE_FIELD_INIT = 0x00100000,
};

// Base class for all types
class Type {
public:
    virtual ~Type() = default;

    virtual Str getName() const = 0;
    virtual Str getFullName() const = 0;
    virtual TypeKind getTypeKind() const = 0;
    virtual size_t getSize() const = 0;
    virtual bool isValueType() const = 0;
    virtual bool isClass() const = 0;
    virtual bool isPrimitive() const = 0;
    virtual bool isArray() const = 0;

    // Factory methods
    /*static SharedPtr<Type> GetType(const Str& fullName);
    static SharedPtr<Type> GetTypeFromHandle(uintptr_t handle);*/

    // Link to metadata
    uint32_t getToken() const { return token_; }
    void setToken(uint32_t token) { token_ = token; }

protected:
    uint32_t token_ = 0;
};

MRK_NS_END