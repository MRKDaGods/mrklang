#pragma once

#include "common/types.h"
#include "parser/ast.h"
#include "access_modifier.h"

MRK_NS_BEGIN_MODULE(semantic)

#define DECLARE_RESOLVED_MEMBERS(...) struct { \
	bool isResolved; \
	##__VA_ARGS__ \
} resolved = {}

using ASTNode = ast::Node;

struct NamespaceSymbol;
struct TypeSymbol;

#define SYMBOL_KINDS \
	X(NAMESPACE, 0) \
	X(VARIABLE, 1) \
	X(FUNCTION, 2) \
	X(FUNCTION_PARAMETER, 3) \
	X(CLASS, 4) \
	X(STRUCT, 5) \
	X(INTERFACE, 6) \
	X(ENUM, 7) \
	X(ENUM_MEMBER, 8) \
	X(BLOCK, 9)

enum class SymbolKind : uint32_t {
	NONE = 0,
	#define X(x, y) x = 1 << y,
	SYMBOL_KINDS
	#undef X

	TYPE = CLASS | STRUCT | INTERFACE | ENUM
};

IMPLEMENT_FLAGS_OPERATORS_INLINE(SymbolKind);

constexpr std::string_view toString(const SymbolKind& type) {
	switch (type) {
		#define X(x, y) case SymbolKind::x: return #x;
		SYMBOL_KINDS
			#undef X

		default:
			return "UNKNOWN";
	}
}

#undef SYMBOL_KINDS

struct SemanticError : std::runtime_error {
	const ASTNode* node;

	SemanticError(const ASTNode* node, Str message)
		: std::runtime_error(Move(message)), node(node) {}
};

struct NamespaceSymbolDescriptor {
	Str name;
	Vec<const NamespaceSymbol*> variants; // One namespace can have multiple variants in different files
	bool isImplicit;

	void addVariant(const NamespaceSymbol* variant);
};

struct Symbol {
	Str name; // unqualified name
	Str qualifiedName;
	Symbol* parent;
	ASTNode* declNode;
	Dict<Str, UniquePtr<Symbol>> members;
	AccessModifier accessModifier;
	Str declSpec; // any additional declaration specs
	const SymbolKind kind;

	Symbol(const SymbolKind& kind, Str&& name, Symbol* parent, ASTNode* declNode)
		: kind(kind), name(Move(name)), parent(parent), declNode(declNode), accessModifier(AccessModifier::NONE) {
		qualifiedName = (parent ? (parent->qualifiedName + "::" + this->name) : this->name);
	}

	virtual Str toString() const { return qualifiedName; }
};

struct NamespaceSymbol : Symbol {
	bool isImplicit; // true if this is a file scope
	Dict<Str, NamespaceSymbolDescriptor> namespaces;
	Str nonImplicitQualifiedName;

	NamespaceSymbol(Str name, bool isImplicit, Symbol* parent, ASTNode* declNode)
		: Symbol(SymbolKind::NAMESPACE, Move(name), parent, declNode), isImplicit(isImplicit) {
		nonImplicitQualifiedName = qualifiedName;
	}
};

struct VariableSymbol : Symbol {
	Str type;

	DECLARE_RESOLVED_MEMBERS(TypeSymbol* type;);

	VariableSymbol(Str name, Str type, Symbol* parent, ASTNode* declNode)
		: Symbol(SymbolKind::VARIABLE, Move(name), parent, declNode), type(Move(type)) {}
};

struct FunctionParameterSymbol : Symbol {
	Str type;
	bool isParams;

	DECLARE_RESOLVED_MEMBERS(TypeSymbol* type;);

	FunctionParameterSymbol(Str name, Str type, bool isParams, Symbol* parent, ASTNode* declNode)
		: Symbol(SymbolKind::FUNCTION_PARAMETER, Move(name), parent, declNode), type(Move(type)), isParams(isParams) {}
};

struct FunctionSymbol : Symbol {
	Str returnType;
	Dict<Str, UniquePtr<FunctionParameterSymbol>> parameters; // name, type

	DECLARE_RESOLVED_MEMBERS(TypeSymbol* returnType;);

	FunctionSymbol(Str name, Str returnType, Dict<Str, UniquePtr<FunctionParameterSymbol>>&& parameters, Symbol* parent, ASTNode* declNode)
		: Symbol(SymbolKind::FUNCTION, Move(name), parent, declNode), returnType(Move(returnType)), parameters(Move(parameters)) {}
};

struct TypeSymbol : Symbol {
	Vec<Str> baseTypes;

	DECLARE_RESOLVED_MEMBERS(Vec<Str> baseTypes;);

	TypeSymbol(const SymbolKind& kind, Str name, Vec<Str>&& baseTypes, Symbol* parent, ASTNode* declNode)
		: Symbol(kind, Move(name), parent, declNode), baseTypes(Move(baseTypes)) {}
};

struct ClassSymbol : TypeSymbol {
	ClassSymbol(Str name, Vec<Str>&& baseTypes, Symbol* parent, ASTNode* declNode)
		: TypeSymbol(SymbolKind::CLASS, Move(name), Move(baseTypes), parent, declNode) {}
};

struct StructSymbol : TypeSymbol {
	StructSymbol(Str name, Vec<Str>&& baseTypes, Symbol* parent, ASTNode* declNode)
		: TypeSymbol(SymbolKind::STRUCT, Move(name), Move(baseTypes), parent, declNode) {}
};

struct InterfaceSymbol : TypeSymbol {
	InterfaceSymbol(Str name, Vec<Str>&& baseTypes, Symbol* parent, ASTNode* declNode)
		: TypeSymbol(SymbolKind::INTERFACE, Move(name), Move(baseTypes), parent, declNode) {}
};

struct EnumSymbol : TypeSymbol {
	EnumSymbol(Str name, Vec<Str>&& baseTypes, Symbol* parent, ASTNode* declNode)
		: TypeSymbol(SymbolKind::ENUM, Move(name), Move(baseTypes), parent, declNode) {}
};

struct EnumMemberSymbol : Symbol {
	Str value;

	EnumMemberSymbol(Str name, Str value, Symbol* parent, ASTNode* declNode)
		: Symbol(SymbolKind::ENUM_MEMBER, Move(name), parent, declNode), value(Move(value)) {}
};

struct BlockSymbol : Symbol {
	BlockSymbol(Str name, Symbol* parent, ASTNode* declNode)
		: Symbol(SymbolKind::BLOCK, Move(name), parent, declNode) {}
};

MRK_NS_END