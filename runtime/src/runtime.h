#pragma once

#include "metadata/metadata_loader.h"
#include "type_system/type_registry.h"
#include "runtime_object.h"

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
    bool executeMethod(uint32_t methodToken, RuntimeObject* instance, const Vec<RuntimeObject*>& args, RuntimeObject* result = nullptr);

    /// Run a program starting from its entry point
    bool runProgram(const Str& assemblyName);

	
	// Runtime externals
	// Method
	void registerNativeMethod(uint32_t methodToken, void* nativeMethod);
	
	// Field
	void registerStaticField(uint32_t fieldToken, void* nativeField, void* staticInit);
	void registerNativeField(uint32_t fieldToken, void* nativeField);
	void registerStaticFieldInit(uint32_t fieldToken, void* nativeMethod);

private:
	bool initialized_ = false;
	RuntimeOptions options_;

    Runtime() = default;
    ~Runtime() = default;

	TypeRegistry& getTypeRegistry() { return TypeRegistry::instance(); }
};

MRK_NS_END