#include "runtime.h"
#include "type_system/primitive_type.h"
#include "type_system/class_type.h"
#include "type_system/array_type.h"
#include "type_system/field.h"
#include "type_system/method.h"
#include "type_system/type_registry.h"

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

void Runtime::registerType(uint32_t typeToken, size_t size) {
	if (!initialized_) {
		MRK_ERROR("Cannot register type: Runtime not initialized");
		return;
	}
	auto* type = getTypeRegistry().getTypeByToken(typeToken);
	if (!type) {
		MRK_ERROR("Type not found for token: {}", typeToken);
		return;
	}

	if (auto* clazz = dynamic_cast<ClassType*>(type)) {
		clazz->setSize(size);
	}
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

void Runtime::registerField(uint32_t fieldToken, size_t offset) {
	if (!initialized_) {
		MRK_ERROR("Cannot register field: Runtime not initialized");
		return;
	}

	auto* field = getTypeRegistry().getFieldByToken(fieldToken);
	if (!field) {
		MRK_ERROR("Field not found for token: {}", fieldToken);
		return;
	}

	field->setOffset(offset);
}

void* Runtime::createInstance(const Type* type) {
	if (type->getSize()) {
		auto instance = malloc(type->getSize());

		// initialize fields
		if (auto* clazz = dynamic_cast<const ClassType*>(type)) {
			for (auto* field : clazz->getFields()) {
				if (field->isStatic()) {
					continue;
				}

				// For now, properly initialize string to an empty one
				// and zero out everything else
				if (field->getFieldType() == TypeRegistry::instance().getStringType()) {
					#pragma warning(suppress: 6386)
					auto newStr = new ((char*)instance + field->getOffset()) Str();
				}
				else {
					auto sz = field->getFieldType()->getSize();
					memset(field->getValue(instance), 0, sz);
				}
			}
		}

		instanceTable_[instance] = type;

		return instance;
	}

	return nullptr;
}

void* Runtime::destroyInstance(void* instance) {
	auto it = instanceTable_.find(instance);
	if (it == instanceTable_.end()) {
		MRK_ERROR("Instance not found in table");
		return nullptr;
	}

	// Destruct types, for now only strings
	// TODO: Impl object model w respective dtors
	auto* type = it->second;
	if (type == TypeRegistry::instance().getStringType()) {
		auto* str = static_cast<Str*>(instance);
		//str->~basic_string();
		delete str;
	}

	instanceTable_.erase(it);
	free(instance);
}

Str Runtime::getInternalCallSignature(const Method* method) const {
	auto typeName = method->getEnclosingType()->getFullName();
	std::replace(typeName.begin(), typeName.end(), ':', '_');
	return typeName + "_" + method->getName();
}

namespace api {
	using namespace runtime::type_system;

	static Runtime& rt = Runtime::instance();
	static TypeRegistry& ts = TypeRegistry::instance();
	static MetadataLoader& ml = MetadataLoader::instance();

	// Object
	void* mrk_create_instance(const TypeDefinition* type) {
		return Runtime::instance().createInstance(ts.getTypeByToken(type->token));
	}

	void mrk_destroy_instance(void* instance) {
		Runtime::instance().destroyInstance(instance);
	}

	// Type
	const TypeDefinition* mrk_get_type(const Str& fullName) {
		return ml.findTypeDefinition(fullName);
	}

	// Field
	const FieldDefinition* mrk_get_field(const TypeDefinition* type, const Str& name) {
		return ml.findFieldDefinition(*type, name);
	}

	void* mrk_get_field_value(const FieldDefinition* field, void* instance) {
		auto* f = ts.getFieldByToken(field->token);
		return f->getValue(instance);
	}

	void mrk_set_field_value(const FieldDefinition* field, void* instance, void* value) {
		auto* f = ts.getFieldByToken(field->token);
		f->setValue(value, instance);
	}
}

MRK_NS_END
