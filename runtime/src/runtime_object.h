#pragma once

#include "common/types.h"
#include "type_system/type.h"
#include "type_system/field.h"

#include <cstdlib>
#include <cstring>

MRK_NS_BEGIN_MODULE(runtime)

using namespace type_system;

class RuntimeObject {
public:
    RuntimeObject(Type* type) : type_(type) {}
    virtual ~RuntimeObject() = default;

    Type* getType() const { return type_; }
    virtual bool isValueType() const = 0;

    template<typename T>
    T* as() { return static_cast<T*>(this); }

    template<typename T>
    const T* as() const { return static_cast<const T*>(this); }

protected:
    Type* type_;
};

/// Runtime object for value types
template<typename T>
class ValueTypeObject : public RuntimeObject {
public:
    ValueTypeObject(Type* type, T value)
        : RuntimeObject(type), value_(value) {}

    bool isValueType() const override { return true; }

    T getValue() const { return value_; }
    void setValue(T value) { value_ = value; }

private:
    T value_;
};

/// Runtime object for reference types
class ReferenceTypeObject : public RuntimeObject {
public:
    ReferenceTypeObject(Type* type) : RuntimeObject(type) {
        if (type && type->getSize() > 0) {
            data_ = std::malloc(type->getSize());
            if (data_) {
                std::memset(data_, 0, type->getSize());
            }
        }
    }

    ~ReferenceTypeObject() override {
        if (data_) {
            std::free(data_);
        }
    }

    bool isValueType() const override { return false; }

    // Get raw pointer to object data
    void* getData() { return data_; }
    const void* getData() const { return data_; }

private:
    void* data_ = nullptr;
};

MRK_NS_END