#include "icalls.h"
#include "common/types.h"
#include "runtime.h"

#include <iostream>

MRK_NS_BEGIN_MODULE(runtime)

void registerInternalCall(const Str& sig, InternalCall call) { 
	Runtime::instance().registerInternalCall(sig, call); 
}

bool mrk__web__Http__request(const Str& url, int method) {
	std::cout << "mrk__web__http_request: " << url << ", " << method << std::endl;
	return true;
}

void registerInternalCalls() {
	registerInternalCall("__global__mrk__web__Http_request", (InternalCall)mrk__web__Http__request);
}

MRK_NS_END