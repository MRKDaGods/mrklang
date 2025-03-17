#pragma once

#include "common/types.h"
#include "type.h"

MRK_NS_BEGIN_MODULE(runtime::type_system)

class Parameter {
public: // TODO: Add param flags
	Parameter(const Str& name, Type* type, uint32_t flags = 0)
		: name_(name), type_(type), flags_(flags) {}
	
	const Str& getName() const { return name_; }
	Type* getType() const { return type_; }
	uint32_t getFlags() const { return flags_; }

private:
	Str name_;
	Type* type_;
	uint32_t flags_;
};

MRK_NS_END