#pragma once

#include "common/types.h"
#include "parser/ast.h"
#include "access_modifier.h"

MRK_NS_BEGIN

using namespace semantic;

using ASTNode = ast::Node;

#define SYMBOL_KINDS \
	X(NAMESPACE) \
	X(VARIABLE) \
	X(FUNCTION) \
	X(FUNCTION_PARAMETER) \
	X(TYPE) \
	X(CLASS) \
	X(STRUCT) \
	X(ENUM) \
	X(ENUM_MEMBER) \
	X(BLOCK)

enum class SymbolKind {
	#define X(x) x,
	SYMBOL_KINDS
	#undef X
};

constexpr std::string_view toString(const SymbolKind& type) {
	switch (type) {
		#define X(x) case SymbolKind::x: return #x;
		SYMBOL_KINDS
		#undef X

		default:
			return "UNKNOWN";
	}
}

#undef SYMBOL_KINDS

struct NamespaceSymbol;
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

	NamespaceSymbol(Str name, bool isImplicit, Symbol* parent, ASTNode* declNode)
		: Symbol(SymbolKind::NAMESPACE, Move(name), parent, declNode), isImplicit(isImplicit) {}
};

struct VariableSymbol : Symbol {
	Str type;

	VariableSymbol(Str name, Str type, Symbol* parent, ASTNode* declNode)
		: Symbol(SymbolKind::VARIABLE, Move(name), parent, declNode), type(Move(type)) {}
};

struct FunctionParameterSymbol : Symbol {
	Str type;
	bool isParams;

	FunctionParameterSymbol(Str name, Str type, bool isParams, Symbol* parent, ASTNode* declNode)
		: Symbol(SymbolKind::FUNCTION_PARAMETER, Move(name), parent, declNode), type(Move(type)), isParams(isParams) {}
};

struct FunctionSymbol : Symbol {
	Str returnType;
	Dict<Str, UniquePtr<FunctionParameterSymbol>> parameters; // name, type

	FunctionSymbol(Str name, Str returnType, Dict<Str, UniquePtr<FunctionParameterSymbol>>&& parameters, Symbol* parent, ASTNode* declNode)
		: Symbol(SymbolKind::FUNCTION, Move(name), parent, declNode), returnType(Move(returnType)), parameters(Move(parameters)) {}
};

struct TypeSymbol : Symbol {
	TypeSymbol(const SymbolKind& kind, Str name, Symbol* parent, ASTNode* declNode)
		: Symbol(kind, Move(name), parent, declNode) {}
};

struct ClassSymbol : TypeSymbol {
	ClassSymbol(Str name, Symbol* parent, ASTNode* declNode)
		: TypeSymbol(SymbolKind::CLASS, Move(name), parent, declNode) {}
};

struct StructSymbol : TypeSymbol {
	StructSymbol(Str name, Symbol* parent, ASTNode* declNode)
		: TypeSymbol(SymbolKind::STRUCT, Move(name), parent, declNode) {}
};

struct EnumSymbol : TypeSymbol {
	EnumSymbol(Str name, Symbol* parent, ASTNode* declNode)
		: TypeSymbol(SymbolKind::ENUM, Move(name), parent, declNode) {}
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

class SymbolTable {
public:
	SymbolTable(Vec<UniquePtr<ast::Program>>&& programs);

	void build();
	void dump() const;
	void dumpSymbol(const Symbol* symbol, int indent) const;
	Str formatAccessModifiers(const Symbol* symbol) const;

	/// Declare a new namespace symbol
	/// @param nsName - unqualified name of the namespace
	/// @param parent - parent namespace
	/// @param declNode - AST node where this namespace is declared
	NamespaceSymbol* declareNamespace(const Str& nsName, NamespaceSymbol* parent = nullptr, ASTNode* declNode = nullptr);

	/// Declare a new file scope namespace (implicit)
	NamespaceSymbol* declareFileScope(const Str& filename);

private:
	Vec<UniquePtr<ast::Program>> programs_;
	Dict<Str, UniquePtr<NamespaceSymbol>> namespaces_;
	Dict<Str, NamespaceSymbol*> fileScopes_;
	NamespaceSymbol* globalNamespace_;
};

MRK_NS_END