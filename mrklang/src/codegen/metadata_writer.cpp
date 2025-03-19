#include "metadata_writer.h"
#include "common/logging.h"
#include "mrk-metadata.h"

#include <algorithm>
#include <set>

#define METADATA_VERSION 1
#define METADATA_MAGIC 0x4D524B4D // MRKM
#define CAST(value) reinterpret_cast<const char*>(&value)
#define CAST_PTR(value) reinterpret_cast<const char*>(value)

MRK_NS_BEGIN_MODULE(codegen)

using namespace runtime::metadata;

MetadataWriter::MetadataWriter(const SymbolTable* symbolTable) : symbolTable_(symbolTable) {}

UniquePtr<CompilerMetadataRegistration> MetadataWriter::writeMetadataFile(const Str& path) {
	file_.open(path, std::ios::out | std::ios::trunc | std::ios::binary);

	if (!file_.is_open()) {
		MRK_ERROR("Failed to open metadata file: {}", path);
		return nullptr;
	}

	registration_ = MakeUnique<CompilerMetadataRegistration, false>();

	generateMetadataHeader();
	generateStringTable();
	generateTypeDefintions();
	generateFieldDefinitions();
	generateMethodDefinitions();
	generateParameterDefinitions();
	generateAssemblyDefinition();
	generateImageDefinition();
	generateReferenceTables();

	file_.close();

	MRK_INFO("Metadata written to {}", path);

	// Good time having you around, registration
	return Move(registration_);
}

void MetadataWriter::generateMetadataHeader() {
	// 15/3/2025: Version 1
	const uint32_t version = METADATA_VERSION;
	file_.write(CAST(version), sizeof(uint32_t));

	// Magic
	const uint32_t magic = METADATA_MAGIC;
	file_.write(CAST(magic), sizeof(uint32_t));
}

void MetadataWriter::generateStringTable() {
	// Collect all strings
	std::set<Str> set;

	// Assembly name
	set.insert("mrklang_runtime");

	// Types and namespaces
	for (const auto& type : symbolTable_->getTypes()) {
		auto pos = type->qualifiedName.find_last_of("::");
		if (pos != Str::npos && pos > 1) {
			// Extract namespace
			set.insert(type->qualifiedName.substr(0, pos - 1));
		}

		set.insert(type->name);
		set.insert(type->qualifiedName);

		// Add member names
		for (const auto& [name, _] : type->members) {
			set.insert(name);
		}
	}

	// Add function names
	for (const auto* func : symbolTable_->getFunctions()) {
		set.insert(func->name);

		// Add parameter names
		for (const auto& [name, _] : func->parameters) {
			set.insert(name);
		}
	}

	// Add variable names
	for (const auto* var : symbolTable_->getVariables()) {
		set.insert(var->name);
		set.insert(var->qualifiedName);
	}

	// Convert set to vector for indexing
	Vec<Str> strings(set.begin(), set.end());

	stringHandleMap_.clear();
	for (size_t i = 0; i < strings.size(); i++) {
		stringHandleMap_[strings[i]] = static_cast<uint32_t>(i);
	}

	// Calc string table size
	uint32_t stringTableSize = 0;
	for (const auto& str : strings) {
		stringTableSize += str.size() + 1; // Include null terminator
	}

	// Write string table size
	file_.write(CAST(stringTableSize), sizeof(uint32_t));

	// Write count
	const uint32_t stringCount = strings.size();
	file_.write(CAST(stringCount), sizeof(uint32_t));

	// Write string data
	uint32_t currentOffset = 0;
	Vec<uint32_t> offsets(stringCount);

	for (size_t i = 0; i < strings.size(); i++) {
		const Str& str = strings[i];
		offsets[i] = currentOffset;

		// Write the string with null terminator and update offset
		file_.write(str.c_str(), str.size() + 1);
		currentOffset += str.size() + 1;
	}

	// Write all offsets
	file_.write(CAST_PTR(offsets.data()), stringCount * sizeof(uint32_t));
}

void MetadataWriter::generateTypeDefintions() {
	const auto& types = symbolTable_->getTypes();

	// Write type count
	uint32_t typeCount = static_cast<uint32_t>(types.size());
	file_.write(CAST(typeCount), sizeof(uint32_t));

	// First pass: Calculate field and method start positions for each type
	Vec<uint32_t> fieldStarts(typeCount, 0);
	Vec<uint32_t> methodStarts(typeCount, 0);
	Vec<uint32_t> interfaceStarts(typeCount, 0);

	uint32_t fieldIndex = 0;
	uint32_t methodIndex = 0;
	uint32_t interfaceIndex = 0;

	for (size_t i = 0; i < typeCount; i++) {
		const auto* type = types[i];

		// Record current indices
		fieldStarts[i] = fieldIndex;
		methodStarts[i] = methodIndex;
		interfaceStarts[i] = interfaceIndex;

		// Count members to update indices for the next type
		for (const auto& [_, member] : type->members) {
			if (member->kind == SymbolKind::VARIABLE) {
				fieldIndex++;
			}
			else if (member->kind == SymbolKind::FUNCTION) {
				methodIndex++;
			}
		}

		// Count interfaces (excluding first base type which is the parent)
		if (type->resolver.baseTypes.size() > 1) {
			interfaceIndex += static_cast<uint32_t>(type->resolver.baseTypes.size() - 1);
		}
	}

	// Second pass: Generate type definitions
	for (size_t i = 0; i < typeCount; i++) {
		const auto* type = types[i];

		// Create TypeDefinition
		TypeDefinition typeDef{};

		// Set name handle
		typeDef.name = stringHandleMap_[type->name];

		// Set namespace handle
		auto pos = type->qualifiedName.find_last_of("::");
		if (pos != Str::npos && pos > 1) {
			Str namespaceName = type->qualifiedName.substr(0, pos - 1);
			typeDef.namespaceName = stringHandleMap_[namespaceName];
		}
		else {
			typeDef.namespaceName = 0; // No namespace
		}

		// Set parent handle (if any)
		if (!type->resolver.baseTypes.empty()) {
			// Use first base type as parent
			const TypeSymbol* baseType = type->resolver.baseTypes[0];
			typeDef.parentHandle = static_cast<uint32_t>(
				std::distance(types.begin(),
					std::find(types.begin(), types.end(), baseType)) + 1
				);
		}
		else {
			typeDef.parentHandle = 0; // No parent
		}

		// Count fields and methods
		uint32_t fieldCount = 0;
		uint32_t methodCount = 0;

		for (const auto& [_, member] : type->members) {
			if (member->kind == SymbolKind::VARIABLE) {
				fieldCount++;
			}
			else if (member->kind == SymbolKind::FUNCTION) {
				methodCount++;
			}
		}

		// Set field and method info using the start indices we calculated
		typeDef.fieldStart = fieldStarts[i];
		typeDef.fieldCount = fieldCount;
		typeDef.methodStart = methodStarts[i];
		typeDef.methodCount = methodCount;

		// Set interface info
		typeDef.interfaceStart = interfaceStarts[i];
		typeDef.interfaceCount = type->resolver.baseTypes.size() > 1 ?
			static_cast<uint32_t>(type->resolver.baseTypes.size() - 1) : 0;

		// Other type properties
		TypeFlags flags = static_cast<TypeFlags>(0);
		flags.isClass = detail::hasFlag(type->kind, SymbolKind::CLASS);
		flags.isStruct = detail::hasFlag(type->kind, SymbolKind::STRUCT);
		flags.isInterface = detail::hasFlag(type->kind, SymbolKind::INTERFACE);
		flags.isEnum = detail::hasFlag(type->kind, SymbolKind::ENUM);
		flags.isPrimitive = detail::hasFlag(type->kind, SymbolKind::PRIMITIVE_TYPE);

		typeDef.flags = flags;

		// Use index+1 as token (1-based)
		typeDef.token = static_cast<uint32_t>(i + 1);

		// Write type definition
		file_.write(CAST(typeDef), sizeof(TypeDefinition));

		// Register type definition
		registration_->typeTokenMap[type] = typeDef.token;
	}
}

void MetadataWriter::generateFieldDefinitions() {
	const auto& types = symbolTable_->getTypes();

	uint32_t totalFields = 0;
	for (const auto& type : types) {
		for (const auto& [_, member] : type->members) {
			if (member->kind == SymbolKind::VARIABLE) {
				totalFields++;
			}
		}
	}

	// Write field count
	file_.write(CAST(totalFields), sizeof(uint32_t));

	// Types again..
	uint32_t fieldIndex = 0;
	for (const auto& type : types) {
		for (const auto& [_, member] : type->members) {
			if (member->kind == SymbolKind::VARIABLE) {
				const auto* field = static_cast<const VariableSymbol*>(member.get());

				// Create FieldDefinition
				FieldDefinition fieldDef{};

				// Set name handle
				fieldDef.name = stringHandleMap_[field->name];

				// Set type handle
				fieldDef.typeHandle = static_cast<uint32_t>(
					std::distance(symbolTable_->getTypes().begin(),
						std::find(symbolTable_->getTypes().begin(), symbolTable_->getTypes().end(), field->resolver.type)) + 1
					);

				// Flags
				fieldDef.flags = static_cast<uint32_t>(field->accessModifier);
				fieldDef.token = ++fieldIndex;

				// Write field definition
				file_.write(CAST(fieldDef), sizeof(FieldDefinition));

				// Register field definition
				registration_->fieldTokenMap[field] = fieldDef.token;
			}
		}
	}
}

void MetadataWriter::generateMethodDefinitions() {
	const auto& types = symbolTable_->getTypes();

	uint32_t totalMethods = 0;
	for (const auto& type : types) {
		for (const auto& [_, member] : type->members) {
			if (member->kind == SymbolKind::FUNCTION) {
				totalMethods++;
			}
		}
	}

	file_.write(CAST(totalMethods), sizeof(uint32_t));

	// Track parameter index for mapping methods to their parameters
	uint32_t parameterStartIndex = 0;

	uint32_t methodIndex = 0;
	for (const auto& type : types) {
		for (const auto& [_, member] : type->members) {
			if (member->kind == SymbolKind::FUNCTION) {
				const auto* func = static_cast<const FunctionSymbol*>(member.get());

				// Generate MethodDefinition
				MethodDefinition methodDef{};

				// Set name handle
				methodDef.name = stringHandleMap_[func->name];

				// Find return type handle (1-based index in the type table)
				methodDef.returnTypeHandle = static_cast<uint32_t>(
					std::distance(symbolTable_->getTypes().begin(),
						std::find(symbolTable_->getTypes().begin(), symbolTable_->getTypes().end(), func->resolver.returnType)) + 1
					);

				// Set parameter info
				methodDef.parameterStart = parameterStartIndex;
				methodDef.parameterCount = static_cast<uint32_t>(func->parameters.size());

				// Update parameter start index for next method
				parameterStartIndex += methodDef.parameterCount;

				// Set flags based on access modifiers
				methodDef.flags = static_cast<uint32_t>(func->accessModifier);
				methodDef.implFlags = 0; // Default implementation flags for now

				// Set token based on function index (1-based)
				methodDef.token = ++methodIndex;

				// Write method definition
				file_.write(CAST(methodDef), sizeof(MethodDefinition));

				// Register method definition
				registration_->methodTokenMap[func] = methodDef.token;
			}
		}
	}
}

void MetadataWriter::generateParameterDefinitions() {
	// Count total parameters across all functions
	uint32_t totalParams = 0;
	for (const auto* func : symbolTable_->getFunctions()) {
		totalParams += static_cast<uint32_t>(func->parameters.size());
	}

	// Write parameter count
	file_.write(CAST(totalParams), sizeof(uint32_t));

	// Start from types again
	const auto& types = symbolTable_->getTypes();
	for (const auto& type : types) {
		for (const auto& [_, member] : type->members) {
			if (member->kind == SymbolKind::FUNCTION) {
				const auto* func = static_cast<const FunctionSymbol*>(member.get());
				for (const auto& [paramName, param] : func->parameters) {
					// Create ParameterDefinition
					ParameterDefinition paramDef{};

					// Set name handle
					paramDef.name = stringHandleMap_[paramName];

					// Find type handle (1-based index in the type table)
					paramDef.typeHandle = static_cast<uint32_t>(
						std::distance(symbolTable_->getTypes().begin(),
							std::find(symbolTable_->getTypes().begin(), symbolTable_->getTypes().end(), param->resolver.type)) + 1
						);

					// Set flags
					paramDef.flags = 0; // TODO: impl params, etc

					// Write parameter definition
					file_.write(CAST(paramDef), sizeof(ParameterDefinition));
				}
			}
		}
	}
}

void MetadataWriter::generateAssemblyDefinition() {
	// Create a single assembly definition for the current compilation
	const uint32_t assemblyCount = 1;
	file_.write(CAST(assemblyCount), sizeof(uint32_t));

	AssemblyDefinition assemblyDef{};

	assemblyDef.name = stringHandleMap_["mrklang_runtime"];
	assemblyDef.majorVersion = 1;
	assemblyDef.minorVersion = 0;
	assemblyDef.buildNumber = 0;
	assemblyDef.revisionNumber = 0;
	assemblyDef.imageIndex = 0;
	assemblyDef.flags = 0;

	file_.write(CAST(assemblyDef), sizeof(AssemblyDefinition));
}

void MetadataWriter::generateImageDefinition() {
	// Create a single image definition
	const uint32_t imageCount = 1;
	file_.write(CAST(imageCount), sizeof(uint32_t));

	ImageDefinition imageDef{};

	imageDef.name = stringHandleMap_["mrklang_runtime"];

	// Set type information
	imageDef.typeStart = 0;
	imageDef.typeCount = static_cast<uint32_t>(symbolTable_->getTypes().size());

	// Find entry point token if available
	auto globalFunction = symbolTable_->getGlobalFunction();
	if (globalFunction) {
		imageDef.entryPointToken = registration_->methodTokenMap.at(globalFunction);
	}
	else {
		imageDef.entryPointToken = 0;
	}

	file_.write(CAST(imageDef), sizeof(ImageDefinition));
}

void MetadataWriter::generateReferenceTables() {
	generateInterfaceReferences();
	generateNestedTypeReferences();
	generateGenericParamReferences();
}

void MetadataWriter::generateInterfaceReferences() {
	// Count total interface references
	uint32_t totalInterfaces = 0;
	for (const auto* type : symbolTable_->getTypes()) {
		// First base type is considered the parent, rest are interfaces
		if (type->resolver.baseTypes.size() > 1) {
			totalInterfaces += static_cast<uint32_t>(type->resolver.baseTypes.size() - 1);
		}
	}

	// Write interface reference count
	file_.write(CAST(totalInterfaces), sizeof(uint32_t));

	// If no interfaces, we're done
	if (totalInterfaces == 0) return;

	// Generate interface references
	for (const auto* type : symbolTable_->getTypes()) {
		if (type->resolver.baseTypes.size() <= 1) continue;

		// Skip first base type (parent class)
		for (size_t i = 1; i < type->resolver.baseTypes.size(); i++) {
			const TypeSymbol* interfaceType = type->resolver.baseTypes[i];

			// Find the interface's index in the type table (1-based)
			TypeDefinitionHandle interfaceHandle = static_cast<uint32_t>(
				std::distance(symbolTable_->getTypes().begin(),
					std::find(symbolTable_->getTypes().begin(), symbolTable_->getTypes().end(), interfaceType)) + 1
				);

			// Write interface handle
			file_.write(CAST(interfaceHandle), sizeof(TypeDefinitionHandle));
		}
	}
}

void MetadataWriter::generateNestedTypeReferences() {
	// Empty for now
	const uint32_t nestedTypeCount = 0;
	file_.write(CAST(nestedTypeCount), sizeof(uint32_t));
}

void MetadataWriter::generateGenericParamReferences() {
	// Empty for now
	const uint32_t genericParamCount = 0;
	file_.write(CAST(genericParamCount), sizeof(uint32_t));
}

MRK_NS_END
