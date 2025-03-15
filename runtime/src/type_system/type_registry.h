#pragma once

#include "common/types.h"

MRK_NS_BEGIN_MODULE(runtime::type_system)

class Type;

// Registry for all types in the system
class TypeRegistry {
public:
    static TypeRegistry& instance();

    TypeRegistry(const TypeRegistry&) = delete;
    TypeRegistry& operator=(const TypeRegistry&) = delete;

    void registerType(Type* type);
	Type* createArrayType(Type* elementType, uint32_t rank);
    Type* findType(const Str& fullName) const;

    void initializeBuiltinTypes();

    // Common primitive types
    Type* getVoidType() { return voidType_; }
    Type* getBoolType() { return boolType_; }
    Type* getCharType() { return charType_; }
    Type* getByteType() { return byteType_; }
    Type* getInt16Type() { return int16Type_; }
    Type* getInt32Type() { return int32Type_; }
    Type* getInt64Type() { return int64Type_; }
    Type* getFloatType() { return floatType_; }
    Type* getDoubleType() { return doubleType_; }
    Type* getStringType() { return stringType_; }
    Type* getObjectType() { return objectType_; }

private:
    Dict<Str, Type*> types_;

    // Cached built-in types
    Type* voidType_;
    Type* boolType_;
    Type* charType_;
    Type* byteType_;
    Type* int16Type_;
    Type* int32Type_;
    Type* int64Type_;
    Type* floatType_;
    Type* doubleType_;
    Type* stringType_;
    Type* objectType_;

    TypeRegistry() = default;
    ~TypeRegistry();
};

MRK_NS_END