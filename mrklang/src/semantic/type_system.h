#pragma once

#include "common/types.h"
#include "parser/ast.h"

// Mini type system to represent types in the runtime

MRK_NS_BEGIN_MODULE(semantic)

struct TypeSymbol;
class SymbolTable;

// From runtime::type_system::TypeKind
enum class TypeKind {
	VOID,
	BOOL,
	CHAR,
	I8,
	U8,
	I16,
	U16,
	I32,
	U32,
	I64,
	U64,
	F32,
	F64,
	PTR,
	BYREF,
	VALUE_TYPE,
	CLASS,
	SZ_ARRAY, // Single-dimensional, zero-based array
	ARRAY, // Multi-dimensional, zero-based array
	TYPE_PARAMETER,
	METHOD_TYPE_PARAMETER,

	// These are supposedly custom types?
	// Compiler specific
	STRING,
	OBJECT
};

class TypeSystem {
public:
	TypeSystem(SymbolTable* symbolTable);
	TypeSymbol* getBuiltinType(TypeKind kind) const;
	TypeSymbol* getErrorType() const { return errorType_; }
	TypeSymbol* getNamespaceType() const { return namespaceType_; }

	bool isPrimitiveType(const TypeSymbol* type, TypeKind* kind = nullptr) const;
	bool isNumericType(const TypeSymbol* type) const;
	bool isIntegralType(const TypeSymbol* type) const;

	size_t getTypeSize(const TypeSymbol* type) const;

	bool isDerivedFrom(const TypeSymbol* type, const TypeSymbol* baseType) const;
	bool isAssignable(const TypeSymbol* target, const TypeSymbol* source) const;

	/// Get the common type between two types (for conditional expressions)
	const TypeSymbol* getCommonType(const TypeSymbol* type1, const TypeSymbol* type2) const;

	/// Get the result type of a binary expression
	const TypeSymbol* getBinaryExpressionType(TokenType op, const TypeSymbol* left, const TypeSymbol* right) const;

	/// Get the result type of a unary expression
	const TypeSymbol* getUnaryExpressionType(TokenType op, const TypeSymbol* type) const;

	/// Literal types are always valid
	TypeSymbol* resolveTypeFromLiteral(ast::LiteralExpr* literalExpr) const;

private:
	SymbolTable* symbolTable_;
	Dict<TypeKind, TypeSymbol*> builtinTypes_;
	TypeSymbol* errorType_;
	TypeSymbol* namespaceType_;

	void initializeBuiltinTypes();
};

MRK_NS_END
