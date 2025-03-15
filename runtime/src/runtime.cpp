#include "runtime.h"
#include "type_system/primitive_type.h"
#include "type_system/class_type.h"
#include "type_system/array_type.h"
#include "type_system/field.h"
#include "type_system/method.h"
#include <iostream>

MRK_NS_BEGIN_MODULE(runtime)

Runtime& Runtime::instance() {
	static Runtime instance;
	return instance;
}

bool Runtime::initialize(const RuntimeOptions& options) {
	if (initialized_) {
		return true; // Already initialized
	}

	options_ = options;

	// Initialize type registry
	type_system::TypeRegistry::instance().initializeBuiltinTypes();

	// Load metadata if path provided
	if (!options.metadataPath.empty()) {
		if (!metadata::MetadataLoader::instance().loadFromFile(options.metadataPath)) {
			std::cerr << "Failed to load metadata from: " << options.metadataPath << std::endl;
			return false;
		}

		// Build runtime types from metadata
		if (options.preloadTypes) {
			if (!buildRuntimeTypes()) {
				std::cerr << "Failed to build runtime types" << std::endl;
				return false;
			}
		}
	}

	initialized_ = true;
	return true;
}

void Runtime::shutdown() {
	if (!initialized_) {
		return;
	}

	// TypeRegistry & MetadataLoader will clean up their resources

	// Clean up resources
	initialized_ = false;
}

bool Runtime::executeMethod(uint32_t methodToken, void* result) {
	if (!initialized_) {
		std::cerr << "Cannot execute method: Runtime not initialized" << std::endl;
		return false;
	}

	// TODO: ill be back hun

	return true;
}

bool Runtime::runProgram(const Str& assemblyName) {
	if (!initialized_) {
		std::cerr << "Cannot run program: Runtime not initialized" << std::endl;
		return false;
	}

	// Find the assembly by name
	const auto* assembly = metadata::MetadataLoader::instance().findAssemblyByName(assemblyName);
	if (!assembly) {
		std::cerr << "Assembly not found: " << assemblyName << std::endl;
		return false;
	}

	// Get the image for this assembly
	const auto& root = *metadata::MetadataLoader::instance().getMetadataRoot();
	if (assembly->imageIndex >= root.imageDefinitionCount) {
		std::cerr << "Invalid image index in assembly: " << assemblyName << std::endl;
		return false;
	}

	const auto& image = root.imageDefinitions[assembly->imageIndex];

	// Check if image has an entry point
	if (image.entryPointToken == 0) {
		std::cerr << "No entry point defined for assembly: " << assemblyName << std::endl;
		return false;
	}

	// Execute the entry point
	return executeMethod(image.entryPointToken);
}

bool Runtime::buildRuntimeTypes() {
	const auto& metadataLoader = metadata::MetadataLoader::instance();
	const auto* metadataRoot = metadataLoader.getMetadataRoot();
	if (!metadataRoot || !metadataRoot->typeDefinitions) {
		std::cerr << "No metadata types available to build" << std::endl;
		return false;
	}

	auto& registry = type_system::TypeRegistry::instance();

	// Create primitive types
	registry.initializeBuiltinTypes();

	// First pass: Create all types
	Dict<uint32_t, type_system::Type*> metadataIndexToType;

	for (uint32_t i = 0; i < metadataRoot->typeDefinitionCount; i++) {
		const auto& typeDef = metadataRoot->typeDefinitions[i];
		const auto nameStr = metadataLoader.getString(typeDef.name);
		const auto namespaceStr = metadataLoader.getString(typeDef.namespaceName);

		type_system::Type* type = nullptr;

		if (typeDef.flags.isPrimitive) {
			std::cerr << "Primitive types already initialized" << std::endl;
		}
		else if (typeDef.flags.isValueType) {
			// Create a value type
			type = new type_system::ClassType(nameStr, namespaceStr, true,
				static_cast<type_system::TypeAttributes>(typeDef.flags.visibility), typeDef.size);
		}
		else {
			// Create a reference type
			type = new type_system::ClassType(nameStr, namespaceStr, false,
				static_cast<type_system::TypeAttributes>(typeDef.flags.visibility), typeDef.size);
		}

		if (type) {
			// Set token and register
			type->setToken(typeDef.token);
			registry.registerType(type);
			metadataIndexToType[i] = type;
		}
	}

	// Second pass: Set up inheritance and add fields and methods
	for (uint32_t i = 0; i < metadataRoot->typeDefinitionCount; i++) {
		const auto& typeDef = metadataRoot->typeDefinitions[i];
		auto typeIter = metadataIndexToType.find(i);

		if (typeIter == metadataIndexToType.end()) {
			continue; // Skip if type wasn't created
		}

		auto* type = typeIter->second;

		// Handle inheritance if this is a ClassType
		if (auto* classType = dynamic_cast<type_system::ClassType*>(type)) {
			// Set base type if it exists
			if (typeDef.parentHandle > 0 && typeDef.parentHandle <= metadataRoot->typeDefinitionCount) {
				auto parentIter = metadataIndexToType.find(typeDef.parentHandle - 1);
				if (parentIter != metadataIndexToType.end()) {
					classType->setBaseType(parentIter->second);
				}
			}

			// Add fields
			for (uint32_t j = 0; j < typeDef.fieldCount; j++) {
				uint32_t fieldIndex = typeDef.fieldStart + j;
				if (fieldIndex >= metadataRoot->fieldDefinitionCount) {
					continue; // Skip invalid field index
				}

				const auto& fieldDef = metadataRoot->fieldDefinitions[fieldIndex];
				const auto fieldName = metadataLoader.getString(fieldDef.name);

				// Find field type
				type_system::Type* fieldType = nullptr;
				if (fieldDef.typeHandle > 0 && fieldDef.typeHandle <= metadataRoot->typeDefinitionCount) {
					auto fieldTypeIter = metadataIndexToType.find(fieldDef.typeHandle - 1);
					if (fieldTypeIter != metadataIndexToType.end()) {
						fieldType = fieldTypeIter->second;
					}
				}

				if (!fieldType) {
					fieldType = registry.getInt32Type(); // Default to int32 if type not found
				}

				// Add field to class
				auto field = new type_system::Field(fieldName, fieldType, 0); // Offset needs proper calculation
				classType->addField(field);
			}

			// Add methods
			for (uint32_t j = 0; j < typeDef.methodCount; j++) {
				uint32_t methodIndex = typeDef.methodStart + j;
				if (methodIndex >= metadataRoot->methodDefinitionCount) {
					continue; // Skip invalid method index
				}

				const auto& methodDef = metadataRoot->methodDefinitions[methodIndex];
				const auto methodName = metadataLoader.getString(methodDef.name);

				// Find return type
				type_system::Type* returnType = registry.getVoidType(); // Default to void
				if (methodDef.returnTypeHandle > 0 && methodDef.returnTypeHandle <= metadataRoot->typeDefinitionCount) {
					auto returnTypeIter = metadataIndexToType.find(methodDef.returnTypeHandle - 1);
					if (returnTypeIter != metadataIndexToType.end()) {
						returnType = returnTypeIter->second;
					}
				}

				// Create method
				auto method = new type_system::Method(methodName, returnType);

				// Add parameters
				for (uint32_t k = 0; k < methodDef.parameterCount; k++) {
					uint32_t paramIndex = methodDef.parameterStart + k;
					if (paramIndex >= metadataRoot->parameterDefinitionCount) {
						continue; // Skip invalid parameter index
					}

					const auto& paramDef = metadataRoot->parameterDefinitions[paramIndex];
					const auto paramName = metadataLoader.getString(paramDef.name);

					// Find parameter type
					type_system::Type* paramType = registry.getInt32Type(); // Default
					if (paramDef.typeHandle > 0 && paramDef.typeHandle <= metadataRoot->typeDefinitionCount) {
						auto paramTypeIter = metadataIndexToType.find(paramDef.typeHandle - 1);
						if (paramTypeIter != metadataIndexToType.end()) {
							paramType = paramTypeIter->second;
						}
					}

					method->addParameter(paramName, paramType);
				}

				// Add method to class
				classType->addMethod(method);
			}
		}
	}

	return true;
}

MRK_NS_END
