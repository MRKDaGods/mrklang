#pragma once

#include "common/types.h"
#include "semantic/symbol_table.h"

MRK_NS_BEGIN_MODULE(codegen)

using namespace ast;
using namespace semantic;

class CodeGenerator;

/// Generates C++ code from the symbol table
class FunctionGenerator : public ast::ASTVisitor {
public:
	FunctionGenerator(CodeGenerator* cppGen, const SymbolTable* symbolTable);

	void generateFunctionBody(const FunctionSymbol* function);
	void generateFieldInitializer(const VariableSymbol* field, const TypeSymbol* enclosingType);

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
	void visit(ArrayAccessExpr* node) override;

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
	CodeGenerator* cppGen_;
	const SymbolTable* symbolTable_;
	const FunctionSymbol* currentFunction_;
	const TypeSymbol* currentFunctionEnclosingType_;
	bool isGlobalFunction_;

	void generateGlobalFunctionBody(const FunctionSymbol* function);
};

MRK_NS_END