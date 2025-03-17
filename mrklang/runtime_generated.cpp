#include "runtime.h"
#include "runtime_defines.h"
MRK_NS_BEGIN_MODULE(runtime::generated)

// Rigid block: __cpp2074874548528

	#include <iostream>

// Type: __global::void, Token: 1
struct __global__void_2074874564784 {
};
// Type: __global::bool, Token: 2
struct __global__bool_2074874565168 {
};
// Type: __global::char, Token: 3
struct __global__char_2074874599136 {
};
// Type: __global::i8, Token: 4
struct __global__i8_2074874599520 {
};
// Type: __global::byte, Token: 5
struct __global__byte_2074874599904 {
};
// Type: __global::short, Token: 6
struct __global__short_2074874600288 {
};
// Type: __global::ushort, Token: 7
struct __global__ushort_2074874600672 {
};
// Type: __global::int, Token: 8
struct __global__int_2074874601056 {
};
// Type: __global::uint, Token: 9
struct __global__uint_2074874565552 {
};
// Type: __global::long, Token: 10
struct __global__long_2074874568112 {
};
// Type: __global::ulong, Token: 11
struct __global__ulong_2074874556192 {
};
// Type: __global::float, Token: 12
struct __global__float_2074874556576 {
};
// Type: __global::double, Token: 13
struct __global__double_2074874556960 {
};
// Type: __global::string, Token: 14
struct __global__string_2074874557344 {
};
// Type: __global::object, Token: 15
struct __global__object_2074874607552 {
};
// Type: __global::__globalType, Token: 16
struct __global____globalType_2074874609088 {
    // Variable: x, Token: 1
static     __global__int_2074874601056 x_2074874559872;
};
// Function: __global::__globalType::readNumber, Token: 2
__global__int_2074874601056 readNumber_2074874617872() {
        __global__int_2074874601056  num = MRK_STATIC_MEMBER(__global____globalType_2074874609088, x_2074874559872);
    
		std::cin >> num;
	
}
// Function: __global::__globalType::main, Token: 3
__global__void_2074874564784 main_2074874618992() {
    
		std::cout << "hey there, enter number: ";
	
        __global__int_2074874601056  num = MRK_STATIC_MEMBER(__global____globalType_2074874609088, readNumber_2074874617872)();
    
		std::cout << "u entered: " << num << std::endl;
	
}
// Function: __global::__globalType::__globalFunction, Token: 1
__global__void_2074874564784 __globalFunction_2074874558032() {
MRK_STATIC_MEMBER(__global____globalType_2074874609088, main_2074874618992)();
}
// Static field initializer: __global::__globalType::x
void staticFieldInit_2074874559872() {
}

// Metadata registration
void registerMetadata() {
    // Register native methods
    MRK_RUNTIME_REGISTER_CODE(1, __globalFunction_2074874558032);
    MRK_RUNTIME_REGISTER_CODE(2, readNumber_2074874617872);
    MRK_RUNTIME_REGISTER_CODE(3, main_2074874618992);
    // Register types
    MRK_RUNTIME_REGISTER_TYPE(4, __global__i8_2074874599520);
    MRK_RUNTIME_REGISTER_TYPE(1, __global__void_2074874564784);
    MRK_RUNTIME_REGISTER_TYPE(2, __global__bool_2074874565168);
    MRK_RUNTIME_REGISTER_TYPE(8, __global__int_2074874601056);
    MRK_RUNTIME_REGISTER_TYPE(3, __global__char_2074874599136);
    MRK_RUNTIME_REGISTER_TYPE(5, __global__byte_2074874599904);
    MRK_RUNTIME_REGISTER_TYPE(6, __global__short_2074874600288);
    MRK_RUNTIME_REGISTER_TYPE(7, __global__ushort_2074874600672);
    MRK_RUNTIME_REGISTER_TYPE(9, __global__uint_2074874565552);
    MRK_RUNTIME_REGISTER_TYPE(10, __global__long_2074874568112);
    MRK_RUNTIME_REGISTER_TYPE(11, __global__ulong_2074874556192);
    MRK_RUNTIME_REGISTER_TYPE(12, __global__float_2074874556576);
    MRK_RUNTIME_REGISTER_TYPE(13, __global__double_2074874556960);
    MRK_RUNTIME_REGISTER_TYPE(14, __global__string_2074874557344);
    MRK_RUNTIME_REGISTER_TYPE(15, __global__object_2074874607552);
    MRK_RUNTIME_REGISTER_TYPE(16, __global____globalType_2074874609088);
    // Register static fields
    MRK_RUNTIME_REGISTER_STATIC_FIELD(1, __global____globalType_2074874609088::x_2074874559872, staticFieldInit_2074874559872);
}
MRK_NS_END
