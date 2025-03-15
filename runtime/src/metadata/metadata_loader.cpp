#include "metadata_loader.h"

#include <fstream>

MRK_NS_BEGIN_MODULE(runtime::metadata)

MetadataLoader& MetadataLoader::instance() {
	static MetadataLoader instance;
	return instance;
}

MetadataLoader::~MetadataLoader() {
	// Clean up metadata
	if (metadataRoot_) {
		// Free string table
		delete[] metadataRoot_->stringTable.strings;
		delete[] metadataRoot_->stringTable.offsets;

		// Free type definitions and related tables
		delete[] metadataRoot_->typeDefinitions;
		delete[] metadataRoot_->fieldDefinitions;
		delete[] metadataRoot_->methodDefinitions;
		delete[] metadataRoot_->parameterDefinitions;
		delete[] metadataRoot_->assemblyDefinitions;
		delete[] metadataRoot_->imageDefinitions;

		// Free reference tables
		delete[] metadataRoot_->interfaceReferences;
		delete[] metadataRoot_->nestedTypeReferences;
		delete[] metadataRoot_->genericParamReferences;
	}
}

bool MetadataLoader::loadFromFile(const Str& filename) {
	// Open as binary & seek to end to get file size
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	if (!file) {
		return false;
	}

	// Get file size
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	// Read entire file into memory
	Vec<char> buffer(size);
	if (!file.read(buffer.data(), size)) {
		return false;
	}

	// Load from the memory buffer
	return loadFromMemory(buffer.data(), buffer.size());
}

bool MetadataLoader::loadFromMemory(const void* data, size_t size) {
	// Metadata format:
	// [uint32_t version] <-- METADATA VERSION
	// [uint32_t stringsSize] <-- SIZE OF STRING TABLE
	// [uint32_t stringsCount] 
	// [char* strings] 
	// [uint32_t* offsets] <-- OFFSETS INTO STRING TABLE
	// [uint32_t typeDefinitionCount] <-- NUMBER OF TYPE DEFINITIONS
	// [TypeDefinition* typeDefinitions] <-- TYPE DEFINITIONS
	// [uint32_t fieldDefinitionCount] <-- NUMBER OF FIELD DEFINITIONS
	// [FieldDefinition* fieldDefinitions] <-- FIELD DEFINITIONS
	// [uint32_t methodDefinitionCount] <-- NUMBER OF METHOD DEFINITIONS
	// [MethodDefinition* methodDefinitions] <-- METHOD DEFINITIONS
	// [uint32_t parameterDefinitionCount] <-- NUMBER OF PARAMETER DEFINITIONS
	// [ParameterDefinition* parameterDefinitions] <-- PARAMETER DEFINITIONS
	// [uint32_t assemblyDefinitionCount] <-- NUMBER OF ASSEMBLY DEFINITIONS
	// [AssemblyDefinition* assemblyDefinitions] <-- ASSEMBLY DEFINITIONS
	// [uint32_t imageDefinitionCount] <-- NUMBER OF IMAGE DEFINITIONS
	// [ImageDefinition* imageDefinitions] <-- IMAGE DEFINITIONS
	// [uint32_t interfaceReferenceCount] <-- NUMBER OF INTERFACE REFERENCES
	// [TypeDefinitionHandle* interfaceReferences] <-- INTERFACE REFERENCES
	// [uint32_t nestedTypeReferenceCount] <-- NUMBER OF NESTED TYPE REFERENCES
	// [TypeDefinitionHandle* nestedTypeReferences] <-- NESTED TYPE REFERENCES
	// [uint32_t genericParamReferenceCount] <-- NUMBER OF GENERIC PARAMETER REFERENCES
	// [TypeDefinitionHandle* genericParamReferences] <-- GENERIC PARAMETER REFERENCES

	// Validate minimum metadata size
	if (size < sizeof(uint32_t) * 2) { // At minimum, we need a header with version and size
		return false;
	}

	const uint8_t* bytes = static_cast<const uint8_t*>(data);
	uint32_t version = *reinterpret_cast<const uint32_t*>(bytes);

	// Validate version
	// 14/3/2025: As of alpha 0.1, we only support version 1
	if (version != 1) {
		return false;
	}

	// Create a new metadata root
	metadataRoot_ = MakeUnique<MetadataRoot>();

	// Read metadata sections
	const uint8_t* currentPos = bytes + sizeof(uint32_t);

	// Read string table
	uint32_t stringsSize = *reinterpret_cast<const uint32_t*>(currentPos);
	currentPos += sizeof(uint32_t);

	uint32_t stringsCount = *reinterpret_cast<const uint32_t*>(currentPos);
	currentPos += sizeof(uint32_t);

	// Allocate and copy string table
	char* stringData = new char[stringsSize];
	memcpy(stringData, currentPos, stringsSize);
	currentPos += stringsSize;

	uint32_t* stringOffsets = new uint32_t[stringsCount];
	memcpy(stringOffsets, currentPos, stringsCount * sizeof(uint32_t));
	currentPos += stringsCount * sizeof(uint32_t);

	metadataRoot_->stringTable.strings = stringData;
	metadataRoot_->stringTable.offsets = stringOffsets;
	metadataRoot_->stringTable.count = stringsCount;

	// Read type definitions
	metadataRoot_->typeDefinitionCount = *reinterpret_cast<const uint32_t*>(currentPos);
	currentPos += sizeof(uint32_t);

	if (metadataRoot_->typeDefinitionCount > 0) {
		metadataRoot_->typeDefinitions = new TypeDefinition[metadataRoot_->typeDefinitionCount];
		memcpy(metadataRoot_->typeDefinitions, currentPos,
			metadataRoot_->typeDefinitionCount * sizeof(TypeDefinition));
		currentPos += metadataRoot_->typeDefinitionCount * sizeof(TypeDefinition);
	}
	else {
		metadataRoot_->typeDefinitions = nullptr;
	}

	// Read field definitions
	metadataRoot_->fieldDefinitionCount = *reinterpret_cast<const uint32_t*>(currentPos);
	currentPos += sizeof(uint32_t);

	if (metadataRoot_->fieldDefinitionCount > 0) {
		metadataRoot_->fieldDefinitions = new FieldDefinition[metadataRoot_->fieldDefinitionCount];
		memcpy(metadataRoot_->fieldDefinitions, currentPos,
			metadataRoot_->fieldDefinitionCount * sizeof(FieldDefinition));
		currentPos += metadataRoot_->fieldDefinitionCount * sizeof(FieldDefinition);
	}
	else {
		metadataRoot_->fieldDefinitions = nullptr;
	}

	// Read method definitions
	metadataRoot_->methodDefinitionCount = *reinterpret_cast<const uint32_t*>(currentPos);
	currentPos += sizeof(uint32_t);

	if (metadataRoot_->methodDefinitionCount > 0) {
		metadataRoot_->methodDefinitions = new MethodDefinition[metadataRoot_->methodDefinitionCount];
		memcpy(metadataRoot_->methodDefinitions, currentPos,
			metadataRoot_->methodDefinitionCount * sizeof(MethodDefinition));
		currentPos += metadataRoot_->methodDefinitionCount * sizeof(MethodDefinition);
	}
	else {
		metadataRoot_->methodDefinitions = nullptr;
	}

	// Read parameter definitions
	metadataRoot_->parameterDefinitionCount = *reinterpret_cast<const uint32_t*>(currentPos);
	currentPos += sizeof(uint32_t);

	if (metadataRoot_->parameterDefinitionCount > 0) {
		metadataRoot_->parameterDefinitions = new ParameterDefinition[metadataRoot_->parameterDefinitionCount];
		memcpy(metadataRoot_->parameterDefinitions, currentPos,
			metadataRoot_->parameterDefinitionCount * sizeof(ParameterDefinition));
		currentPos += metadataRoot_->parameterDefinitionCount * sizeof(ParameterDefinition);
	}
	else {
		metadataRoot_->parameterDefinitions = nullptr;
	}

	// Read assembly definitions
	metadataRoot_->assemblyDefinitionCount = *reinterpret_cast<const uint32_t*>(currentPos);
	currentPos += sizeof(uint32_t);

	if (metadataRoot_->assemblyDefinitionCount > 0) {
		metadataRoot_->assemblyDefinitions = new AssemblyDefinition[metadataRoot_->assemblyDefinitionCount];
		memcpy(metadataRoot_->assemblyDefinitions, currentPos,
			metadataRoot_->assemblyDefinitionCount * sizeof(AssemblyDefinition));
		currentPos += metadataRoot_->assemblyDefinitionCount * sizeof(AssemblyDefinition);
	}
	else {
		metadataRoot_->assemblyDefinitions = nullptr;
	}

	// Read image definitions
	metadataRoot_->imageDefinitionCount = *reinterpret_cast<const uint32_t*>(currentPos);
	currentPos += sizeof(uint32_t);

	if (metadataRoot_->imageDefinitionCount > 0) {
		metadataRoot_->imageDefinitions = new ImageDefinition[metadataRoot_->imageDefinitionCount];
		memcpy(metadataRoot_->imageDefinitions, currentPos,
			metadataRoot_->imageDefinitionCount * sizeof(ImageDefinition));
		currentPos += metadataRoot_->imageDefinitionCount * sizeof(ImageDefinition);
	}
	else {
		metadataRoot_->imageDefinitions = nullptr;
	}

	// Read interface references
	metadataRoot_->interfaceReferenceCount = *reinterpret_cast<const uint32_t*>(currentPos);
	currentPos += sizeof(uint32_t);

	if (metadataRoot_->interfaceReferenceCount > 0) {
		metadataRoot_->interfaceReferences = new TypeDefinitionHandle[metadataRoot_->interfaceReferenceCount];
		memcpy(metadataRoot_->interfaceReferences, currentPos,
			metadataRoot_->interfaceReferenceCount * sizeof(TypeDefinitionHandle));
		currentPos += metadataRoot_->interfaceReferenceCount * sizeof(TypeDefinitionHandle);
	}
	else {
		metadataRoot_->interfaceReferences = nullptr;
	}

	// Read nested type references
	metadataRoot_->nestedTypeReferenceCount = *reinterpret_cast<const uint32_t*>(currentPos);
	currentPos += sizeof(uint32_t);

	if (metadataRoot_->nestedTypeReferenceCount > 0) {
		metadataRoot_->nestedTypeReferences = new TypeDefinitionHandle[metadataRoot_->nestedTypeReferenceCount];
		memcpy(metadataRoot_->nestedTypeReferences, currentPos,
			metadataRoot_->nestedTypeReferenceCount * sizeof(TypeDefinitionHandle));
		currentPos += metadataRoot_->nestedTypeReferenceCount * sizeof(TypeDefinitionHandle);
	}
	else {
		metadataRoot_->nestedTypeReferences = nullptr;
	}

	// Read generic parameter references
	metadataRoot_->genericParamReferenceCount = *reinterpret_cast<const uint32_t*>(currentPos);
	currentPos += sizeof(uint32_t);

	if (metadataRoot_->genericParamReferenceCount > 0) {
		metadataRoot_->genericParamReferences = new TypeDefinitionHandle[metadataRoot_->genericParamReferenceCount];
		memcpy(metadataRoot_->genericParamReferences, currentPos,
			metadataRoot_->genericParamReferenceCount * sizeof(TypeDefinitionHandle));
		currentPos += metadataRoot_->genericParamReferenceCount * sizeof(TypeDefinitionHandle);
	}
	else {
		metadataRoot_->genericParamReferences = nullptr;
	}

	// Build type lookup for faster access
	buildTypeLookup();

	return true;
}

const char* MetadataLoader::getString(StringHandle handle) const {
	return metadataRoot_ ? metadataRoot_->stringTable.getString(handle) : nullptr;
}


const AssemblyDefinition* MetadataLoader::findAssemblyByName(const Str& assemblyName) const {
	if (!metadataRoot_ || !metadataRoot_->assemblyDefinitions) {
		return nullptr;
	}

	for (uint32_t i = 0; i < metadataRoot_->assemblyDefinitionCount; i++) {
		const AssemblyDefinition* assemblyDef = &metadataRoot_->assemblyDefinitions[i];
		if (getString(assemblyDef->name) == assemblyName) {
			return assemblyDef;
		}
	}

	return nullptr;
}

const ImageDefinition* MetadataLoader::findImageByName(const Str& imageName) const {
	if (!metadataRoot_ || !metadataRoot_->imageDefinitions) {
		return nullptr;
	}

	for (uint32_t i = 0; i < metadataRoot_->imageDefinitionCount; i++) {
		const ImageDefinition* imageDef = &metadataRoot_->imageDefinitions[i];
		if (getString(imageDef->name) == imageName) {
			return imageDef;
		}
	}

	return nullptr;
}

const TypeDefinition* MetadataLoader::findTypeDefinition(const Str& fullName) const {
	if (!metadataRoot_) {
		return nullptr;
	}

	auto it = typeLookupMap_.find(fullName);
	if (it != typeLookupMap_.end()) {
		return &metadataRoot_->typeDefinitions[it->second];
	}

	return nullptr;
}

const MethodDefinition* MetadataLoader::findMethodDefinition(const TypeDefinition& typeDef, const Str& methodName) const {
	if (!metadataRoot_ || !metadataRoot_->methodDefinitions) {
		return nullptr;
	}

	// Loop through methods of this type
	for (uint32_t i = 0; i < typeDef.methodCount; i++) {
		uint32_t methodIndex = typeDef.methodStart + i;
		if (methodIndex >= metadataRoot_->methodDefinitionCount) {
			continue; // Out of bounds
		}

		const MethodDefinition* methodDef = &metadataRoot_->methodDefinitions[methodIndex];
		if (getString(methodDef->name) == methodName) {
			return methodDef;
		}
	}

	return nullptr;
}

const FieldDefinition* MetadataLoader::findFieldDefinition(const TypeDefinition& typeDef, const Str& fieldName) const {
	if (!metadataRoot_ || !metadataRoot_->fieldDefinitions) {
		return nullptr;
	}

	// Loop through fields of this type
	for (uint32_t i = 0; i < typeDef.fieldCount; i++) {
		uint32_t fieldIndex = typeDef.fieldStart + i;
		if (fieldIndex >= metadataRoot_->fieldDefinitionCount) {
			continue; // Out of bounds
		}

		const FieldDefinition* fieldDef = &metadataRoot_->fieldDefinitions[fieldIndex];
		if (getString(fieldDef->name) == fieldName) {
			return fieldDef;
		}
	}

	return nullptr;
}

const TypeDefinition* MetadataLoader::getParentType(const TypeDefinition& typeDef) const {
	if (!metadataRoot_ || typeDef.parentHandle == 0) {
		return nullptr;
	}

	// Type handle is 1-based, so subtract 1 for zero-based array
	uint32_t parentIndex = typeDef.parentHandle - 1;
	if (parentIndex >= metadataRoot_->typeDefinitionCount) {
		return nullptr;
	}

	return &metadataRoot_->typeDefinitions[parentIndex];
}

Vec<const TypeDefinition*> MetadataLoader::getInterfaces(const TypeDefinition& typeDef) const {
	Vec<const TypeDefinition*> interfaces;

	if (!metadataRoot_ || typeDef.interfaceCount == 0) {
		return interfaces;
	}

	for (uint32_t i = 0; i < typeDef.interfaceCount; i++) {
		uint32_t referenceIndex = typeDef.interfaceStart + i;
		if (referenceIndex >= metadataRoot_->interfaceReferenceCount) {
			continue; // Out of bounds
		}

		TypeDefinitionHandle interfaceHandle = metadataRoot_->interfaceReferences[referenceIndex];
		uint32_t interfaceIndex = interfaceHandle - 1;
		if (interfaceIndex >= metadataRoot_->typeDefinitionCount) {
			continue;
		}

		interfaces.push_back(&metadataRoot_->typeDefinitions[interfaceIndex]);
	}

	return interfaces;
}

void MetadataLoader::buildTypeLookup() {
	if (!metadataRoot_ || !metadataRoot_->typeDefinitions) {
		return;
	}

	typeLookupMap_.clear();

	// Build full name for each type and store in lookup map
	for (uint32_t i = 0; i < metadataRoot_->typeDefinitionCount; i++) {
		const TypeDefinition& typeDef = metadataRoot_->typeDefinitions[i];
		Str namespaceName = getString(typeDef.namespaceName);
		Str name = getString(typeDef.name);
		Str fullName = namespaceName.empty() ? name : namespaceName + "::" + name;

		typeLookupMap_[fullName] = i;
	}
}

MRK_NS_END