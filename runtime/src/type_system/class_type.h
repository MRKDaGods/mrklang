#pragma once

#include "type.h"

MRK_NS_BEGIN_MODULE(runtime::type_system)

class Field;
class Method;

// Represents a class/struct type
class ClassType : public Type {
public:
	ClassType(const Str& name, const Str& namespaceName, bool isValueType, TypeAttributes attributes, size_t size)
		: name_(name), namespaceName_(namespaceName), isValueType_(isValueType), attributes_(attributes), 
		  size_(size), baseType_(nullptr) {}

	Str getName() const override { return name_; }
	Str getFullName() const override { return namespaceName_.empty() ? name_ : namespaceName_ + "::" + name_; }
	Str getNamespaceName() const { return namespaceName_; }
	TypeKind getTypeKind() const override { return isValueType_ ? TypeKind::VALUE_TYPE : TypeKind::CLASS; }
	size_t getSize() const override { return size_; }
	bool isValueType() const override { return isValueType_; }
	bool isClass() const override { return !isValueType_; }
	bool isPrimitive() const override { return false; }
	bool isArray() const override { return false; }

	void setSize(size_t size) { size_ = size; }

	void addField(Field* field) { fields_.push_back(field); }
	void addMethod(Method* method) { methods_.push_back(method); }

	const Vec<Field*>& getFields() const { return fields_; }
	const Vec<Method*>& getMethods() const { return methods_; }

	Type* getBaseType() const { return baseType_; }
	void setBaseType(Type* baseType) { baseType_ = baseType; }

	template<std::invocable<const Method&> Predicate>
	auto findMethods(Predicate&& pred) const {
		Vec<Method*> result;
		for (const auto& method : methods_) {
			if (pred(*method)) {
				result.push_back(method);
			}
		}

		return result;
	}

private:
	Str name_;
	Str namespaceName_;
	bool isValueType_;
	TypeAttributes attributes_;
	size_t size_;
	Type* baseType_;
	Vec<Field*> fields_;
	Vec<Method*> methods_;
};

MRK_NS_END