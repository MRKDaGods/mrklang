#include "symbol_collector.h"
#include "symbol_table.h"
#include "common/utils.h"

#include <algorithm>

MRK_NS_BEGIN

SymbolCollector::SymbolCollector(SymbolTable* symbolTable)
	: symbolTable_(symbolTable), currentNamespace_(nullptr), currentScope_(nullptr), currentModifiers_(AccessModifier::NONE) {}

/// Visit a Program node - entry point for processing a file
void SymbolCollector::visit(Program* node) {
	// Update namespace
	currentNamespace_ = symbolTable_->declareFileScope(node->filename);
	currentScope_ = currentNamespace_;
	currentFile_ = node->filename;

	// Reset modifiers at the start of a file
	resetModifiers();

	// Push global scope
	pushScope(currentNamespace_);

	for (const auto& stmt : node->statements) {
		stmt->accept(*this);
	}

	// Pop global scope
	popScope();
}

void SymbolCollector::visit(LiteralExpr* node) {
	// None
}

void SymbolCollector::visit(InterpolatedStringExpr* node) {
	for (const auto& part : node->parts) {
		part->accept(*this);
	}
}

void SymbolCollector::visit(InteropCallExpr* node) {
	// TODO: impl
}

void SymbolCollector::visit(IdentifierExpr* node) {
	// None
}

void SymbolCollector::visit(TypeReferenceExpr* node) {
	// None
}

void SymbolCollector::visit(CallExpr* node) {
	if (node->target) {
		node->target->accept(*this);
	}

	for (const auto& arg : node->arguments) {
		arg->accept(*this);
	}
}

void SymbolCollector::visit(BinaryExpr* node) {
	node->left->accept(*this);
	node->right->accept(*this);
}

void SymbolCollector::visit(UnaryExpr* node) {
	node->right->accept(*this);
}

void SymbolCollector::visit(TernaryExpr* node) {
	node->condition->accept(*this);
	node->thenBranch->accept(*this);
	node->elseBranch->accept(*this);
}

void SymbolCollector::visit(AssignmentExpr* node) {
	node->target->accept(*this);

	if (node->value) {
		node->value->accept(*this);
	}
}

void SymbolCollector::visit(NamespaceAccessExpr* node) {
	for (const auto& part : node->path) {
		part->accept(*this);
	}
}

void SymbolCollector::visit(MemberAccessExpr* node) {
	node->target->accept(*this);
}

void SymbolCollector::visit(ArrayExpr* node) {
	for (const auto& elem : node->elements) {
		elem->accept(*this);
	}
}

void SymbolCollector::visit(ExprStmt* node) {
	node->expr->accept(*this);
}

void SymbolCollector::visit(VarDeclStmt* node) {
	auto typeName = node->typeName ? node->typeName->getTypeName() : "object";
	auto varName = node->name->name;

	auto varSymbol = MakeUnique<VariableSymbol>(
		varName,
		typeName,
		currentScope_,
		node
	);

	// Add modifiers
	varSymbol->accessModifier = currentModifiers_;
	varSymbol->declSpec = currentDeclSpec_;
	resetModifiers();

	// Add to current scope
	currentScope_->members[varName] = Move(varSymbol);

	// Check initializer
	if (node->initializer) {
		node->initializer->accept(*this);
	}
}

void SymbolCollector::visit(BlockStmt* node) {
	auto blockName = "block_" + std::to_string(reinterpret_cast<uintptr_t>(node));
	auto blockSymbol = MakeUnique<BlockSymbol>(Move(blockName), currentScope_, node);
	
	// Add modifiers
	blockSymbol->accessModifier = currentModifiers_;
	blockSymbol->declSpec = currentDeclSpec_;
	resetModifiers();

	BlockSymbol* blockPtr = blockSymbol.get();
	currentScope_->members[blockPtr->name] = Move(blockSymbol);

	// Push block scope
	pushScope(blockPtr);

	for (const auto& stmt : node->statements) {
		stmt->accept(*this);
	}

	// Pop block scope
	popScope();

	// Reset again incase? Im lost lmao
	resetModifiers();
}

void SymbolCollector::visit(ParamDeclStmt* node) {
	// Processed by FuncDeclStmt
}

void SymbolCollector::visit(FuncDeclStmt* node) {
	// Collect parameters
	Dict<Str, UniquePtr<FunctionParameterSymbol>> params;
	for (const auto& param : node->parameters) {
		auto paramSymbol = MakeUnique<FunctionParameterSymbol>(
			param->name->name,
			param->type->getTypeName(),
			param->isParams,
			nullptr,
			param.get()
		);

		params[param->name->name] = Move(paramSymbol);
	}

	auto funcSymbol = MakeUnique<FunctionSymbol>(
		node->getSignature(),
		node->returnType ? node->returnType->getTypeName() : "void",
		Move(params),
		currentScope_,
		node
	);

	// Add modifiers
	funcSymbol->accessModifier = currentModifiers_;
	funcSymbol->declSpec = currentDeclSpec_;
	resetModifiers();

	auto funcPtr = funcSymbol.get();

	// update params parent
	for (auto& param : funcPtr->parameters) {
		param.second->parent = funcPtr;
	}

	currentScope_->members[node->getSignature()] = Move(funcSymbol);

	// Push function scope
	pushScope(funcPtr);

	// Process function body
	node->body->accept(*this);

	// Pop function scope
	popScope();
}

void SymbolCollector::visit(IfStmt* node) {
	node->condition->accept(*this);
	node->thenBlock->accept(*this);

	if (node->elseBlock) {
		node->elseBlock->accept(*this);
	}
}

void SymbolCollector::visit(ForStmt* node) {
	if (node->init) {
		node->init->accept(*this);
	}
	if (node->condition) {
		node->condition->accept(*this);
	}
	if (node->increment) {
		node->increment->accept(*this);
	}

	node->body->accept(*this);
}

void SymbolCollector::visit(ForeachStmt* node) {
	node->variable->accept(*this);
	node->collection->accept(*this);
	node->body->accept(*this);
}

void SymbolCollector::visit(WhileStmt* node) {
	node->condition->accept(*this);
	node->body->accept(*this);
}

void SymbolCollector::visit(LangBlockStmt* node) {
	// Untracked
}

void SymbolCollector::visit(AccessModifierStmt* node) {
	// To be consumed by next declaration
	AccessModifier modifiers = AccessModifier::NONE;
	for (const auto& token : node->modifiers) {
		modifiers = modifiers | detail::parseAccessModifier(token.lexeme);
	}

	currentModifiers_ = modifiers;
}


void SymbolCollector::visit(NamespaceDeclStmt* node) {
	// Save current namespace
	auto prevNamespace = currentNamespace_;

	// Declare new namespace and mark as current
	auto nsLocalName = formatCollection(node->path, "::", [](const auto& item) { return item->name; });
	currentNamespace_ = symbolTable_->declareNamespace(nsLocalName, currentNamespace_, node);

	// Namespaces may have declspec
	currentNamespace_->declSpec = currentDeclSpec_;
	resetModifiers();

	// Push namespace scope
	pushScope(currentNamespace_);

	// Process namespace body
	node->body->accept(*this);

	// Pop namespace scope
	popScope();

	// Restore previous namespace
	currentNamespace_ = prevNamespace;

	// Reset modifiers again?
	// TODO: Block should already reset modifiers, this is redundant?
	resetModifiers();
}

void SymbolCollector::visit(DeclSpecStmt* node) {
	currentDeclSpec_ = node->spec->name;
}

void SymbolCollector::visit(UseStmt* node) {
	// Do we add import symbols?
}

void SymbolCollector::visit(ReturnStmt* node) {
	if (node->value) {
		node->value->accept(*this);
	}
}

void SymbolCollector::visit(EnumDeclStmt* node) {
	auto enumSymbol = MakeUnique<EnumSymbol>(
		node->name->name,
		currentScope_,
		node
	);

	// Add modifiers
	enumSymbol->accessModifier = currentModifiers_;
	enumSymbol->declSpec = currentDeclSpec_;
	resetModifiers();

	// Resolve enum members
	for (const auto& member : node->members) {
		auto memberName = member.first->name;

		// TODO: Resolve member value at compile time
		auto memberValue = member.second ? member.second->toString() : "null";
		auto memberSymbol = MakeUnique<EnumMemberSymbol>(
			memberName,
			Move(memberValue),
			enumSymbol.get(),
			member.first.get());

		enumSymbol->members[Move(memberName)] = Move(memberSymbol);
	}

	currentScope_->members[node->name->name] = Move(enumSymbol);
}

void SymbolCollector::visit(TypeDeclStmt* node) {
	UniquePtr<Symbol> typeSymbol;
	Symbol* typePtr = nullptr;

	if (node->type.lexeme == "class") {
		typeSymbol = MakeUnique<ClassSymbol>(
			node->name->getTypeName(),
			currentScope_,
			node
		);

		typePtr = typeSymbol.get();
	}
	else if (node->type.lexeme == "struct") {
		typeSymbol = MakeUnique<StructSymbol>(
			node->name->getTypeName(),
			currentScope_,
			node
		);

		typePtr = typeSymbol.get();
	}
	else { // shouldnt happen
		throw std::runtime_error("Invalid type declaration");
	}

	// Add modifiers
	typePtr->accessModifier = currentModifiers_;
	typePtr->declSpec = currentDeclSpec_;
	resetModifiers();

	// Add to current scope
	currentScope_->members[node->name->getTypeName()] = Move(typeSymbol);

	// Push type scope
	pushScope(typePtr);

	// Process type body
	node->body->accept(*this);

	// Pop type scope
	popScope();
}

void SymbolCollector::pushScope(Symbol* scope) {
	scopeStack_.push(scope);
	currentScope_ = scope;
}

void SymbolCollector::popScope() {
	scopeStack_.pop();
	currentScope_ = scopeStack_.empty() ? nullptr : scopeStack_.top();
}

void SymbolCollector::resetModifiers() {
	currentModifiers_ = AccessModifier::NONE;
	currentDeclSpec_ = "";
}

MRK_NS_END