#pragma once

#include "common/types.h"

MRK_NS_BEGIN_MODULE(runtime::type_system)

class Type;

class Field {
public:
	Field(const Str& name, Type* fieldType, size_t offset, bool isStatic)
		: name_(name), fieldType_(fieldType), offset_(offset), isStatic_(isStatic),
		staticInit_(nullptr), initialized_(false), nativeField_(nullptr) {}

	const Str& getName() const { return name_; }
	Type* getFieldType() const { return fieldType_; }
	size_t getOffset() const { return offset_; }
	void setOffset(size_t offset) { offset_ = offset; }

	bool isStatic() const { return isStatic_; }
	void setStaticInit(void* staticInit) { staticInit_ = staticInit; }

	void setNativeField(void* nativeField) { nativeField_ = nativeField; }

	void* getValue(void* instance = nullptr) {
		if (isStatic_) {
			//if (!initialized_) {
			//	initialized_ = true;

			//	// Call static init
			//	if (staticInit_) {
			//		((void(*)())staticInit_)();
			//	}
			//}

			return nativeField_;
		}

		if (instance) {
			return (char*)instance + offset_;
		}

		return nullptr;
	}

	void setValue(void* value, void* instance = nullptr) {
		if (isStatic_) {
			nativeField_ = value;
		}
		else if (instance) {
			// Handle strings separately
			if (fieldType_ == TypeRegistry::instance().getStringType()) {
				*(Str*)((char*)instance + offset_) = *(Str*)value;
			}
			else {
				memcpy((char*)instance + offset_, value, fieldType_->getSize());
			}
		}
	}

private:
	Str name_;
	Type* fieldType_;
	size_t offset_;
	bool isStatic_;
	void* staticInit_;
	bool initialized_;

	void* nativeField_;
};

MRK_NS_END
