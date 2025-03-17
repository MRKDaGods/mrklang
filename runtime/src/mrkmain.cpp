#include "common/types.h"
#include "common/logging.h"
#include "runtime.h"

#define RUNTIME_VERSION "0.1"

using namespace mrklang::runtime;

// CONSOLE ENTRYPOINT
int main() {
	MRK_INFO("RUNTIME STARTED, mrklang v" RUNTIME_VERSION);

	auto result = Runtime::instance().initialize(RuntimeOptions{
			.metadataPath = "runtime_metadata.mrkmeta",
	});

	if (!result) {
		MRK_ERROR("Failed to initialize runtime");
		return 1;
	}

	MRK_INFO("Runtime initialized! Running mrklang_runtime");

	Runtime::instance().runProgram("mrklang_runtime");

	return 0;
}
