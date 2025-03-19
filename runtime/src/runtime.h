#pragma once

#include "metadata/metadata_loader.h"
#include "type_system/type_registry.h"
#include "common/logging.h"
#include "runtime_object.h"
#include "icalls.h"

MRK_NS_BEGIN_MODULE(runtime)

using namespace runtime::type_system;

struct RuntimeOptions {
	// Path to the metadata file
	Str metadataPath;

	// Whether to preload all types or load on demand
	bool preloadTypes = true;
};

/// Main runtime class
/// Contains the runtime environment and manages the execution of the program
class Runtime {
public:
	static Runtime& instance();

	Runtime(const Runtime&) = delete;
	Runtime& operator=(const Runtime&) = delete;

	/// Initializes the runtime with the given options
	bool initialize(const RuntimeOptions& options);

	/// Shuts down the runtime
	void shutdown();

    /// Check if the runtime is initialized
    bool isInitialized() const { return initialized_; }

    /// Execute a method by its metadata token
    bool executeMethod(uint32_t methodToken, void* instance, const Vec<void*>& args, void* result);

    /// Run a program starting from its entry point
    bool runProgram(const Str& assemblyName);

	void registerInternalCall(const Str& signature, InternalCall call);

	template<typename ...Args>
	void* invokeInternalCall(uint32_t methodToken, Args... args) {
		auto method = getTypeRegistry().getMethodByToken(methodToken);
		if (!method) {
			MRK_ERROR("Method not found for token: {}", methodToken);
			return nullptr;
		}

		auto sig = getInternalCallSignature(method);
		auto it = internalCalls_.find(sig);
		if (it == internalCalls_.end()) {
			MRK_ERROR("Internal call not found: {}", sig);
			return nullptr;
		}

		return it->second(args...);
	}

	template<typename T>
	void registerAllocator(uint32_t typeToken) {
		// Enta mot wla eh ya magnus
	}
	
	// Runtime externals
	// Type
	void registerType(uint32_t typeToken, size_t size);

	// Method
	void registerNativeMethod(uint32_t methodToken, void* nativeMethod);
	
	// Field
	void registerStaticField(uint32_t fieldToken, void* nativeField, void* staticInit);
	void registerNativeField(uint32_t fieldToken, void* nativeField);
	void registerStaticFieldInit(uint32_t fieldToken, void* nativeMethod);
	void registerField(uint32_t fieldToken, size_t offset);

	void* createInstance(const Type* type);
	void* destroyInstance(void* instance);

private:
	bool initialized_ = false;
	RuntimeOptions options_;
	Dict<Str, InternalCall> internalCalls_;
	Dict<void*, const Type*> instanceTable_;

    Runtime() = default;
    ~Runtime() = default;

	TypeRegistry& getTypeRegistry() { return TypeRegistry::instance(); }
	Str getInternalCallSignature(const Method* method) const;
};

MRK_NS_END