#include "function_generator.h"
#include "code_generator.h"

MRK_NS_BEGIN_MODULE(codegen)

FunctionGenerator::FunctionGenerator(CodeGenerator* cppGen, const SymbolTable* symbolTable)
	: cppGen_(cppGen), symbolTable_(symbolTable), isGlobalFunction_(false),
	currentFunction_(nullptr), currentFunctionEnclosingType_(nullptr) {}

void FunctionGenerator::generateFunctionBody(const FunctionSymbol* function) {
	currentFunction_ = function;
	currentFunctionEnclosingType_ = static_cast<const TypeSymbol*>(symbolTable_->findAncestorOfKind(function, SymbolKind::TYPE));

	if (function == symbolTable_->getGlobalFunction()) {
		generateGlobalFunctionBody(function);
		return;
	}

	auto astNode = static_cast<const FuncDeclStmt*>(function->declNode);
	// We already printed our braces in the function declaration
	for (const auto& stmt : astNode->body->statements) {
		// Indent the statement
		cppGen_->write<true>("");
		stmt->accept(*this);
	}
}

void FunctionGenerator::generateFieldInitializer(const VariableSymbol* field, const TypeSymbol* enclosingType) {
	if (!field || !enclosingType) {
		return;
	}

	auto* varNode = static_cast<const VarDeclStmt*>(field->declNode);
	if (!varNode->initializer) {
		// No initializer, return default
		cppGen_->write<true>("return ");
		cppGen_->write(cppGen_->getReferenceTypeName(field->resolver.type), "()");
		cppGen_->writeLine(';');
		return;
	}

	cppGen_->write<true>("return ");
	varNode->initializer->accept(*this);
	cppGen_->writeLine(';');
}

void FunctionGenerator::generateGlobalFunctionBody(const FunctionSymbol* function) {
	isGlobalFunction_ = true;

	for (const auto& program : symbolTable_->getPrograms()) {
		for (const auto& stmt : program->statements) {
			// Indent the statement
			//cppGen_->write<true>("");
			stmt->accept(*this);
		}
	}
}

void FunctionGenerator::visit(Program* node) {
	// None
}

void FunctionGenerator::visit(LiteralExpr* node) {
	// Write the literal value
	if (node->value.type == TokenType::LIT_STRING) {
		cppGen_->write('"', node->value.lexeme, '"');
	}
	else if (node->value.type == TokenType::LIT_CHAR) {
		cppGen_->write('\'', node->value.lexeme, '\'');
	}
	else if (node->value.type == TokenType::LIT_NULL) {
		cppGen_->write("__mrk_null");
	}
	else {
		cppGen_->write(node->value.lexeme);
	}
}

void FunctionGenerator::visit(InterpolatedStringExpr* node) {
	// Write the interpolated string
	// TODO: Implement
}

void FunctionGenerator::visit(InteropCallExpr* node) {
	// None
}

void FunctionGenerator::visit(IdentifierExpr* node) {
	auto sym = symbolTable_->getNodeResolvedSymbol(node);
	if (sym) {
		// Check if it's a member
		bool isMember = currentFunctionEnclosingType_ == symbolTable_->findAncestorOfKind(sym, SymbolKind::TYPE)
			&& currentFunctionEnclosingType_->getMember(sym->name);
		if (isMember) {
			// Static or instance?
			if (detail::isSTATIC(sym->accessModifier)) {
				cppGen_->write("MRK_STATIC_MEMBER(", cppGen_->getMappedName(currentFunctionEnclosingType_), ", ");
			}
			else {
				cppGen_->write("MRK_INSTANCE_MEMBER(");
			}
		}

		// Write the identifier
		cppGen_->write(cppGen_->getMappedName(sym));

		if (isMember) {
			cppGen_->write(')');
		}

		return;
	}

	// Write the identifier
	cppGen_->write(node->name);
}

void FunctionGenerator::visit(TypeReferenceExpr* node) {
	auto sym = symbolTable_->getNodeResolvedSymbol(node);
	if (!sym) {
		cppGen_->write("ERROR");
		return;
	}

	cppGen_->write(cppGen_->getMappedName(static_cast<TypeSymbol*>(sym)), ' ');
}

void FunctionGenerator::visit(CallExpr* node) {
	// Write the target
	node->target->accept(*this);

	cppGen_->write('(');

	// Write arguments
	for (int i = 0; i < node->arguments.size(); i++) {
		node->arguments[i]->accept(*this);

		if (i < node->arguments.size() - 1) {
			cppGen_->write(", ");
		}
	}

	cppGen_->write(')');
}

void FunctionGenerator::visit(BinaryExpr* node) {
	// Write the left side
	node->left->accept(*this);

	// Write the operator
	cppGen_->write(' ', node->op.lexeme, ' ');

	// Write the right side
	node->right->accept(*this);
}

void FunctionGenerator::visit(UnaryExpr* node) {
	// Write the operator
	cppGen_->write(node->op.lexeme);

	// Write the operand
	node->right->accept(*this);
}

void FunctionGenerator::visit(TernaryExpr* node) {
	// Write the condition
	node->condition->accept(*this);

	// Write the then branch
	cppGen_->write(" ? ");
	node->thenBranch->accept(*this);

	// Write the else branch
	cppGen_->write(" : ");
	node->elseBranch->accept(*this);
}

void FunctionGenerator::visit(AssignmentExpr* node) {
	// Write the target
	node->target->accept(*this);

	// Write the operator
	cppGen_->write(' ', node->op.lexeme, ' ');

	// Write the value
	node->value->accept(*this);
}

void FunctionGenerator::visit(NamespaceAccessExpr* node) {
	// Write the path
	for (int i = 0; i < node->path.size(); i++) {
		node->path[i]->accept(*this);
		if (i < node->path.size() - 1) {
			cppGen_->write("::");
		}
	}
}

void FunctionGenerator::visit(MemberAccessExpr* node) {
	// Write the target
	node->target->accept(*this);

	// Write the operator
	cppGen_->write(node->op.lexeme);

	// Write the member
	node->member->accept(*this);
}

void FunctionGenerator::visit(ArrayExpr* node) {
	// Write the array
	cppGen_->write('{');
	for (int i = 0; i < node->elements.size(); i++) {
		node->elements[i]->accept(*this);

		if (i < node->elements.size() - 1) {
			cppGen_->write(", ");
		}
	}

	cppGen_->write('}');
}

void FunctionGenerator::visit(ArrayAccessExpr* node) {
	// Write the target
	node->target->accept(*this);

	// Write the index
	cppGen_->write('[');
	node->index->accept(*this);
	cppGen_->write(']');
}

void FunctionGenerator::visit(ExprStmt* node) {
	// Write the expression
	node->expr->accept(*this);

	// End the statement
	cppGen_->writeLine<false>(';');
}

void FunctionGenerator::visit(VarDeclStmt* node) {
	if (isGlobalFunction_) return;

	// Write the type
	node->typeName->accept(*this);

	// Write the name
	cppGen_->write(' ');
	node->name->accept(*this);

	// Write the nativeInitializerMethod
	if (node->initializer) {
		cppGen_->write(" = ");
		node->initializer->accept(*this);
	}

	// End the statement
	cppGen_->writeLine<false>(';');
}

void FunctionGenerator::visit(BlockStmt* node) {
	// Write the block
	cppGen_->writeLine('{');

	// Write the statements
	cppGen_->indent();

	for (const auto& stmt : node->statements) {
		// Indent the statement
		cppGen_->write<true>("");
		stmt->accept(*this);
	}

	cppGen_->unindent();

	// End the block
	cppGen_->writeLine('}');
}

void FunctionGenerator::visit(ParamDeclStmt* node) {
	// None
}

void FunctionGenerator::visit(FuncDeclStmt* node) {
	// None
}

void FunctionGenerator::visit(IfStmt* node) {
	// Write the condition
	cppGen_->write("if (");
	node->condition->accept(*this);
	cppGen_->writeLine<false>(")");

	// Write the then block
	node->thenBlock->accept(*this);

	// Write the else block
	if (node->elseBlock) {
		cppGen_->writeLine("else");
		node->elseBlock->accept(*this);
	}
}

void FunctionGenerator::visit(ForStmt* node) {
	// Write the nativeInitializerMethod
	if (node->init) {
		node->init->accept(*this);
	}

	// Write the condition
	cppGen_->write("for (");
	if (node->condition) {
		node->condition->accept(*this);
	}

	cppGen_->write("; ");
	// Write the increment
	if (node->increment) {
		node->increment->accept(*this);
	}

	cppGen_->writeLine(")");

	// Write the body
	node->body->accept(*this);
}

void FunctionGenerator::visit(ForeachStmt* node) {
	// foreach (x in y) -> for (auto x : y)
	cppGen_->write("for (auto ");
	node->variable->accept(*this);

	cppGen_->write(" : ");
	node->collection->accept(*this);
	cppGen_->writeLine(")");

	// Write the body
	node->body->accept(*this);
}

void FunctionGenerator::visit(WhileStmt* node) {
	// Write the condition
	cppGen_->write("while (");
	node->condition->accept(*this);
	cppGen_->writeLine(")");

	// Write the body
	node->body->accept(*this);
}

void FunctionGenerator::visit(LangBlockStmt* node) {
	// For now, keep cpp blocks as is
	if (node->language == "__cpp") {
		const auto& blocks = symbolTable_->getRigidLanguageBlocks();
		if (blocks.find(node) == blocks.end()) { // Not a rigid block
			cppGen_->writeLine(node->rawCode);
		}
	}
}

void FunctionGenerator::visit(AccessModifierStmt* node) {
	// Write the modifiers
	for (const auto& mod : node->modifiers) {
		cppGen_->write(mod.lexeme, ' ');
	}
}

void FunctionGenerator::visit(NamespaceDeclStmt* node) {
	// None
}

void FunctionGenerator::visit(DeclSpecStmt* node) {
	// None
}

void FunctionGenerator::visit(UseStmt* node) {
	// None
}

void FunctionGenerator::visit(ReturnStmt* node) {
	// Write the return statement
	cppGen_->write("return ");
	if (node->value) {
		node->value->accept(*this);
	}

	cppGen_->writeLine<false>(';');
}

void FunctionGenerator::visit(EnumDeclStmt* node) {
	// None
}

void FunctionGenerator::visit(TypeDeclStmt* node) {
	// None
}

MRK_NS_END
