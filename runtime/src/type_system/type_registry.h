#pragma once

#include "common/types.h"
#include "metadata/metadata_structures.h"

MRK_NS_BEGIN_MODULE(runtime::type_system)

using namespace runtime::metadata;

class Type;
class Field;
class Method;

using RuntimeMetadataRegistration = MetadataRegistration<Type, Field, Method, true>;

// Registry for all types in the system
class TypeRegistry {
public:
    static TypeRegistry& instance();

    TypeRegistry(const TypeRegistry&) = delete;
    TypeRegistry& operator=(const TypeRegistry&) = delete;

    void registerType(Type* type);
	Type* createArrayType(Type* elementType, uint32_t rank);
    Type* findType(const Str& fullName) const;

	void initializeMetadata(const MetadataRoot* root);

	// Metadata methods
    Type* registerTypeFromMetadata(const TypeDefinition& typeDef);
    Field* registerFieldFromMetadata(const TypeDefinition& typeDef,
        const FieldDefinition& fieldDef);
    Method* registerMethodFromMetadata(const TypeDefinition& typeDef,
        const MethodDefinition& methodDef);

    void initializeBuiltinTypes();
	void dumpTree() const;
    void dumpType(Type* type, int indent) const;

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

    // External
    Type* getTypeByToken(uint32_t token) const;
	Field* getFieldByToken(uint32_t token) const;
	Method* getMethodByToken(uint32_t token) const;

    // API
    Field* getField(FieldDefinition* fieldDef);

private:
    Dict<Str, Type*> types_;
	UniquePtr<RuntimeMetadataRegistration> registration_;

    // Cached built-in types
    Type* voidType_ = nullptr;
    Type* boolType_ = nullptr;
    Type* charType_ = nullptr;
    Type* byteType_ = nullptr;
    Type* int16Type_ = nullptr;
	Type* uint16Type_ = nullptr;
    Type* int32Type_ = nullptr;
	Type* uint32Type_ = nullptr;
    Type* int64Type_ = nullptr;
	Type* uint64Type_ = nullptr;
    Type* floatType_ = nullptr;
    Type* doubleType_ = nullptr;
    Type* stringType_ = nullptr;
    Type* objectType_ = nullptr;

    TypeRegistry() = default;
    ~TypeRegistry();

    void regsterPrimitiveType(Type* type);
};

MRK_NS_END