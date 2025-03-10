#pragma once

#include "common/types.h"
#include "optional"
#include "parser/ast.h"
#include "symbols.h"

MRK_NS_BEGIN_MODULE(semantic)

struct ImportEntry {
	Str path;
	Str file;
	const ASTNode* node; // Keep track of the node where this import was declared
};

enum class SymbolResolveFlags : uint32_t {
	NONE = 0,
	ANCESTORS = 1 << 0,
	IMPORTS = 1 << 1,
	ALL = ANCESTORS | IMPORTS
};

IMPLEMENT_FLAGS_OPERATORS_INLINE(SymbolResolveFlags);

class SymbolTable {
public:
	SymbolTable() = default;
	SymbolTable(Vec<UniquePtr<ast::Program>>&& programs);
	void build();
	void resolve();
	void dump() const;
	void dumpSymbol(const Symbol* symbol, int indent) const;
	Str formatAccessModifiers(const Symbol* symbol) const;

	/// Declare a new namespace symbol
	/// @param nsName - unqualified name of the namespace
	/// @param parent - parent namespace
	/// @param declNode - AST node where this namespace is declared
	NamespaceSymbol* declareNamespace(const Str& nsName, NamespaceSymbol* parent = nullptr, ASTNode* declNode = nullptr);

	void addType(TypeSymbol* type);
	void addVariable(VariableSymbol* variable);
	void addFunction(FunctionSymbol* function);
	void addImport(const SourceFile* file, ImportEntry&& entry);
	void error(const ASTNode* node, const Str& message);

	/// Find the first non-implicit parent of a symbol
	/// Implicit symbols are either file scopes or blocks
	const Symbol* findFirstNonImplicitParent(const Symbol* symbol, bool includeMe = false) const;

	/// Find the nearest ancestor of a symbol of a specific kind
	Symbol* findAncestorOfKind(const Symbol* symbol, SymbolKind kind) const;

	NamespaceSymbol* getGlobalNamespace() const;

private:
	Vec<UniquePtr<ast::Program>> programs_;
	Dict<Str, UniquePtr<NamespaceSymbol>> namespaces_;
	Vec<TypeSymbol*> types_;
	Vec<VariableSymbol*> variables_;
	Vec<FunctionSymbol*> functions_;
	NamespaceSymbol* globalNamespace_;
	Dict<const SourceFile*, Vec<ImportEntry>> imports_;

	void validateImport(const ImportEntry& entry);
	void validateImports();

	/// Resolve a symbol within a scope, its ancestors and imports
	Symbol* resolveSymbol(SymbolKind kind, const Str& symbolText, const Symbol* scope, SymbolResolveFlags flags = SymbolResolveFlags::ALL);

	/// Resolve a symbol within a scope and its ancestors
	Symbol* resolveSymbolInternal(SymbolKind kind, const Str& symbolText, const Symbol* scope);
};

MRK_NS_END