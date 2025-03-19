#include "type_system.h"
#include "symbols.h"
#include "symbol_table.h"

MRK_NS_BEGIN_MODULE(semantic)

TypeSystem::TypeSystem(SymbolTable* symbolTable)
	: symbolTable_(symbolTable), errorType_(nullptr), namespaceType_(nullptr) {
	initializeBuiltinTypes();
}

TypeSymbol* TypeSystem::getBuiltinType(TypeKind kind) const {
	auto it = builtinTypes_.find(kind);
	if (it != builtinTypes_.end()) {
		return it->second;
	}

	return errorType_; // TODO: Return null?
}

bool TypeSystem::isPrimitiveType(const TypeSymbol* type, TypeKind* kind) const {
	return std::find_if(builtinTypes_.begin(), builtinTypes_.end(), [type, kind](const auto& pair) {
		if (pair.second == type) {
			if (kind) {
				*kind = pair.first;
			}

			return true;
		}

		return false;
	}) != builtinTypes_.end();
}

bool TypeSystem::isNumericType(const TypeSymbol* type) const {
	TypeKind kind;
	if (!isPrimitiveType(type, &kind)) {
		return false;
	}

	switch (kind) {
		case TypeKind::I8:
		case TypeKind::U8:
		case TypeKind::I16:
		case TypeKind::U16:
		case TypeKind::I32:
		case TypeKind::U32:
		case TypeKind::I64:
		case TypeKind::U64:
		case TypeKind::F32:
		case TypeKind::F64:
			return true;

		default:
			return false;
	}
}

bool TypeSystem::isIntegralType(const TypeSymbol* type) const {
	TypeKind kind;
	if (!isPrimitiveType(type, &kind)) {
		return false;
	}

	switch (kind) {
		case TypeKind::I8:
		case TypeKind::U8:
		case TypeKind::I16:
		case TypeKind::U16:
		case TypeKind::I32:
		case TypeKind::U32:
		case TypeKind::I64:
		case TypeKind::U64:
			return true;

		default:
			return false;
	}
}

size_t TypeSystem::getTypeSize(const TypeSymbol* type) const {
	TypeKind kind;

	if (isPrimitiveType(type, &kind)) {
		switch (kind) {
			case TypeKind::BOOL:
			case TypeKind::CHAR:
			case TypeKind::I8:
			case TypeKind::U8:
				return 1;

			case TypeKind::I16:
			case TypeKind::U16:
				return 2;

			case TypeKind::I32:
			case TypeKind::U32:
			case TypeKind::F32:
				return 4;

			case TypeKind::I64:
			case TypeKind::U64:
			case TypeKind::F64:
			case TypeKind::PTR:
				return 8;

			default:
				return 0; // TODO: Unknown size, but do we return 8 for a pointer?
		}
	}

	return 0;
}

bool TypeSystem::isDerivedFrom(const TypeSymbol* type, const TypeSymbol* baseType) const {
	// Every type is derived from object
	if (baseType == getBuiltinType(TypeKind::OBJECT)) {
		return true;
	}

	const auto& bases = type->resolver.baseTypes;
	return std::find(bases.begin(), bases.end(), baseType) != bases.end();
}

bool TypeSystem::isAssignable(const TypeSymbol* target, const TypeSymbol* source) const {
	if (!target || !source) {
		return false;
	}

	// Same type is always assignable
	if (target == source) {
		return true;
	}

	if (target->isGenericParameter || source->isGenericParameter) {
		return true;
	}

	// Error type is assignable to anything for error recovery
	if (source == errorType_) {
		return true;
	}

	// Check for primitive type conversions
	if (isPrimitiveType(target) && isPrimitiveType(source)) {
		// Allow implicit numeric conversions where there's no data loss
		if (isNumericType(target) && isNumericType(source)) {
			// Allow smaller types to be assigned to larger types
			if (getTypeSize(target) >= getTypeSize(source)) {
				return true;
			}
		}
	}

	// Check inheritance - if source is derived from target
	if (isDerivedFrom(source, target)) {
		return true;
	}

	// if any is ptr, ignore
	TypeKind kind;
	if ((isPrimitiveType(target, &kind) && kind == TypeKind::PTR)) {
		return true;
	}

	return false;
}

const TypeSymbol* TypeSystem::getCommonType(const TypeSymbol* type1, const TypeSymbol* type2) const {
	if (!type1 || !type2) {
		return errorType_;
	}

	// If they're the same type, that's the common type
	if (type1 == type2) {
		return type1;
	}

	// If one is assignable to the other, use that as the common type
	if (isAssignable(type1, type2)) {
		return type1;
	}

	if (isAssignable(type2, type1)) {
		return type2;
	}

	// For primitive numeric types, use the larger type
	if (isNumericType(type1) && isNumericType(type2)) {
		if (getTypeSize(type1) >= getTypeSize(type2)) {
			return type1;
		}
		else {
			return type2;
		}
	}

	// Find common base type
	for (const auto& baseType1 : type1->resolver.baseTypes) {
		for (const auto& baseType2 : type2->resolver.baseTypes) {
			if (baseType1 == baseType2) {
				return baseType1;
			}
		}
	}

	// If no common type found but both are reference types, use object
	if (!isPrimitiveType(type1) && !isPrimitiveType(type2)) {
		return getBuiltinType(TypeKind::OBJECT);
	}

	// No common type found
	return nullptr;
}

const TypeSymbol* TypeSystem::getBinaryExpressionType(TokenType op, const TypeSymbol* left, const TypeSymbol* right) const {
	if (!left || !right) {
		return errorType_;
	}

	// Handle arithmetic operators
	if (op == TokenType::OP_PLUS || op == TokenType::OP_MINUS ||
		op == TokenType::OP_ASTERISK || op == TokenType::OP_SLASH ||
		op == TokenType::OP_MOD) {
		// Special case for string concatenation
		if (op == TokenType::OP_PLUS &&
			(left == getBuiltinType(TypeKind::STRING) || right == getBuiltinType(TypeKind::STRING))) {
			return getBuiltinType(TypeKind::STRING);
		}

		// Numeric operations return the common numeric type
		if (isNumericType(left) && isNumericType(right)) {
			return getCommonType(left, right);
		}
	}

	// Handle comparison operators
	if (op == TokenType::OP_EQ_EQ || op == TokenType::OP_NOT_EQ ||
		op == TokenType::OP_LT || op == TokenType::OP_LE ||
		op == TokenType::OP_GT || op == TokenType::OP_GE) {
		return getBuiltinType(TypeKind::BOOL);
	}

	// Handle logical operators
	if (op == TokenType::OP_AND || op == TokenType::OP_OR) {
		if (left == getBuiltinType(TypeKind::BOOL) && right == getBuiltinType(TypeKind::BOOL)) {
			return getBuiltinType(TypeKind::BOOL);
		}
	}

	// Handle bitwise operators
	if (op == TokenType::OP_BAND || op == TokenType::OP_BOR || op == TokenType::OP_BNOT ||
		op == TokenType::OP_BXOR || op == TokenType::OP_SHL || op == TokenType::OP_SHR) {
		if (isIntegralType(left) && isIntegralType(right)) {
			return getCommonType(left, right);
		}
	}

	// Unknown or invalid operator for these types
	return nullptr;
}

const TypeSymbol* TypeSystem::getUnaryExpressionType(TokenType op, const TypeSymbol* type) const {
	if (!type) {
		return errorType_;
	}

	// Handle logical not
	if (op == TokenType::OP_NOT) {
		if (type == getBuiltinType(TypeKind::BOOL)) {
			return getBuiltinType(TypeKind::BOOL);
		}
	}

	// Handle numeric negation
	if (op == TokenType::OP_MINUS) {
		if (isNumericType(type)) {
			return type;
		}
	}

	// Handle bitwise not
	if (op == TokenType::OP_BNOT) {
		if (isIntegralType(type)) {
			return type;
		}
	}

	// Handle increment/decrement
	if (op == TokenType::OP_INCREMENT || op == TokenType::OP_DECREMENT) {
		if (isNumericType(type)) {
			return type;
		}
	}

	// Unknown or invalid operator for this type
	return nullptr;
}

TypeSymbol* TypeSystem::resolveTypeFromLiteral(ast::LiteralExpr* literalExpr) const {
	const auto& flags = literalExpr->value.flags;

	switch (literalExpr->value.type) {
		case TokenType::LIT_INT:
			if (flags.isLong) {
				return getBuiltinType(flags.isUnsigned ? TypeKind::U64 : TypeKind::I64);
			}

			if (flags.isShort) {
				return getBuiltinType(flags.isUnsigned ? TypeKind::U16 : TypeKind::I16);
			}

			return getBuiltinType(flags.isUnsigned ? TypeKind::U32 : TypeKind::I32);

		case TokenType::LIT_FLOAT:
			// If not explicitlty specified as float, default to double
			return getBuiltinType(flags.isFloat ? TypeKind::F32 : TypeKind::F64);

		case TokenType::LIT_STRING:
			return getBuiltinType(TypeKind::STRING);

		case TokenType::LIT_CHAR:
			return getBuiltinType(TypeKind::CHAR);

		case TokenType::LIT_BOOL:
			return getBuiltinType(TypeKind::BOOL);

		case TokenType::LIT_NULL:
			// Null is compatible with any reference type
			return getBuiltinType(TypeKind::PTR);

		default:
			return errorType_;
	}
}

void TypeSystem::initializeBuiltinTypes() {
	using BaseTypesVec = decltype(TypeSymbol::resolver.baseTypes);
	const auto& globalNamespace = symbolTable_->getGlobalNamespace();

	// Create error type for error recovery
	errorType_ = new TypeSymbol(SymbolKind::TYPE, "Error", {}, globalNamespace, nullptr);
	errorType_->resolver.resolve(BaseTypesVec());

	// Create namespace "type" (not a real type, but used for expression type tracking)
	namespaceType_ = new TypeSymbol(SymbolKind::TYPE, "Namespace", {}, globalNamespace, nullptr);
	namespaceType_->resolver.resolve(BaseTypesVec());

	// Create primitive types
	auto createBuiltinType = [&](TypeKind kind, const Str& name) {
		auto type = MakeUnique<TypeSymbol>(SymbolKind::PRIMITIVE_TYPE, name, Vec<Str>(), globalNamespace, nullptr);
		type->resolver.resolve(BaseTypesVec());

		auto typePtr = type.get();
		builtinTypes_[kind] = typePtr;

		globalNamespace->members[name] = Move(type);

		symbolTable_->addType(typePtr);
		return type;
	};

	createBuiltinType(TypeKind::VOID, "void");
	createBuiltinType(TypeKind::BOOL, "bool");
	createBuiltinType(TypeKind::CHAR, "char");
	createBuiltinType(TypeKind::I8, "i8");
	createBuiltinType(TypeKind::U8, "byte");
	createBuiltinType(TypeKind::I16, "short");
	createBuiltinType(TypeKind::U16, "ushort");
	createBuiltinType(TypeKind::I32, "int");
	createBuiltinType(TypeKind::U32, "uint");
	createBuiltinType(TypeKind::I64, "long");
	createBuiltinType(TypeKind::U64, "ulong");
	createBuiltinType(TypeKind::F32, "float");
	createBuiltinType(TypeKind::F64, "double");
	createBuiltinType(TypeKind::STRING, "string");
	createBuiltinType(TypeKind::OBJECT, "object");
	createBuiltinType(TypeKind::PTR, "void*");
}

MRK_NS_END
