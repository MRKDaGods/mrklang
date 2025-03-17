#include "runtime.h"
#include "runtime_defines.h"
MRK_NS_BEGIN_MODULE(runtime::generated)

// Rigid block: __cpp2901448660848

 // WOW
 #include <iostream>
 std::cout << "Hello, World!" << std::endl;

// Type: __global::void, Token: 1
struct __global__void_2901448674352 {
};
// Type: __global::bool, Token: 2
struct __global__bool_2901448674736 {
};
// Type: __global::char, Token: 3
struct __global__char_2901448720048 {
};
// Type: __global::i8, Token: 4
struct __global__i8_2901448720432 {
};
// Type: __global::byte, Token: 5
struct __global__byte_2901448675120 {
};
// Type: __global::short, Token: 6
struct __global__short_2901448675504 {
};
// Type: __global::ushort, Token: 7
struct __global__ushort_2901448675888 {
};
// Type: __global::int, Token: 8
struct __global__int_2901448676272 {
};
// Type: __global::uint, Token: 9
struct __global__uint_2901448676656 {
};
// Type: __global::long, Token: 10
struct __global__long_2901448678128 {
};
// Type: __global::ulong, Token: 11
struct __global__ulong_2901448678512 {
};
// Type: __global::float, Token: 12
struct __global__float_2901448763136 {
};
// Type: __global::double, Token: 13
struct __global__double_2901448763520 {
};
// Type: __global::string, Token: 14
struct __global__string_2901448768016 {
};
// Type: __global::object, Token: 15
struct __global__object_2901448690592 {
};
// Type: __global::__globalType, Token: 16
struct __global____globalType_2901448690208 {
    // Variable: AMMAR, Token: 1
static     __global__int_2901448676272 AMMAR_2901448699872;
};
// Type: __global::TestClass, Token: 17
struct __global__TestClass_2901448691360 {
    // Variable: MRKSTA, Token: 3
static     __global__string_2901448768016 MRKSTA_2901448794464;
    // Variable: y, Token: 4
    __global__int_2901448676272 y_2901448794832;
    // Variable: pi, Token: 5
    __global__double_2901448763520 pi_2901448795200;
};
// Type: __global::TestClass::TestStruct, Token: 18
struct __global__TestClass__TestStruct_2901448690976 {
    // Variable: x, Token: 2
    __global__int_2901448676272 x_2901448701152;
};
// Function: __global::__globalType::print, Token: 2
__global__void_2901448674352 print_2901448694560(__global__string_2901448768016 msg_2901448769152) {
    msg_2901448769152 += "123";
}
// Function: __global::__globalType::main, Token: 3
__global__void_2901448674352 main_2901448695312() {
    MRK_STATIC_MEMBER(__global____globalType_2901448690208, print_2901448694560)("Hello, World!");
}
// Function: __global::TestClass::ctor, Token: 4
__global__void_2901448674352 ctor_2901448795568(__global__TestClass_2901448691360* __instance) {
    print_2901448694560("Hello, World!");
    if (MRK_INSTANCE_MEMBER(pi_2901448795200) > 3.0)
    {
        print_2901448694560("Pi is greater than 3.0");
    }
    else
    {
        print_2901448694560("Pi is less than or equal to 3.0");
    }
}
// Function: __global::TestClass::dtor, Token: 5
__global__void_2901448674352 dtor_2901448693472(__global__TestClass_2901448691360* __instance) {
    print_2901448694560("Goodbye, World!");
}
// Function: __global::TestClass::test, Token: 6
__global__void_2901448674352 test_2901448817552(__global__TestClass_2901448691360* __instance) {
    print_2901448694560("Hello, World!");
}
// Function: __global::__globalType::__globalFunction, Token: 1
__global__void_2901448674352 __globalFunction_2901448768400() {
if (MRK_STATIC_MEMBER(__global____globalType_2901448690208, AMMAR_2901448699872) > 3)
    {
        MRK_STATIC_MEMBER(__global____globalType_2901448690208, print_2901448694560)("AMMAR is greater than 8");
    }
}
// Static field initializer: __global::__globalType::AMMAR
void staticFieldInit2901448699872() {
    __global____globalType_2901448690208::AMMAR_2901448699872 = 9999    ;
}
// Static field initializer: __global::TestClass::MRKSTA
void staticFieldInit2901448794464() {
    __global__TestClass_2901448691360::MRKSTA_2901448794464 = "hey bbx"    ;
}

// Metadata registration
void registerMetadata() {
    // Register native methods
    MRK_RUNTIME_REGISTER_CODE(1, __globalFunction_2901448768400);
    MRK_RUNTIME_REGISTER_CODE(4, ctor_2901448795568);
    MRK_RUNTIME_REGISTER_CODE(2, print_2901448694560);
    MRK_RUNTIME_REGISTER_CODE(5, dtor_2901448693472);
    MRK_RUNTIME_REGISTER_CODE(3, main_2901448695312);
    MRK_RUNTIME_REGISTER_CODE(6, test_2901448817552);
    // Register types
    MRK_RUNTIME_REGISTER_TYPE(1, __global__void_2901448674352);
    MRK_RUNTIME_REGISTER_TYPE(4, __global__i8_2901448720432);
    MRK_RUNTIME_REGISTER_TYPE(2, __global__bool_2901448674736);
    MRK_RUNTIME_REGISTER_TYPE(3, __global__char_2901448720048);
    MRK_RUNTIME_REGISTER_TYPE(5, __global__byte_2901448675120);
    MRK_RUNTIME_REGISTER_TYPE(6, __global__short_2901448675504);
    MRK_RUNTIME_REGISTER_TYPE(7, __global__ushort_2901448675888);
    MRK_RUNTIME_REGISTER_TYPE(8, __global__int_2901448676272);
    MRK_RUNTIME_REGISTER_TYPE(9, __global__uint_2901448676656);
    MRK_RUNTIME_REGISTER_TYPE(16, __global____globalType_2901448690208);
    MRK_RUNTIME_REGISTER_TYPE(10, __global__long_2901448678128);
    MRK_RUNTIME_REGISTER_TYPE(11, __global__ulong_2901448678512);
    MRK_RUNTIME_REGISTER_TYPE(12, __global__float_2901448763136);
    MRK_RUNTIME_REGISTER_TYPE(18, __global__TestClass__TestStruct_2901448690976);
    MRK_RUNTIME_REGISTER_TYPE(13, __global__double_2901448763520);
    MRK_RUNTIME_REGISTER_TYPE(14, __global__string_2901448768016);
    MRK_RUNTIME_REGISTER_TYPE(15, __global__object_2901448690592);
    MRK_RUNTIME_REGISTER_TYPE(17, __global__TestClass_2901448691360);
    // Register static fields
    MRK_RUNTIME_REGISTER_STATIC_FIELD(1, __global____globalType_2901448690208::AMMAR_2901448699872, staticFieldInit2901448699872);
    MRK_RUNTIME_REGISTER_STATIC_FIELD(3, __global__TestClass_2901448691360::MRKSTA_2901448794464, staticFieldInit2901448794464);
}
MRK_NS_END
