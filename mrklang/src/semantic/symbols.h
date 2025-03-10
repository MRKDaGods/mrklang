#pragma once

#include "common/types.h"
#include "parser/ast.h"
#include "access_modifier.h"

MRK_NS_BEGIN_MODULE(semantic)

#define DECLARE_RESOLVABLE_MEMBERS(...) struct { \
    bool isResolved = false; \
    __VA_ARGS__ \
	\
	template<typename... Args> \
	void resolve(Args... args) { \
		size_t offset = sizeof(bool); \
		\
		auto f = [this, &offset](auto&& x) { \
			using argType = std::decay_t<decltype(x)>; \
			\
			/* Get the real address of the first member */ \
			/* since "this" is not the actual address of the struct */ \
			auto realBase = (uintptr_t)&this->isResolved; \
			\
			/* Calculate proper alignment */ \
			constexpr size_t alignment = alignof(argType); \
			offset = (offset + alignment - 1) & ~(alignment - 1); /* Round up to alignment*/ \
			\
			*(argType*)(realBase + offset) = x; \
			offset += sizeof(argType); \
		}; \
		\
		(f(args), ...); \
		\
		isResolved = true; \
	} \
} resolver = {}


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

struct Symbol {
	Str name; // unqualified name
	Str qualifiedName;
	Symbol* parent;
	ASTNode* declNode;
	Dict<Str, UniquePtr<Symbol>> members;
	AccessModifier accessModifier;
	Str declSpec; // any additional declaration specs
	const SymbolKind kind;

	Symbol(SymbolKind kind, Str&& name, Symbol* parent, ASTNode* declNode)
		: kind(kind), name(Move(name)), parent(parent), declNode(declNode), accessModifier(AccessModifier::NONE) {
		qualifiedName = (parent ? (parent->qualifiedName + "::" + this->name) : this->name);
	}

	virtual Str toString() const { return qualifiedName; }
	virtual Symbol* getMember(const Str& name) const {
		auto it = members.find(name);
		return it != members.end() ? it->second.get() : nullptr;
	}
};

struct NamespaceSymbol : Symbol {
	Dict<Str, NamespaceSymbol*> namespaces; // Local name to ptr

	NamespaceSymbol(Str name, Symbol* parent, ASTNode* declNode)
		: Symbol(SymbolKind::NAMESPACE, Move(name), parent, declNode) {}

	virtual Symbol* getMember(const Str& name) const override {
		auto nsIt = namespaces.find(name);
		if (nsIt != namespaces.end()) {
			return nsIt->second;
		}

		return Symbol::getMember(name);
	}
};

struct VariableSymbol : Symbol {
	Str type;

	DECLARE_RESOLVABLE_MEMBERS(const TypeSymbol* type;);

	VariableSymbol(Str name, Str type, Symbol* parent, ASTNode* declNode)
		: Symbol(SymbolKind::VARIABLE, Move(name), parent, declNode), type(Move(type)) {}
};

struct FunctionParameterSymbol : Symbol {
	Str type;
	bool isParams;

	DECLARE_RESOLVABLE_MEMBERS(const TypeSymbol* type;);

	FunctionParameterSymbol(Str name, Str type, bool isParams, Symbol* parent, ASTNode* declNode)
		: Symbol(SymbolKind::FUNCTION_PARAMETER, Move(name), parent, declNode), type(Move(type)), isParams(isParams) {}
};

struct FunctionSymbol : Symbol {
	Str returnType;
	Dict<Str, UniquePtr<FunctionParameterSymbol>> parameters; // name, type

	DECLARE_RESOLVABLE_MEMBERS(const TypeSymbol* returnType;);

	FunctionSymbol(Str name, Str returnType, Dict<Str, UniquePtr<FunctionParameterSymbol>>&& parameters, Symbol* parent, ASTNode* declNode)
		: Symbol(SymbolKind::FUNCTION, Move(name), parent, declNode), returnType(Move(returnType)), parameters(Move(parameters)) {}
};

struct TypeSymbol : Symbol {
	Vec<Str> baseTypes;

	DECLARE_RESOLVABLE_MEMBERS(Vec<const TypeSymbol*> baseTypes;);

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