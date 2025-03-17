#include "runtime.h"
#include "runtime_defines.h"
// Rigid block: __cpp2331676006384

#include <iostream>

MRK_NS_BEGIN_MODULE(runtime::generated)

// Forward declarations
struct __global____globalType_2331676099648;
struct __global__mrk__web__HttpMethod_2331676100032;
struct __global__mrk__web__Http_2331676100416;
// Type: __global::__globalType, Token: 17
struct __global____globalType_2331676099648 {
    // Function: __global::__globalType::main, Token: 1
    static __mrkprimitive_void main_2331676138480();
        // Function: __global::__globalType::print, Token: 2
    static __mrkprimitive_void print_2331676137360(__mrkprimitive_string msg_2331676132880);
        // Function: __global::__globalType::readNumber, Token: 3
    static __mrkprimitive_int readNumber_2331676127648();
        // Function: __global::__globalType::__globalFunction, Token: 4
    static __mrkprimitive_void __globalFunction_2331676101760();
        // Function: __global::__globalType::testNative, Token: 5
    static __mrkprimitive_void testNative_2331676001184();
};
// Type: __global::mrk::web::HttpMethod, Token: 18
struct __global__mrk__web__HttpMethod_2331676100032 {
    // Variable: POST, Token: 1
    static     __mrkprimitive_int POST_2331676108048;
        // Variable: GET, Token: 2
    static     __mrkprimitive_int GET_2331676107680;
};
// Type: __global::mrk::web::Http, Token: 19
struct __global__mrk__web__Http_2331676100416 {
    // Function: __global::mrk::web::Http::request, Token: 6
    static __mrkprimitive_bool request_2331676126896(__mrkprimitive_string url_2331676126160, __mrkprimitive_int method_2331676126528);
};
// Function: __global::mrk::web::Http::request, Token: 6
__mrkprimitive_bool __global__mrk__web__Http_2331676100416::request_2331676126896(__mrkprimitive_string url_2331676126160, __mrkprimitive_int method_2331676126528) {
    // Native function: __global::mrk::web::Http::request
    return MRK_INVOKE_ICALL(6, url_2331676126160, method_2331676126528);
}
// Function: __global::__globalType::readNumber, Token: 3
__mrkprimitive_int __global____globalType_2331676099648::readNumber_2331676127648() {
    __mrkprimitive_int  num;

    std::cin >> num;

    return num;
}
// Function: __global::__globalType::print, Token: 2
__mrkprimitive_void __global____globalType_2331676099648::print_2331676137360(__mrkprimitive_string msg_2331676132880) {
    __mrkprimitive_string  m = msg_2331676132880;

    std::cout << m << std::endl;

}
// Function: __global::__globalType::main, Token: 1
__mrkprimitive_void __global____globalType_2331676099648::main_2331676138480() {

    std::cout << "hey there, enter number: ";

    __mrkprimitive_int  num = MRK_STATIC_MEMBER(__global____globalType_2331676099648, readNumber_2331676127648)();

    std::cout << "u entered: " << num << std::endl;

    __mrkprimitive_bool  res_2331676164208 = __global__mrk__web__Http_2331676100416::request_2331676126896("AMMAR MAGNUS.com", __global__mrk__web__HttpMethod_2331676100032::GET_2331676107680);
    MRK_STATIC_MEMBER(__global____globalType_2331676099648, print_2331676137360)(res_2331676164208 ? "NICE" : "NO");
}
// Function: __global::__globalType::testNative, Token: 5
__mrkprimitive_void __global____globalType_2331676099648::testNative_2331676001184() {
    // Native function: __global::__globalType::testNative
    MRK_INVOKE_ICALL(5);
}
// Function: __global::__globalType::__globalFunction, Token: 4
__mrkprimitive_void __global____globalType_2331676099648::__globalFunction_2331676101760() {
    MRK_STATIC_MEMBER(__global____globalType_2331676099648, main_2331676138480)();
}
// Static field initializer: __global::mrk::web::HttpMethod::POST
__mrkprimitive_int staticFieldInit_2331676108048() {
    return 1;
}
__mrkprimitive_int __global__mrk__web__HttpMethod_2331676100032::POST_2331676108048 = staticFieldInit_2331676108048();
// Static field initializer: __global::mrk::web::HttpMethod::GET
__mrkprimitive_int staticFieldInit_2331676107680() {
    return 0;
}
__mrkprimitive_int __global__mrk__web__HttpMethod_2331676100032::GET_2331676107680 = staticFieldInit_2331676107680();

// Metadata registration
void registerMetadata() {
    // Register native methods
    MRK_RUNTIME_REGISTER_CODE(1, __global____globalType_2331676099648::main_2331676138480);
    MRK_RUNTIME_REGISTER_CODE(2, __global____globalType_2331676099648::print_2331676137360);
    MRK_RUNTIME_REGISTER_CODE(4, __global____globalType_2331676099648::__globalFunction_2331676101760);
    MRK_RUNTIME_REGISTER_CODE(3, __global____globalType_2331676099648::readNumber_2331676127648);
    MRK_RUNTIME_REGISTER_CODE(5, __global____globalType_2331676099648::testNative_2331676001184);
    MRK_RUNTIME_REGISTER_CODE(6, __global__mrk__web__Http_2331676100416::request_2331676126896);
    // Register types
    MRK_RUNTIME_REGISTER_TYPE(1, __mrkprimitive_void);
    MRK_RUNTIME_REGISTER_TYPE(2, __mrkprimitive_bool);
    MRK_RUNTIME_REGISTER_TYPE(3, __mrkprimitive_char);
    MRK_RUNTIME_REGISTER_TYPE(19, __global__mrk__web__Http_2331676100416);
    MRK_RUNTIME_REGISTER_TYPE(6, __mrkprimitive_short);
    MRK_RUNTIME_REGISTER_TYPE(4, __mrkprimitive_i8);
    MRK_RUNTIME_REGISTER_TYPE(7, __mrkprimitive_ushort);
    MRK_RUNTIME_REGISTER_TYPE(5, __mrkprimitive_byte);
    MRK_RUNTIME_REGISTER_TYPE(15, __mrkprimitive_object);
    MRK_RUNTIME_REGISTER_TYPE(8, __mrkprimitive_int);
    MRK_RUNTIME_REGISTER_TYPE(14, __mrkprimitive_string);
    MRK_RUNTIME_REGISTER_TYPE(9, __mrkprimitive_uint);
    MRK_RUNTIME_REGISTER_TYPE(10, __mrkprimitive_long);
    MRK_RUNTIME_REGISTER_TYPE(11, __mrkprimitive_ulong);
    MRK_RUNTIME_REGISTER_TYPE(12, __mrkprimitive_float);
    MRK_RUNTIME_REGISTER_TYPE(13, __mrkprimitive_double);
    MRK_RUNTIME_REGISTER_TYPE(16, __mrkprimitive_void*);
    MRK_RUNTIME_REGISTER_TYPE(17, __global____globalType_2331676099648);
    MRK_RUNTIME_REGISTER_TYPE(18, __global__mrk__web__HttpMethod_2331676100032);
    // Register static fields
    MRK_RUNTIME_REGISTER_STATIC_FIELD(1, __global__mrk__web__HttpMethod_2331676100032::POST_2331676108048, staticFieldInit_2331676108048);
    MRK_RUNTIME_REGISTER_STATIC_FIELD(2, __global__mrk__web__HttpMethod_2331676100032::GET_2331676107680, staticFieldInit_2331676107680);
}
MRK_NS_END
