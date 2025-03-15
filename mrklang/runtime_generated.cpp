#include "runtime.h"
MRK_NS_BEGIN_MODULE(runtime::generated)

// Type: __global::void
struct __global__void_2552654099712 {
};
// Type: __global::bool
struct __global__bool_2552654100096 {
};
// Type: __global::char
struct __global__char_2552654100480 {
};
// Type: __global::i8
struct __global__i8_2552654069200 {
};
// Type: __global::byte
struct __global__byte_2552654069584 {
};
// Type: __global::short
struct __global__short_2552654100864 {
};
// Type: __global::ushort
struct __global__ushort_2552654101248 {
};
// Type: __global::int
struct __global__int_2552654101632 {
};
// Type: __global::uint
struct __global__uint_2552654102016 {
};
// Type: __global::long
struct __global__long_2552654104576 {
};
// Type: __global::ulong
struct __global__ulong_2552654104960 {
};
// Type: __global::float
struct __global__float_2552654105344 {
};
// Type: __global::double
struct __global__double_2552654105728 {
};
// Type: __global::string
struct __global__string_2552654106112 {
};
// Type: __global::object
struct __global__object_2552654045392 {
};
// Type: __global::__globalType
struct __global____globalType_2552654046160 {
    // Variable: AMMAR
    __global__int_2552654101632 AMMAR_2552654117664;
};
// Type: __global::TestClass
struct __global__TestClass_2552654043472 {
    // Variable: y
    __global__int_2552654101632 y_2552654131632;
    // Variable: pi
    __global__double_2552654105728 pi_2552654132000;
};
// Type: __global::TestClass::TestStruct
struct __global__TestClass__TestStruct_2552654045776 {
    // Variable: x
    __global__int_2552654101632 x_2552654123056;
};
// Function: __global::__globalType::print
static __global__void_2552654099712 print_2552654116160(__global__string_2552654106112 msg_2552654114704) {
    msg_2552654114704 += "123";
}
// Function: __global::__globalType::main
static __global__void_2552654099712 main_2552654116912() {
    print_2552654116160("Hello, World!");
}
// Function: __global::TestClass::ctor
__global__void_2552654099712 ctor_2552654132368(__global__TestClass_2552654043472* instance) {
    print_2552654116160("Hello, World!");
    if (pi_2552654132000 > 3.0)
    {
        print_2552654116160("Pi is greater than 3.0");
    }
    else
    {
        print_2552654116160("Pi is less than or equal to 3.0");
    }
}
// Function: __global::TestClass::dtor
__global__void_2552654099712 dtor_2552654137840(__global__TestClass_2552654043472* instance) {
    print_2552654116160("Goodbye, World!");
}
// Function: __global::TestClass::test
__global__void_2552654099712 test_2552654115072(__global__TestClass_2552654043472* instance) {
    print_2552654116160("Hello, World!");
}
// Function: __global::__globalType::__globalFunction
static __global__void_2552654099712 __globalFunction_2552654046736() {
if (AMMAR_2552654117664 > 8)
    {
        print_2552654116160("AMMAR is greater than 8");
    }
}
MRK_NS_END
