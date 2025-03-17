#pragma once

#include "common/types.h"

#include <cstdint>

MRK_NS_BEGIN_MODULE(runtime::metadata)

// Handles
using StringHandle = uint32_t;
using TypeDefinitionHandle = uint32_t;

struct StringTable {
	const char* strings;
	uint32_t* offsets;
	uint32_t count;

	const char* getString(StringHandle handle) const {
		if (handle >= count) {
			return nullptr;
		}

		return strings + offsets[handle];
	}
};

struct MemberFlags {
	#define ACCESS_MODIFIERS \
	X(PUBLIC, 0) \
	X(PROTECTED, 1) \
	X(PRIVATE, 2) \
	X(INTERNAL, 3) \
	X(STATIC, 4) \
	X(ABSTRACT, 5) \
	X(SEALED, 6) \
	X(VIRTUAL, 7) \
	X(OVERRIDE, 8) \
	X(CONST, 9) \
	X(READONLY, 10) \
	X(EXTERN, 11) \
	X(IMPLICIT, 12) \
	X(EXPLICIT, 13) \
	X(NEW, 14) \
	X(ASYNC, 15)

	#define X(x, y) uint32_t x : 1;
	ACCESS_MODIFIERS
	#undef X
};

struct FieldDefinition {
	StringHandle name;
	TypeDefinitionHandle typeHandle;
	MemberFlags flags;
	uint32_t token;
};

struct ParameterDefinition {
	StringHandle name;
	TypeDefinitionHandle typeHandle;
	uint32_t flags;
};

struct MethodDefinition {
	StringHandle name;
	TypeDefinitionHandle returnTypeHandle;
	uint32_t parameterStart; // Index into the parameter table
	uint32_t parameterCount;
	MemberFlags flags; // Method flags
	uint32_t implFlags; // Method implementation flags
	uint32_t token; // Method token
};

struct TypeFlags {
	uint32_t isPrimitive : 1;     // Is a fundamental type
	uint32_t isValueType : 1;     // Value vs reference type
	uint32_t isAbstract : 1;      // Cannot be instantiated
	uint32_t isSealed : 1;        // Cannot be inherited from
	uint32_t isGeneric : 1;       // Is a generic type
	uint32_t isEnum : 1;          // Is an enum
	uint32_t isInterface : 1;     // Is an interface
	uint32_t isClass : 1;         // Is a class
	uint32_t isStruct : 1;        // Is a struct
	uint32_t isNested : 1;        // Is a nested type
	uint32_t visibility : 3;      // Visibility flags
	uint32_t layout : 2;          // Layout flags
};

struct TypeDefinition {
	StringHandle name;
	StringHandle namespaceName;
	TypeDefinitionHandle parentHandle;
	uint32_t interfaceStart;
	uint32_t interfaceCount;
	uint32_t fieldStart;
	uint32_t fieldCount;
	uint32_t methodStart;
	uint32_t methodCount;
	uint32_t nestedTypeStart;
	uint32_t nestedTypeCount;
	uint32_t genericParamStart;
	uint32_t genericParamCount;
	TypeFlags flags;
	uint32_t size;
	uint32_t token;
};

// Follow a similar style to good old .NET metadata
struct AssemblyDefinition {
	StringHandle name;
	uint16_t majorVersion;
	uint16_t minorVersion;
	uint16_t buildNumber;
	uint16_t revisionNumber;
	uint32_t imageIndex;
	uint32_t flags;

	// Implement public key sometime later :P
	// will I ever need it?
	// I couldnt care about strong naming
	// Versioning is enough
	// If you ever see this mohamed, text yourself hi in the future :P
	// I wanna work on Windows at Microsoft tbh
};

struct ImageDefinition {
	StringHandle name;
	uint32_t typeStart;
	uint32_t typeCount;
	uint32_t entryPointToken;
};

struct MetadataRoot {
	StringTable stringTable;

	TypeDefinition* typeDefinitions;
	uint32_t typeDefinitionCount;

	FieldDefinition* fieldDefinitions;
	uint32_t fieldDefinitionCount;

	MethodDefinition* methodDefinitions;
	uint32_t methodDefinitionCount;

	ParameterDefinition* parameterDefinitions;
	uint32_t parameterDefinitionCount;

	AssemblyDefinition* assemblyDefinitions;
	uint32_t assemblyDefinitionCount;

	ImageDefinition* imageDefinitions;
	uint32_t imageDefinitionCount;

	// Reference tables
	TypeDefinitionHandle* interfaceReferences;
	uint32_t interfaceReferenceCount;

	TypeDefinitionHandle* nestedTypeReferences;
	uint32_t nestedTypeReferenceCount;

	TypeDefinitionHandle* genericParamReferences;
	uint32_t genericParamReferenceCount;
};

template<typename TypeT, typename FieldT, typename MethodT>
struct MetadataRegistrationBase {
	Dict<TypeT*, uint32_t> typeTokenMap;
	Dict<FieldT*, uint32_t> fieldTokenMap;
	Dict<MethodT*, uint32_t> methodTokenMap;

	MetadataRegistrationBase() = default;
};

template<typename TypeT, typename FieldT, typename MethodT, bool ReverseLookup = false>
struct MetadataRegistration : MetadataRegistrationBase<TypeT, FieldT, MethodT> {};

template<typename TypeT, typename FieldT, typename MethodT>
struct MetadataRegistration<TypeT, FieldT, MethodT, true> : MetadataRegistrationBase<TypeT, FieldT, MethodT> {
	Dict<uint32_t, TypeT*> typeTokenReverseMap;
	Dict<uint32_t, FieldT*> fieldTokenReverseMap;
	Dict<uint32_t, MethodT*> methodTokenReverseMap;
};


MRK_NS_END