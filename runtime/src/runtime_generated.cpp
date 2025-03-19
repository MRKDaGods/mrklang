#include "runtime.h"
#include "runtime_defines.h"
MRK_NS_BEGIN_MODULE(runtime::generated)

// Forward declarations
struct __global____globalType_2458067366784;
struct __global__Array_2458067367552;
struct __global__mrk__models__User_2458067369088;
struct __global__mrk__models__Company_2458067439200;
struct __global__mrk__web__HttpMethod_2458067440352;
struct __global__mrk__web__HttpResponse_2458067440736;
struct __global__mrk__web__Http_2458067441504;
// Type: __global::__globalType, Token: 17
struct __global____globalType_2458067366784 {
    // Function: __global::__globalType::writeInt, Token: 1
static __mrkprimitive_void writeInt_2458067406768(__mrkprimitive_int __msg);
    // Function: __global::__globalType::__globalFunction, Token: 2
static __mrkprimitive_void __globalFunction_2458067242544();
    // Function: __global::__globalType::alloc, Token: 3
template<typename T>
static T alloc_2458067244416();
    // Function: __global::__globalType::writeStr, Token: 4
static __mrkprimitive_void writeStr_2458067416144(__mrkprimitive_string __msg);
    // Function: __global::__globalType::deref, Token: 5
template<typename T>
static T deref_2458067383216(__mrkprimitive_void* __ptr);
    // Function: __global::__globalType::free, Token: 6
static __mrkprimitive_void free_2458067382368(__mrkprimitive_void* __ptr);
    // Function: __global::__globalType::setptr, Token: 7
template<typename T>
static __mrkprimitive_void setptr_2458067385104(__mrkprimitive_void* __ptr, T __value);
    // Function: __global::__globalType::main, Token: 8
static __mrkprimitive_void main_2458067497072();
};
// Type: __global::Array, Token: 18
struct __global__Array_2458067367552 {
    
// Variable: data, Token: 1
    __mrkprimitive_void* data_2458067425440;
};
// Type: __global::mrk::models::User, Token: 19
struct __global__mrk__models__User_2458067369088 {
    
// Variable: last_name, Token: 2
    __mrkprimitive_string last_name_2458067435296;
    
// Variable: first_name, Token: 3
    __mrkprimitive_string first_name_2458067434928;
};
// Type: __global::mrk::models::Company, Token: 20
struct __global__mrk__models__Company_2458067439200 {
    
// Variable: name, Token: 4
    __mrkprimitive_string name_2458067444464;
    
// Variable: age, Token: 5
    __mrkprimitive_int age_2458067243328;
};
// Type: __global::mrk::web::HttpMethod, Token: 21
struct __global__mrk__web__HttpMethod_2458067440352 {
    
// Variable: POST, Token: 6
static     __mrkprimitive_int POST_2458067476896;
    
// Variable: GET, Token: 7
static     __mrkprimitive_int GET_2458067474976;
};
// Type: __global::mrk::web::HttpResponse, Token: 22
struct __global__mrk__web__HttpResponse_2458067440736 {
    
// Variable: body, Token: 8
    __mrkprimitive_string body_2458067475792;
    
// Variable: code, Token: 9
    __mrkprimitive_int code_2458067477264;
};
// Type: __global::mrk::web::Http, Token: 23
struct __global__mrk__web__Http_2458067441504 {
    // Function: __global::mrk::web::Http::request, Token: 9
static __global__mrk__web__HttpResponse_2458067440736* request_2458067496192(__mrkprimitive_string url_2458067477632, __mrkprimitive_int method_2458067476160);
};
// Function: __global::__globalType::alloc, Token: 3
template<typename T>
T __global____globalType_2458067366784::alloc_2458067244416() {
        __mrkprimitive_void*  ptr;
    
        using TPTRLESS = std::remove_pointer<T*>;
        ptr            = new TPTRLESS;
        
        return reinterpret_cast<T>(ptr);
    
}
// Function: __global::__globalType::free, Token: 6
__mrkprimitive_void __global____globalType_2458067366784::free_2458067382368(__mrkprimitive_void* __ptr) {
    
        delete __ptr;
    
}
// Function: __global::__globalType::deref, Token: 5
template<typename T>
T __global____globalType_2458067366784::deref_2458067383216(__mrkprimitive_void* __ptr) {
        T  res;
    
        res = *reinterpret_cast<T*>(__ptr);
    
    return res;
}
// Function: __global::__globalType::setptr, Token: 7
template<typename T>
__mrkprimitive_void __global____globalType_2458067366784::setptr_2458067385104(__mrkprimitive_void* __ptr, T __value) {
    
        *reinterpret_cast<T*>(__ptr) = __value;
    
}
// Function: __global::__globalType::writeInt, Token: 1
__mrkprimitive_void __global____globalType_2458067366784::writeInt_2458067406768(__mrkprimitive_int __msg) {
    
        std::cout << __msg << std::endl;
    
}
// Function: __global::__globalType::writeStr, Token: 4
__mrkprimitive_void __global____globalType_2458067366784::writeStr_2458067416144(__mrkprimitive_string __msg) {
    
        std::cout << __msg << std::endl;
    
}
// Function: __global::mrk::web::Http::request, Token: 9
__global__mrk__web__HttpResponse_2458067440736* __global__mrk__web__Http_2458067441504::request_2458067496192(__mrkprimitive_string url_2458067477632, __mrkprimitive_int method_2458067476160) {
    // Native function: __global::mrk::web::Http::request
return (__global__mrk__web__HttpResponse_2458067440736*)MRK_INVOKE_ICALL(9, url_2458067477632, method_2458067476160);
}
// Function: __global::__globalType::main, Token: 8
__mrkprimitive_void __global____globalType_2458067366784::main_2458067497072() {
    __global__mrk__web__HttpResponse_2458067440736*  res_2458067478736 = __global__mrk__web__Http_2458067441504::__global__mrk__web__Http_2458067441504::request_2458067496192("urlSample", __global__mrk__web__HttpMethod_2458067440352::GET_2458067474976);
    MRK_STATIC_MEMBER(__global____globalType_2458067366784, writeInt_2458067406768)(res_2458067478736->code_2458067477264);
    MRK_STATIC_MEMBER(__global____globalType_2458067366784, writeStr_2458067416144)(res_2458067478736->body_2458067475792);
    MRK_STATIC_MEMBER(__global____globalType_2458067366784, free_2458067382368)(res_2458067478736);
}
// Function: __global::__globalType::__globalFunction, Token: 2
__mrkprimitive_void __global____globalType_2458067366784::__globalFunction_2458067242544() {
MRK_STATIC_MEMBER(__global____globalType_2458067366784, main_2458067497072)();
}

// Static field initializer: __global::mrk::web::HttpMethod::POST
__mrkprimitive_int staticFieldInit_2458067476896() {
    return 1    ;
}
__mrkprimitive_int __global__mrk__web__HttpMethod_2458067440352::POST_2458067476896 = staticFieldInit_2458067476896();

// Static field initializer: __global::mrk::web::HttpMethod::GET
__mrkprimitive_int staticFieldInit_2458067474976() {
    return 0    ;
}
__mrkprimitive_int __global__mrk__web__HttpMethod_2458067440352::GET_2458067474976 = staticFieldInit_2458067474976();

// Metadata registration
void registerMetadata() {
    // Register native methods
    MRK_RUNTIME_REGISTER_CODE(1, __global____globalType_2458067366784::writeInt_2458067406768);
    MRK_RUNTIME_REGISTER_CODE(2, __global____globalType_2458067366784::__globalFunction_2458067242544);
    MRK_RUNTIME_REGISTER_CODE(4, __global____globalType_2458067366784::writeStr_2458067416144);
    MRK_RUNTIME_REGISTER_CODE(6, __global____globalType_2458067366784::free_2458067382368);
    MRK_RUNTIME_REGISTER_CODE(8, __global____globalType_2458067366784::main_2458067497072);
    MRK_RUNTIME_REGISTER_CODE(9, __global__mrk__web__Http_2458067441504::request_2458067496192);
    
// Register types
    MRK_RUNTIME_REGISTER_TYPE(9, __mrkprimitive_uint);
    MRK_RUNTIME_REGISTER_TYPE(1, __mrkprimitive_void);
    MRK_RUNTIME_REGISTER_TYPE(2, __mrkprimitive_bool);
    MRK_RUNTIME_REGISTER_TYPE(3, __mrkprimitive_char);
    MRK_RUNTIME_REGISTER_TYPE(7, __mrkprimitive_ushort);
    MRK_RUNTIME_REGISTER_TYPE(4, __mrkprimitive_i8);
    MRK_RUNTIME_REGISTER_TYPE(10, __mrkprimitive_long);
    MRK_RUNTIME_REGISTER_TYPE(5, __mrkprimitive_byte);
    MRK_RUNTIME_REGISTER_TYPE(6, __mrkprimitive_short);
    MRK_RUNTIME_REGISTER_TYPE(20, __global__mrk__models__Company_2458067439200);
    MRK_RUNTIME_REGISTER_TYPE(17, __global____globalType_2458067366784);
    MRK_RUNTIME_REGISTER_TYPE(8, __mrkprimitive_int);
    MRK_RUNTIME_REGISTER_TYPE(11, __mrkprimitive_ulong);
    MRK_RUNTIME_REGISTER_TYPE(12, __mrkprimitive_float);
    MRK_RUNTIME_REGISTER_TYPE(13, __mrkprimitive_double);
    MRK_RUNTIME_REGISTER_TYPE(14, __mrkprimitive_string);
    MRK_RUNTIME_REGISTER_TYPE(15, __mrkprimitive_object);
    MRK_RUNTIME_REGISTER_TYPE(16, __mrkprimitive_void*);
    MRK_RUNTIME_REGISTER_TYPE(18, __global__Array_2458067367552);
    MRK_RUNTIME_REGISTER_TYPE(19, __global__mrk__models__User_2458067369088);
    MRK_RUNTIME_REGISTER_TYPE(21, __global__mrk__web__HttpMethod_2458067440352);
    MRK_RUNTIME_REGISTER_TYPE(22, __global__mrk__web__HttpResponse_2458067440736);
    MRK_RUNTIME_REGISTER_TYPE(23, __global__mrk__web__Http_2458067441504);
    
// Register fields
    MRK_RUNTIME_REGISTER_FIELD(9, offsetof(__global__mrk__web__HttpResponse_2458067440736, code_2458067477264));
    MRK_RUNTIME_REGISTER_FIELD(1, offsetof(__global__Array_2458067367552, data_2458067425440));
    MRK_RUNTIME_REGISTER_FIELD(2, offsetof(__global__mrk__models__User_2458067369088, last_name_2458067435296));
    MRK_RUNTIME_REGISTER_FIELD(3, offsetof(__global__mrk__models__User_2458067369088, first_name_2458067434928));
    MRK_RUNTIME_REGISTER_FIELD(4, offsetof(__global__mrk__models__Company_2458067439200, name_2458067444464));
    MRK_RUNTIME_REGISTER_FIELD(8, offsetof(__global__mrk__web__HttpResponse_2458067440736, body_2458067475792));
    MRK_RUNTIME_REGISTER_FIELD(5, offsetof(__global__mrk__models__Company_2458067439200, age_2458067243328));
    
// Register static fields
    MRK_RUNTIME_REGISTER_STATIC_FIELD(6, __global__mrk__web__HttpMethod_2458067440352::POST_2458067476896, staticFieldInit_2458067476896);
    __global__mrk__web__HttpMethod_2458067440352::POST_2458067476896 = staticFieldInit_2458067476896();
    MRK_RUNTIME_REGISTER_STATIC_FIELD(7, __global__mrk__web__HttpMethod_2458067440352::GET_2458067474976, staticFieldInit_2458067474976);
    __global__mrk__web__HttpMethod_2458067440352::GET_2458067474976 = staticFieldInit_2458067474976();
}
MRK_NS_END
