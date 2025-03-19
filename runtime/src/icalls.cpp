#include "icalls.h"
#include "common/types.h"
#include "runtime.h"
#include "runtime-api/mrk-metadata.h"

#include <iostream>

MRK_NS_BEGIN_MODULE(runtime)

using namespace api;

void registerInternalCall(const Str& sig, InternalCall call) { 
	Runtime::instance().registerInternalCall(sig, call); 
}

void* mrk__alloc(uint32_t len) {
	void* ptr = malloc(len);
	if (ptr) {
		memset(ptr, 0, len);
	}

	return ptr;
}

void* mrk__web__Http_request(const Str& url, int method) {
	// get http response type
	auto* httpResponseType = mrk_get_type("__global::mrk::web::HttpResponse");
	if (!httpResponseType) {
		std::cerr << "Http response type not found" << std::endl;
		return nullptr;
	}

	// create new
	auto* response = mrk_create_instance(httpResponseType);

	// set fields
	auto* codeField = mrk_get_field(httpResponseType, "code");
	if (codeField) {
		int code = 696969 - method;
		mrk_set_field_value(codeField, response, &code);
	}

	auto* bodyField = mrk_get_field(httpResponseType, "body");
	if (bodyField) {
		Str body = "Hello, World!" + url;
		mrk_set_field_value(bodyField, response, &body);
	}

	return response;
}

void registerInternalCalls() {
	registerInternalCall("__global__mrk__alloc", (InternalCall)mrk__alloc);
	registerInternalCall("__global__mrk__web__Http_request", (InternalCall)mrk__web__Http_request);
}

MRK_NS_END