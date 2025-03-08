#pragma once

#include "common/types.h"
#include "parser/ast.h"
#include "symbols.h"
#include "optional"

MRK_NS_BEGIN_MODULE(semantic)

struct ImportEntry {
	Str path;
	Str file;
};

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

	/// Declare a new file scope namespace (implicit)
	NamespaceSymbol* declareFileScope(const Str& filename);
	void addType(TypeSymbol* type);
	void addFunction(FunctionSymbol* function);
	void addImport(const SourceFile* file, ImportEntry&& entry);
	void error(const ASTNode* node, const Str& message);

	/// Find the first non-implicit parent of a symbol
	/// Implicit symbols are either file scopes or blocks
	Symbol* findFirstNonImplicitParent(const Symbol* symbol, bool includeMe = false) const;

	/// Find the nearest ancestor of a symbol of a specific kind
	Symbol* findAncestorOfKind(const Symbol* symbol, const SymbolKind& kind) const;

	NamespaceSymbol* getGlobalNamespace() const;

private:
	Vec<UniquePtr<ast::Program>> programs_;
	Dict<Str, UniquePtr<NamespaceSymbol>> namespaces_;
	Dict<Str, NamespaceSymbolDescriptor> namespaceDescriptors_;
	Dict<Str, NamespaceSymbol*> fileScopes_;
	Vec<TypeSymbol*> types_;
	Vec<FunctionSymbol*> functions_;
	NamespaceSymbol* globalNamespace_;
	Dict<const SourceFile*, Vec<ImportEntry>> imports_;

	Str getNonImplicitSymbolName(const Symbol* symbol) const;
	TypeSymbol* findTypeSymbol(const SourceFile* fromFile, const Str& symbolText, const Symbol* currentScope = nullptr);
	TypeSymbol* checkCurrentScopeHierarchy(const Symbol* scope, const Str& name) const;
	TypeSymbol* checkQualifiedPath(const Vec<Str>& components) const;
	TypeSymbol* checkFileScope(const SourceFile* fromFile, const Str& symbolText) const;
	TypeSymbol* checkImports(const SourceFile* fromFile, const Str& symbolText) const;
	TypeSymbol* checkGlobalNamespace(const Str& symbolText) const;
	TypeSymbol* lookupInSymbol(const Symbol* context, const Str& name) const;
	NamespaceSymbol* resolveNamespaceComponent(const Symbol* context, const Str& component) const;
	TypeSymbol* resolveTypeComponent(const Symbol* context, const Str& component) const;
};

MRK_NS_END