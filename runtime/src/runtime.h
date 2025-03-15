#pragma once

#include "metadata/metadata_loader.h"
#include "type_system/type_registry.h"

MRK_NS_BEGIN_MODULE(runtime)

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
    bool executeMethod(uint32_t methodToken, void* result = nullptr);

    /// Run a program starting from its entry point
    bool runProgram(const Str& assemblyName);

private:
    Runtime() = default;
    ~Runtime() = default;

    /// Convert metadata types to runtime types
    bool buildRuntimeTypes();

    bool initialized_ = false;
    RuntimeOptions options_;
};

MRK_NS_END