#include "runtime.h"
#include "common/logging.h"
#include "type_system/primitive_type.h"
#include "type_system/class_type.h"
#include "type_system/array_type.h"
#include "type_system/field.h"
#include "type_system/method.h"
#include <iostream>

MRK_NS_BEGIN_MODULE(runtime)

using namespace type_system;
using namespace metadata;

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
	auto& typeRegistry = getTypeRegistry();
	typeRegistry.initializeBuiltinTypes();

	// Load metadata if path provided
	if (!options.metadataPath.empty()) {
		auto& metadataLoader = MetadataLoader::instance();
		if (!metadataLoader.loadFromFile(options.metadataPath)) {
			MRK_ERROR("Failed to load metadata from: {}", options.metadataPath);
			return false;
		}

		MRK_INFO("Loaded metadata from: {}", options.metadataPath);
		typeRegistry.initializeMetadata(metadataLoader.getMetadataRoot());
	}

	typeRegistry.dumpTree();

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

bool Runtime::executeMethod(uint32_t methodToken, RuntimeObject* instance, const Vec<RuntimeObject*>& args, RuntimeObject* result) {
	if (!initialized_) {
		MRK_ERROR("Cannot execute method: Runtime not initialized");
		return false;
	}

	auto* method = getTypeRegistry().getMethodByToken(methodToken);
	if (!method) {
		MRK_ERROR("Method not found for token: {}", methodToken);
		return false;
	}

	// Check native method
	auto* nativeMethod = method->getNativeMethod();
	if (!nativeMethod) {
		MRK_ERROR("Native method not registered for method: {}", method->getName());
		return false;
	}

	// Check params
	auto parameters = method->getParameters();
	if (args.size() != parameters.size()) {
		MRK_ERROR("Invalid number of arguments for method: {}", method->getName());
		return false;
	}

	Vec<void*> nativeArgs;
	if (!method->isStatic()) {
		// Add instance as first argument
		if (!instance) {
			MRK_ERROR("Instance not provided for instance method: {}", method->getName());
			return false;
		}

		if (instance->isValueType()) {
			MRK_ERROR("Instance is a value type for instance method: {}", method->getName());
			return false;
		}

		// Must be of ref type
		nativeArgs.push_back(static_cast<ReferenceTypeObject*>(instance)->getData());
	}

	// Add the method arguments
	for (size_t i = 0; i < args.size(); ++i) {
		RuntimeObject* arg = args[i];
		if (!arg) {
			MRK_ERROR("Null argument at position {}", i);
			return false;
		}

		const auto& param = parameters[i];
		// TODO: Make sure argument type matches parameter type

		if (arg->isValueType()) {
			// Extract the value from ValueTypeObject based on the parameter type
			if (param.getType()->getTypeKind() == TypeKind::BOOL) {
				bool value = static_cast<ValueTypeObject<bool>*>(arg)->getValue();
				nativeArgs.push_back(&value);
			}
			else if (param.getType()->getTypeKind() == TypeKind::I32) {
				int32_t value = static_cast<ValueTypeObject<int32_t>*>(arg)->getValue();
				nativeArgs.push_back(&value);
			}
			else {
				// Handle other primitive types
				MRK_ERROR("Unsupported value type parameter at position {}", i);
				return false;
			}
		}
		else {
			// Reference type - pass the pointer to the object data
			nativeArgs.push_back(static_cast<ReferenceTypeObject*>(arg)->getData());
		}
	}

	// Call the native method
	try {
		typedef void (*VoidFunc)(void**, void*);
		VoidFunc funcPtr = reinterpret_cast<VoidFunc>(nativeMethod);

		void* resultPtr = nullptr;
		if (result) {
			if (result->isValueType()) {
				// For value types, we'll need to handle differently based on type
				resultPtr = result;
			}
			else {
				// For reference types, we use the object data
				resultPtr = static_cast<ReferenceTypeObject*>(result)->getData();
			}
		}

		// Call the function with arguments and result pointer
		funcPtr(nativeArgs.data(), resultPtr);

		MRK_INFO("Successfully executed method: {}", method->getName());
		return true;
	}
	catch (const std::exception& e) {
		MRK_ERROR("Exception during method execution: {}", e.what());
		return false;
	}

	return true;
}

bool Runtime::runProgram(const Str& assemblyName) {
	if (!initialized_) {
		MRK_ERROR("Cannot run program: Runtime not initialized");
		return false;
	}

	// Find the assembly by name
	const auto* assembly = MetadataLoader::instance().findAssemblyByName(assemblyName);
	if (!assembly) {
		MRK_ERROR("Assembly not found: {}", assemblyName);
		return false;
	}

	// Get the image for this assembly
	const auto& root = *MetadataLoader::instance().getMetadataRoot();
	if (assembly->imageIndex >= root.imageDefinitionCount) {
		MRK_ERROR("Invalid image index in assembly: {}", assemblyName);
		return false;
	}

	const auto& image = root.imageDefinitions[assembly->imageIndex];

	// Check if image has an entry point
	if (image.entryPointToken == 0) {
		MRK_ERROR("No entry point defined for assembly: {}", assemblyName);
		return false;
	}

	// Execute the entry point
	return executeMethod(image.entryPointToken, nullptr, {});
}

void Runtime::registerNativeMethod(uint32_t methodToken, void* nativeMethod) {
	if (!initialized_) {
		MRK_ERROR("Cannot register native method: Runtime not initialized");
		return;
	}

	auto* method = getTypeRegistry().getMethodByToken(methodToken);
	if (!method) {
		MRK_ERROR("Method not found for token: {}", methodToken);
		return;
	}

	method->setNativeMethod(nativeMethod);
}

void Runtime::registerStaticField(uint32_t fieldToken, void* nativeField, void* staticInit) {
	registerNativeField(fieldToken, nativeField);
	registerStaticFieldInit(fieldToken, staticInit);
}

void Runtime::registerNativeField(uint32_t fieldToken, void* nativeField) {
	if (!initialized_) {
		MRK_ERROR("Cannot register native method: Runtime not initialized");
		return;
	}

	auto* field = getTypeRegistry().getFieldByToken(fieldToken);
	if (!field) {
		MRK_ERROR("Field not found for token: {}", fieldToken);
		return;
	}

	field->setNativeField(nativeField);
}

void Runtime::registerStaticFieldInit(uint32_t fieldToken, void* nativeMethod) {
	if (!initialized_) {
		MRK_ERROR("Cannot register static field init: Runtime not initialized");
		return;
	}

	auto* field = getTypeRegistry().getFieldByToken(fieldToken);
	if (!field) {
		MRK_ERROR("Field not found for token: {}", fieldToken);
		return;
	}

	field->setStaticInit(nativeMethod);
}

MRK_NS_END
