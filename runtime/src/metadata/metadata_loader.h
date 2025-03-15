#pragma once

#include "common/types.h"
#include "metadata_structures.h"

MRK_NS_BEGIN_MODULE(runtime::metadata)

class MetadataLoader {
public:
	static MetadataLoader& instance();

	MetadataLoader(const MetadataLoader&) = delete;
	MetadataLoader& operator=(const MetadataLoader&) = delete;

	bool loadFromFile(const Str& filename);
	bool loadFromMemory(const void* data, size_t size);

	const MetadataRoot* getMetadataRoot() const { return metadataRoot_.get(); }
	const char* getString(StringHandle handle) const;

	// Find types, etc
	const AssemblyDefinition* findAssemblyByName(const Str& assemblyName) const;
	const ImageDefinition* findImageByName(const Str& imageName) const;
	const TypeDefinition* findTypeDefinition(const Str& fullName) const;
	const MethodDefinition* findMethodDefinition(const TypeDefinition& typeDef, const Str& methodName) const;
	const FieldDefinition* findFieldDefinition(const TypeDefinition& typeDef, const Str& fieldName) const;
	const TypeDefinition* getParentType(const TypeDefinition& typeDef) const;
	Vec<const TypeDefinition*> getInterfaces(const TypeDefinition& typeDef) const;

private:
	UniquePtr<MetadataRoot> metadataRoot_;
	Dict<Str, uint32_t> typeLookupMap_;

	MetadataLoader() = default;
	~MetadataLoader();

	/// Builds an internal lookup table for types
	void buildTypeLookup();
};

MRK_NS_END