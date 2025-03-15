#include "type_registry.h"
#include "type.h"
#include "primitive_type.h"
#include "class_type.h"
#include "array_type.h"

MRK_NS_BEGIN_MODULE(runtime::type_system)

TypeRegistry& TypeRegistry::instance() {
	static TypeRegistry instance;
	return instance;
}

TypeRegistry::~TypeRegistry() {
	for (auto& [name, type] : types_) {
		delete type;
	}

	types_.clear();

	voidType_ = nullptr;
	boolType_ = nullptr;
	charType_ = nullptr;
	byteType_ = nullptr;
	int16Type_ = nullptr;
	int32Type_ = nullptr;
	int64Type_ = nullptr;
	floatType_ = nullptr;
	doubleType_ = nullptr;
	stringType_ = nullptr;
	objectType_ = nullptr;
}

void TypeRegistry::registerType(Type* type) {
	types_[type->getFullName()] = type;
}

Type* TypeRegistry::createArrayType(Type* elementType, uint32_t rank) {
	Str arrayName = elementType->getFullName();
	if (rank == 1) {
		arrayName += "[]";
	}
	else {
		arrayName += "[";
		for (uint32_t i = 1; i < rank; i++) {
			arrayName += ",";
		}
		arrayName += "]";
	}

	// Check if the type already exists
	Type* existingType = findType(arrayName);
	if (existingType != nullptr) {
		return existingType;
	}

	// Create a new array type
	Type* arrayType = new ArrayType(elementType, rank);
	registerType(arrayType);
	return arrayType;
}

Type* TypeRegistry::findType(const Str& fullName) const {
	auto it = types_.find(fullName);
	return it != types_.end() ? it->second : nullptr;
}

void TypeRegistry::initializeBuiltinTypes() {
	voidType_ = new PrimitiveType(TypeKind::VOID, "void", 0);
	registerType(voidType_);

	boolType_ = new PrimitiveType(TypeKind::BOOL, "bool", 1);
	registerType(boolType_);
	
	charType_ = new PrimitiveType(TypeKind::CHAR, "char", 1);
	registerType(charType_);
	
	byteType_ = new PrimitiveType(TypeKind::U8, "byte", 1);
	registerType(byteType_);
	
	int16Type_ = new PrimitiveType(TypeKind::I16, "int16", 2);
	registerType(int16Type_);
	
	int32Type_ = new PrimitiveType(TypeKind::I32, "int32", 4);
	registerType(int32Type_);
	
	int64Type_ = new PrimitiveType(TypeKind::I64, "int64", 8);
	registerType(int64Type_);
	
	floatType_ = new PrimitiveType(TypeKind::F32, "float", 4);
	registerType(floatType_);
	
	doubleType_ = new PrimitiveType(TypeKind::F64, "double", 8);
	registerType(doubleType_);
	
	stringType_ = new ClassType("string", MRK_STL_NAME, false, TypeAttributes::PUBLIC, sizeof(void*));
	registerType(stringType_);

	objectType_ = new ClassType("object", MRK_STL_NAME, false, TypeAttributes::PUBLIC, sizeof(void*));
	registerType(objectType_);
}

MRK_NS_END