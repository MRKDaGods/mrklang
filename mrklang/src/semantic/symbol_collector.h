#pragma once

#include "common/types.h"
#include "parser/ast.h"
#include "access_modifier.h"

#include <stack>

MRK_NS_BEGIN

class SymbolTable;
class Symbol;
class NamespaceSymbol;

using namespace semantic;
using namespace ast;

class SymbolCollector : public ASTVisitor {
public:
	SymbolCollector(SymbolTable* symbolTable);

	void visit(Program* node) override;

	void visit(LiteralExpr* node) override;
	void visit(InterpolatedStringExpr* node) override;
	void visit(InteropCallExpr* node) override;
	void visit(IdentifierExpr* node) override;
	void visit(TypeReferenceExpr* node) override;
	void visit(CallExpr* node) override;
	void visit(BinaryExpr* node) override;
	void visit(UnaryExpr* node) override;
	void visit(TernaryExpr* node) override;
	void visit(AssignmentExpr* node) override;
	void visit(NamespaceAccessExpr* node) override;
	void visit(MemberAccessExpr* node) override;
	void visit(ArrayExpr* node) override;

	void visit(ExprStmt* node) override;
	void visit(VarDeclStmt* node) override;
	void visit(BlockStmt* node) override;
	void visit(ParamDeclStmt* node) override;
	void visit(FuncDeclStmt* node) override;
	void visit(IfStmt* node) override;
	void visit(ForStmt* node) override;
	void visit(ForeachStmt* node) override;
	void visit(WhileStmt* node) override;
	void visit(LangBlockStmt* node) override;
	void visit(AccessModifierStmt* node) override;
	void visit(NamespaceDeclStmt* node) override;
	void visit(DeclSpecStmt* node) override;
	void visit(UseStmt* node) override;
	void visit(ReturnStmt* node) override;
	void visit(EnumDeclStmt* node) override;
	void visit(TypeDeclStmt* node) override;

private:
	SymbolTable* symbolTable_;
	Symbol* currentScope_;
	NamespaceSymbol* currentNamespace_;
	Str currentFile_;
	AccessModifier currentModifiers_;
	Str currentDeclSpec_;
	std::stack<Symbol*> scopeStack_;

	void pushScope(Symbol* scope);
	void popScope();
	void resetModifiers();
};

MRK_NS_END