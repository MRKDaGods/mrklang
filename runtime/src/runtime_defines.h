#pragma once

typedef void (*NativeMethodPtr)(void**, void*);

// Macros for method registration and execution
#define MRK_RUNTIME_REGISTER_CODE(token, method) \
    Runtime::instance().registerNativeMethod(token, reinterpret_cast<void*>(method))

#define MRK_RUNTIME_REGISTER_STATIC_FIELD(token, field, init) \
    Runtime::instance().registerStaticField(token, reinterpret_cast<void*>(&field), reinterpret_cast<void*>(init))

#define MRK_RUNTIME_REGISTER_TYPE(token, type) \
    (void)token; (void)type // Type registration is handled separately

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