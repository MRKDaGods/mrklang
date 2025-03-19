#pragma once

#include "../metadata/metadata_structures.h"

#ifndef MRK_COMPILER

MRK_NS_BEGIN_MODULE(runtime::api)

using namespace metadata;

// Object
void* mrk_create_instance(const TypeDefinition* type);
void mrk_destroy_instance(void* instance);

// Type
const TypeDefinition* mrk_get_type(const Str& fullName);

// Field
const FieldDefinition* mrk_get_field(const TypeDefinition* type, const Str& name);
void* mrk_get_field_value(const FieldDefinition* field, void* instance);
void mrk_set_field_value(const FieldDefinition* field, void* instance, void* value);

MRK_NS_END

#endif