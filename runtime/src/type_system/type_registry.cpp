#include "type_registry.h"
#include "type.h"
#include "primitive_type.h"
#include "class_type.h"
#include "array_type.h"
#include "field.h"
#include "method.h"
#include "common/logging.h"
#include "metadata/metadata_loader.h"
#include <map>

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

void TypeRegistry::initializeMetadata(const MetadataRoot* root) {
	if (!root) {
		MRK_ERROR("No metadata types available to build");
		return;
	}

	// Alloc registration
	registration_ = MakeUnique<RuntimeMetadataRegistration, false>();

	MRK_INFO("Initializing metadata types");

	auto& metadataLoader = MetadataLoader::instance();

	// First, register all types
	for (uint32_t i = 0; i < root->typeDefinitionCount; i++) {
		const auto& typeDef = root->typeDefinitions[i];
		registerTypeFromMetadata(typeDef);
	}

	// Then, populate fields and methods after all types are registered
	for (uint32_t i = 0; i < root->typeDefinitionCount; i++) {
		const auto& typeDef = root->typeDefinitions[i];

		// Register fields
		for (uint32_t f = 0; f < typeDef.fieldCount; f++) {
			uint32_t fieldIndex = typeDef.fieldStart + f;
			if (fieldIndex < root->fieldDefinitionCount) {
				registerFieldFromMetadata(typeDef, root->fieldDefinitions[fieldIndex]);
			}
		}

		// Register methods
		for (uint32_t m = 0; m < typeDef.methodCount; m++) {
			uint32_t methodIndex = typeDef.methodStart + m;
			if (methodIndex < root->methodDefinitionCount) {
				registerMethodFromMetadata(typeDef, root->methodDefinitions[methodIndex]);
			}
		}
	}
}

Type* TypeRegistry::registerTypeFromMetadata(const TypeDefinition& typeDef) {
	// Get type name from metadata
	auto& metadataLoader = MetadataLoader::instance();
	const char* typeName = metadataLoader.getString(typeDef.name);
	const char* namespaceName = typeDef.namespaceName ? metadataLoader.getString(typeDef.namespaceName) : "";

	if (!typeName) {
		MRK_ERROR("Invalid type name in metadata");
		return nullptr;
	}

	// Construct full name
	Str fullName = namespaceName && strlen(namespaceName) > 0
		? Str(namespaceName) + "::" + typeName
		: typeName;

	// Check if we already have this type registered
	Type* existingType = findType(fullName);
	if (existingType) {
		// Update token for cross-referencing
		existingType->setToken(typeDef.token);

		registration_->typeTokenMap[existingType] = typeDef.token;
		registration_->typeTokenReverseMap[typeDef.token] = existingType;
		return existingType;
	}

	// Create new type
	Type* newType = nullptr;

	// Determine type kind
	bool isClass = typeDef.flags.isClass != 0;

	if (isClass) {
		// Create class type
		auto* classType = new ClassType(typeName, namespaceName, false, TypeAttributes::CLASS, typeDef.size);

		// Set parent type if available
		if (typeDef.parentHandle > 0) {
			// Find parent type
			auto* parentType = getTypeByToken(typeDef.parentHandle);
			if (parentType) {
				classType->setBaseType(parentType);
			}
		}

		newType = classType;
	}
	else {
		// Create an interface type as fallback
		// Prims go here for now

		#pragma warning(suppress: 6387)
		newType = new ClassType(typeName, namespaceName, false, TypeAttributes::INTERFACE, typeDef.size);
	}

	// Set token and register
	newType->setToken(typeDef.token);
	registerType(newType);

	registration_->typeTokenMap[newType] = typeDef.token;
	registration_->typeTokenReverseMap[typeDef.token] = newType;

	// Check if it's a primitive
	if (typeDef.flags.isPrimitive) {
		regsterPrimitiveType(newType);
	}

	return newType;
}

Field* TypeRegistry::registerFieldFromMetadata(const TypeDefinition& typeDef,
	const FieldDefinition& fieldDef) {
	auto& metadataLoader = MetadataLoader::instance();
	const char* fieldName = metadataLoader.getString(fieldDef.name);

	if (!fieldName) {
		MRK_ERROR("Invalid field name in metadata");
		return nullptr;
	}

	// Find the containing type
	Type* containingType = getTypeByToken(typeDef.token);
	if (!containingType) {
		MRK_ERROR("Could not find containing type for field: {}", fieldName);
		return nullptr;
	}

	// Find the field type
	Type* fieldType = getTypeByToken(fieldDef.typeHandle);
	if (!fieldType) {
		MRK_ERROR("Could not find type for field: {}", fieldName);
		return nullptr;
	}

	// Create field
	ClassType* classType = dynamic_cast<ClassType*>(containingType);
	if (!classType) {
		MRK_ERROR("Field's containing type is not a class type");
		return nullptr;
	}

	// Create and add the field
	Field* field = new Field(fieldName, fieldType, 0, fieldDef.flags.STATIC);
	classType->addField(field);

	// Register field
	registration_->fieldTokenMap[field] = fieldDef.token;
	registration_->fieldTokenReverseMap[fieldDef.token] = field;

	return field;
}

Method* TypeRegistry::registerMethodFromMetadata(const TypeDefinition& typeDef,
	const MethodDefinition& methodDef) {
	auto& metadataLoader = MetadataLoader::instance();
	const char* methodName = metadataLoader.getString(methodDef.name);

	if (!methodName) {
		MRK_ERROR("Invalid method name in metadata");
		return nullptr;
	}

	// Find the containing type
	Type* containingType = getTypeByToken(typeDef.token);
	if (!containingType) {
		MRK_ERROR("Could not find containing type for method: {}", methodName);
		return nullptr;
	}

	// Find the return type
	Type* returnType = getTypeByToken(methodDef.returnTypeHandle);
	if (!returnType) {
		MRK_ERROR("Could not find return type for method: {}", methodName);
		return nullptr;
	}

	// Create method
	ClassType* classType = dynamic_cast<ClassType*>(containingType);
	if (!classType) {
		MRK_ERROR("Method's containing type is not a class type");
		return nullptr;
	}

	// Params
	Vec<Parameter> parameters;
	const auto* root = metadataLoader.getMetadataRoot();

	// Add parameters from metadata
	for (uint32_t i = 0; i < methodDef.parameterCount; i++) {
		uint32_t paramIndex = methodDef.parameterStart + i;
		if (paramIndex >= root->parameterDefinitionCount) {
			MRK_ERROR("Parameter index out of bounds: {}", paramIndex);
			continue;
		}

		const auto& paramDef = root->parameterDefinitions[paramIndex];
		const char* paramName = metadataLoader.getString(paramDef.name);

		if (!paramName) {
			MRK_ERROR("Invalid parameter name in metadata");
			continue;
		}

		// Find parameter type
		Type* paramType = getTypeByToken(paramDef.typeHandle);
		if (!paramType) {
			MRK_ERROR("Could not find type for parameter: {}", paramName);
			continue;
		}

		// Add parameter
		parameters.emplace_back(paramName, paramType, paramDef.flags);
	}

	// Create and add the method
	Method* method = new Method(methodName, returnType, containingType, methodDef.flags, Move(parameters));
	classType->addMethod(method);

	// Register method
	registration_->methodTokenMap[method] = methodDef.token;
	registration_->methodTokenReverseMap[methodDef.token] = method;

	return method;
}

void TypeRegistry::initializeBuiltinTypes() {
	// Consume from metadata instead

	/*voidType_ = new PrimitiveType(TypeKind::VOID, "void", 0);
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
	registerType(objectType_);*/
}

void TypeRegistry::dumpTree() const {
	MRK_INFO("Type Registry Dump:");
	MRK_INFO("==========================================");

	// Create a map to organize types by namespace
	std::map<Str, std::vector<Type*>> typesByNamespace;

	// Group types by namespace
	for (const auto& [name, type] : types_) {
		Str ns;
		if (ClassType* classType = dynamic_cast<ClassType*>(type)) {
			if (!classType->getNamespaceName().empty()) {
				ns = classType->getNamespaceName();
			}
		}
		else {
		 // For non-class types, use "System" namespace as a fallback
			ns = MRK_STL_NAME;
		}

		typesByNamespace[ns].push_back(type);
	}

	// Print types by namespace
	for (const auto& [ns, types] : typesByNamespace) {
		MRK_INFO("Namespace: {}", ns);
		MRK_INFO("------------------------------------------");

		for (Type* type : types) {
			dumpType(type, 0);
		}

		MRK_INFO("------------------------------------------");
	}

	MRK_INFO("Total types: {}", types_.size());
	MRK_INFO("==========================================");
}

void TypeRegistry::dumpType(Type* type, int indent) const {
	if (!type) return;

	// Create indentation string
	Str indentStr(indent * 2, ' ');

	// Print type info
	Str typeKindStr;
	switch (type->getTypeKind()) {
		case TypeKind::CLASS: typeKindStr = "class"; break;
		//case TypeKind::STRUCT: typeKindStr = "struct"; break;
	   // case TypeKind::INTERFACE: typeKindStr = "interface"; break;
		case TypeKind::ARRAY: typeKindStr = "array"; break;
		//case TypeKind::ENUM: typeKindStr = "enum"; break;
		//case TypeKind::PRIMITIVE: typeKindStr = "primitive"; break;
		case TypeKind::VOID: typeKindStr = "void"; break;
		default: typeKindStr = "unknown";
	}

	MRK_INFO("{}+ {} {} (size: {})", indentStr, typeKindStr, type->getFullName(), type->getSize());

	// For class types, print inheritance, fields and methods
	if (ClassType* classType = dynamic_cast<ClassType*>(type)) {
		// Print base type
		if (Type* baseType = classType->getBaseType()) {
			MRK_INFO("{}  Inherits from: {}", indentStr, baseType->getFullName());
		}

		// Print fields
		const auto& fields = classType->getFields();
		if (!fields.empty()) {
			MRK_INFO("{}  Fields:", indentStr);
			for (const Field* field : fields) {
				MRK_INFO("{}    {} {} (offset: {})", indentStr,
					field->getFieldType()->getFullName(),
					field->getName(),
					field->getOffset());
			}
		}

		// Print methods
		const auto& methods = classType->getMethods();
		if (!methods.empty()) {
			MRK_INFO("{}  Methods:", indentStr);
			for (const Method* method : methods) {
				Str signature = method->getReturnType()->getFullName() + " " + method->getName() + "(";

				const auto& params = method->getParameters();
				for (size_t i = 0; i < params.size(); ++i) {
					if (i > 0) signature += ", ";
					signature += params[i].getType()->getFullName() + " " + params[i].getName();
				}
				signature += ")";

				MRK_INFO("{}    {}", indentStr, signature);
			}
		}
	}
	// For array types, print element type
	else if (ArrayType* arrayType = dynamic_cast<ArrayType*>(type)) {
		MRK_INFO("{}  Element Type: {}", indentStr, arrayType->getElementType()->getFullName());
		MRK_INFO("{}  Rank: {}", indentStr, arrayType->getRank());
	}
}

Type* TypeRegistry::getTypeByToken(uint32_t token) const {
	auto it = registration_->typeTokenReverseMap.find(token);
	return it != registration_->typeTokenReverseMap.end() ? it->second : nullptr;
}

Field* TypeRegistry::getFieldByToken(uint32_t token) const {
	auto it = registration_->fieldTokenReverseMap.find(token);
	return it != registration_->fieldTokenReverseMap.end() ? it->second : nullptr;
}

Method* TypeRegistry::getMethodByToken(uint32_t token) const {
	auto it = registration_->methodTokenReverseMap.find(token);
	return it != registration_->methodTokenReverseMap.end() ? it->second : nullptr;
}

Field* TypeRegistry::getField(FieldDefinition* fieldDef) {
	return getFieldByToken(fieldDef->token);
}

void TypeRegistry::regsterPrimitiveType(Type* type) {
	auto name = type->getName();

	if (name == "void") {
		voidType_ = type;
	}
	else if (name == "bool") {
		boolType_ = type;
	}
	else if (name == "char") {
		charType_ = type;
	}
	else if (name == "byte") {
		byteType_ = type;
	}
	else if (name == "short") {
		int16Type_ = type;
	}
	else if (name == "ushort") {
		uint16Type_ = type;
	}
	else if (name == "int32") {
		int32Type_ = type;
	}
	else if (name == "uint") {
		uint32Type_ = type;
	}
	else if (name == "int64") {
		int64Type_ = type;
	}
	else if (name == "ulong") {
		uint64Type_ = type;
	}
	else if (name == "float") {
		floatType_ = type;
	}
	else if (name == "double") {
		doubleType_ = type;
	}
	else if (name == "string") {
		stringType_ = type;
	}
	else if (name == "object") {
		objectType_ = type;
	}
}

MRK_NS_END