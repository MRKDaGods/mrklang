#include "runtime.h"
#include "type_system/primitive_type.h"
#include "type_system/class_type.h"
#include "type_system/array_type.h"
#include "type_system/field.h"
#include "type_system/method.h"

#include <iostream>
#include <algorithm>

MRK_NS_BEGIN_MODULE(runtime)

extern void registerInternalCalls();

namespace generated {
	extern void registerMetadata();
}

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

	MRK_INFO("Runtime initialized, registering metadata...");
	generated::registerMetadata();

	registerInternalCalls();
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

bool Runtime::executeMethod(uint32_t methodToken, void* instance, const Vec<void*>& args, void* result) {
	if (!initialized_) {
		MRK_ERROR("Cannot execute method: Runtime not initialized");
		return false;
	}

	auto* method = getTypeRegistry().getMethodByToken(methodToken);
	if (!method) {
		MRK_ERROR("Method not found for token: {}", methodToken);
		return false;
	}

	if (method->getNativeMethod()) {
		using NativeMethodPtr = void (*)();
		auto nativeMethod = reinterpret_cast<NativeMethodPtr>(method->getNativeMethod());
		nativeMethod();
		return true;
	}

	MRK_ERROR("Method not implemented: {}", method->getName());
	return false;
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
	return executeMethod(image.entryPointToken, nullptr, Vec<void*>(), nullptr);
}

void Runtime::registerInternalCall(const Str& signature, InternalCall call) {
	internalCalls_[signature] = call;
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

Str Runtime::getInternalCallSignature(const Method* method) const {
	auto typeName = method->getEnclosingType()->getFullName();
	std::replace(typeName.begin(), typeName.end(), ':', '_');
	return typeName + "_" + method->getName();
}

MRK_NS_END
