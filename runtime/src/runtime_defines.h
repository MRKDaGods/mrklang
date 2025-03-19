#pragma once

#include <cstdint>
#include <string>
#include <iostream>

typedef void (*NativeMethodPtr)(void**, void*);

template<typename T>
constexpr size_t MRK_SIZE_OF() {
	if constexpr (std::is_void_v<T>) {
		return 0;
	}
	else {
		return sizeof(T);
	}
}

// Macros for method registration and execution
#define MRK_RUNTIME_REGISTER_CODE(token, method) \
    Runtime::instance().registerNativeMethod(token, reinterpret_cast<void*>(method))

#define MRK_RUNTIME_REGISTER_STATIC_FIELD(token, field, init) \
    Runtime::instance().registerStaticField(token, reinterpret_cast<void*>(&field), reinterpret_cast<void*>(init))

#define MRK_RUNTIME_REGISTER_FIELD(token, offset) Runtime::instance().registerField(token, offset)

#define MRK_RUNTIME_REGISTER_TYPE(token, type) Runtime::instance().registerType(token, MRK_SIZE_OF<type>())

// Helper macros for method calls
#define MRK_CALL_METHOD(methodToken, instance, args, result) \
    Runtime::instance().executeMethod(methodToken, instance, args, result)

// Static vs instance member access
#define MRK_STATIC_MEMBER(typeName, member) typeName::member
#define MRK_INSTANCE_MEMBER(member) __instance->member

// Runtime object creation helpers
#define MRK_BOX_BOOL(value) \
    new ValueTypeObject<bool>(TypeRegistry::instance().getBoolType(), value)

#define MRK_BOX_CHAR(value) \
    new ValueTypeObject<char>(TypeRegistry::instance().getCharType(), value)

#define MRK_BOX_BYTE(value) \
    new ValueTypeObject<uint8_t>(TypeRegistry::instance().getByteType(), value)

#define MRK_BOX_INT16(value) \
    new ValueTypeObject<int16_t>(TypeRegistry::instance().getInt16Type(), value)

#define MRK_BOX_INT32(value) \
    new ValueTypeObject<int32_t>(TypeRegistry::instance().getInt32Type(), value)

#define MRK_BOX_INT64(value) \
    new ValueTypeObject<int64_t>(TypeRegistry::instance().getInt64Type(), value)

#define MRK_BOX_FLOAT(value) \
    new ValueTypeObject<float>(TypeRegistry::instance().getFloatType(), value)

#define MRK_BOX_DOUBLE(value) \
    new ValueTypeObject<double>(TypeRegistry::instance().getDoubleType(), value)

#define MRK_BOX_STRING(value) \
    new StringObject(TypeRegistry::instance().getStringType(), value)

// Unboxing helpers
#define MRK_UNBOX_BOOL(obj) \
    static_cast<ValueTypeObject<bool>*>(obj)->getValue()

#define MRK_UNBOX_CHAR(obj) \
    static_cast<ValueTypeObject<char>*>(obj)->getValue()

#define MRK_UNBOX_BYTE(obj) \
    static_cast<ValueTypeObject<uint8_t>*>(obj)->getValue()

#define MRK_UNBOX_INT16(obj) \
    static_cast<ValueTypeObject<int16_t>*>(obj)->getValue()

#define MRK_UNBOX_INT32(obj) \
    static_cast<ValueTypeObject<int32_t>*>(obj)->getValue()

#define MRK_UNBOX_INT64(obj) \
    static_cast<ValueTypeObject<int64_t>*>(obj)->getValue()

#define MRK_UNBOX_FLOAT(obj) \
    static_cast<ValueTypeObject<float>*>(obj)->getValue()

#define MRK_UNBOX_DOUBLE(obj) \
    static_cast<ValueTypeObject<double>*>(obj)->getValue()

#define MRK_UNBOX_STRING(obj) \
    static_cast<StringObject*>(obj)->getValue()

// Reference type helpers
#define MRK_GET_REFERENCE_DATA(obj) \
    static_cast<ReferenceTypeObject*>(obj)->getData()

#define MRK_CREATE_REFERENCE_OBJECT(type) \
    new ReferenceTypeObject(type)

#define MRK_INVOKE_ICALL(token, ...) \
    Runtime::instance().invokeInternalCall(token __VA_OPT__(,) __VA_ARGS__)

using __mrkprimitive_void = void;
using __mrkprimitive_bool = bool;
using __mrkprimitive_char = char;
using __mrkprimitive_i8 = int8_t;
using __mrkprimitive_byte = uint8_t;
using __mrkprimitive_short = int16_t;
using __mrkprimitive_ushort = uint16_t;
using __mrkprimitive_int = int32_t;
using __mrkprimitive_uint = uint32_t;
using __mrkprimitive_long = int64_t;
using __mrkprimitive_ulong = uint64_t;
using __mrkprimitive_float = float;
using __mrkprimitive_double = double;
using __mrkprimitive_string = std::string;
using __mrkprimitive_object = void*;

#define __mrk_null nullptr