#pragma once

#include "common/types.h"
#include "optional"
#include "parser/ast.h"
#include "symbols.h"
#include "type_system.h"

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

	/// The global namespace
	NamespaceSymbol* getGlobalNamespace() const { return globalNamespace_; }
	TypeSystem* getTypeSystem() const { return typeSystem_.get(); }

	/// Determine if an expression is an l-value
	bool isLValue(ast::ExprNode* expr);

	/// Resolve a type from a TypeReferenceExpr
	TypeSymbol* resolveType(ast::TypeReferenceExpr* typeRef, const Symbol* scope);

	void setNodeScope(const ASTNode* node, const Symbol* scope);
	const Symbol* getNodeScope(const ASTNode* node);

	void setNodeResolvedSymbol(const ast::ExprNode* node, Symbol* symbol);
	Symbol* getNodeResolvedSymbol(const ast::ExprNode* node) const;

	/// Resolve a symbol within a scope, its ancestors and imports
	Symbol* resolveSymbol(SymbolKind kind, const Str& symbolText, const Symbol* scope, SymbolResolveFlags flags = SymbolResolveFlags::ALL);

	const Vec<UniquePtr<ast::Program>>& getPrograms() const { return programs_; }
	Vec<TypeSymbol*> getTypes() const { return types_; }
	Vec<VariableSymbol*> getVariables() const { return variables_; }
	Vec<FunctionSymbol*> getFunctions() const { return functions_; }
	TypeSymbol* getGlobalType() const { return globalType_; }
	FunctionSymbol* getGlobalFunction() const { return globalFunction_; }

private:
	Vec<UniquePtr<ast::Program>> programs_;
	Dict<Str, UniquePtr<NamespaceSymbol>> namespaces_;
	Vec<TypeSymbol*> types_;
	Vec<VariableSymbol*> variables_;
	Vec<FunctionSymbol*> functions_;
	NamespaceSymbol* globalNamespace_;
	Dict<const SourceFile*, Vec<ImportEntry>> imports_;
	UniquePtr<TypeSystem> typeSystem_;
	TypeSymbol* globalType_;
	FunctionSymbol* globalFunction_;

	/// Keep track of the scope for each AST node
	Dict<const ASTNode*, const Symbol*> nodeScopes_;

	/// Keep track of resolved symbols for each expr node
	Dict<const ast::ExprNode*, Symbol*> resolvedSymbols_;

	/// Setup global symbols
	void setupGlobals();

	void validateImport(const ImportEntry& entry);
	void validateImports();

	/// Resolve a symbol within a scope and its ancestors
	Symbol* resolveSymbolInternal(SymbolKind kind, const Str& symbolText, const Symbol* scope, const Symbol* requestor = nullptr);
};

MRK_NS_END