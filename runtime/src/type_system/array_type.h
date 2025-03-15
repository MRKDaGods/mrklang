#pragma once

#include "type.h"

MRK_NS_BEGIN_MODULE(runtime::type_system)

// Represents an array type
class ArrayType : public Type {
public:
	ArrayType(Type* elementType, uint32_t rank)
		: elementType_(elementType), rank_(rank) {}

	Str getName() const override {
		Str elementName = elementType_->getName();
		if (rank_ == 1) {
			return elementName + "[]";
		}

		Str result = elementName + "[";
		for (uint32_t i = 1; i < rank_; i++) {
			result += ",";
		}

		result += "]";
		return result;
	}

	Str getFullName() const override {
		Str elementName = elementType_->getFullName();
		if (rank_ == 1) {
			return elementName + "[]";
		}

		Str result = elementName + "[";
		for (uint32_t i = 1; i < rank_; i++) {
			result += ",";
		}

		result += "]";
		return result;
	}

	TypeKind getTypeKind() const override { return rank_ == 1 ? TypeKind::SZ_ARRAY : TypeKind::ARRAY; }
	size_t getSize() const override { return sizeof(void*); } // Arrays are reference types
	bool isValueType() const override { return false; }
	bool isClass() const override { return true; }
	bool isPrimitive() const override { return false; }
	bool isArray() const override { return true; }

	Type* getElementType() const { return elementType_; }
	int32_t getRank() const { return rank_; }

private:
	Type* elementType_;
	uint32_t rank_;
};

MRK_NS_END