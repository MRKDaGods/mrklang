#pragma once

#include "common/types.h"

MRK_NS_BEGIN_MODULE(runtime::type_system)

class Type;

class Field {
public:
    Field(const Str& name, Type* fieldType, size_t offset)
		: name_(name), fieldType_(fieldType), offset_(offset) {}

    const Str& getName() const { return name_; }
    Type* getFieldType() const { return fieldType_; }
    size_t getOffset() const { return offset_; }

private:
    Str name_;
    Type* fieldType_;
    size_t offset_;
};

MRK_NS_END
