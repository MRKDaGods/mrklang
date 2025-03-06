#include "symbol_collector.h"
#include "symbol_table.h"
#include "common/utils.h"

#include <algorithm>

MRK_NS_BEGIN_MODULE(semantic)

SymbolCollector::SymbolCollector(SymbolTable* symbolTable)
	: symbolTable_(symbolTable), currentNamespace_(nullptr), currentScope_(nullptr), currentModifiers_(AccessModifier::NONE) {}

/// Visit a Program node - entry point for processing a file
void SymbolCollector::visit(Program* node) {
	// Update namespace
	currentNamespace_ = symbolTable_->declareFileScope(node->sourceFile->filename);
	currentScope_ = currentNamespace_;
	currentFile_ = node->sourceFile;

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
	bindSourceFile(node);

	// None
}

void SymbolCollector::visit(InterpolatedStringExpr* node) {
	bindSourceFile(node);

	for (const auto& part : node->parts) {
		part->accept(*this);
	}
}

void SymbolCollector::visit(InteropCallExpr* node) {
	bindSourceFile(node);

	// TODO: impl
}

void SymbolCollector::visit(IdentifierExpr* node) {
	bindSourceFile(node);

	// None
}

void SymbolCollector::visit(TypeReferenceExpr* node) {
	bindSourceFile(node);

	// None
}

void SymbolCollector::visit(CallExpr* node) {
	bindSourceFile(node);

	if (node->target) {
		node->target->accept(*this);
	}

	for (const auto& arg : node->arguments) {
		arg->accept(*this);
	}
}

void SymbolCollector::visit(BinaryExpr* node) {
	bindSourceFile(node);

	node->left->accept(*this);
	node->right->accept(*this);
}

void SymbolCollector::visit(UnaryExpr* node) {
	bindSourceFile(node);

	node->right->accept(*this);
}

void SymbolCollector::visit(TernaryExpr* node) {
	bindSourceFile(node);

	node->condition->accept(*this);
	node->thenBranch->accept(*this);
	node->elseBranch->accept(*this);
}

void SymbolCollector::visit(AssignmentExpr* node) {
	bindSourceFile(node);

	node->target->accept(*this);

	if (node->value) {
		node->value->accept(*this);
	}
}

void SymbolCollector::visit(NamespaceAccessExpr* node) {
	bindSourceFile(node);

	for (const auto& part : node->path) {
		part->accept(*this);
	}
}

void SymbolCollector::visit(MemberAccessExpr* node) {
	bindSourceFile(node);

	node->target->accept(*this);
}

void SymbolCollector::visit(ArrayExpr* node) {
	bindSourceFile(node);

	for (const auto& elem : node->elements) {
		elem->accept(*this);
	}
}

void SymbolCollector::visit(ExprStmt* node) {
	bindSourceFile(node);

	node->expr->accept(*this);
}

void SymbolCollector::visit(VarDeclStmt* node) {
	bindSourceFile(node);

	// Check for const initialization
	if (detail::isCONST(currentModifiers_) && !node->initializer) {
		symbolTable_->error(node, "Const variable must be initialized");
		resetModifiers();
		return;
	}

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
	bindSourceFile(node);

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
	bindSourceFile(node);

	// Processed by FuncDeclStmt
}

void SymbolCollector::visit(FuncDeclStmt* node) {
	bindSourceFile(node);

	// Collect parameters
	bool hasVarargs = false; // Varargs must be last parameter

	Dict<Str, UniquePtr<FunctionParameterSymbol>> params;
	for (const auto& param : node->parameters) {
		if (hasVarargs) {
			symbolTable_->error(param.get(), "Varargs must be the last parameter");
			resetModifiers();
			return;
		}

		if (param->isParams) {
			hasVarargs = true;
		}

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
	bindSourceFile(node);

	node->condition->accept(*this);
	node->thenBlock->accept(*this);

	if (node->elseBlock) {
		node->elseBlock->accept(*this);
	}
}

void SymbolCollector::visit(ForStmt* node) {
	bindSourceFile(node);

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
	bindSourceFile(node);

	node->variable->accept(*this);
	node->collection->accept(*this);
	node->body->accept(*this);
}

void SymbolCollector::visit(WhileStmt* node) {
	bindSourceFile(node);

	node->condition->accept(*this);
	node->body->accept(*this);
}

void SymbolCollector::visit(LangBlockStmt* node) {
	bindSourceFile(node);

	// Untracked
}

void SymbolCollector::visit(AccessModifierStmt* node) {
	bindSourceFile(node);

	// Validate modifiers
	// Ensure no duplicates and no conflicts
	bool hasError = false;
	Str errorMsg;

	for (const auto& token : node->modifiers) {
		auto modifier = detail::parseAccessModifier(token.lexeme);

		if (detail::hasFlag(currentModifiers_, modifier)) {
			errorMsg = "Duplicate modifier: " + token.lexeme;
			hasError = true;
			break;
		}

		if (detail::hasFlag(currentModifiers_, AccessModifier::PRIVATE) && detail::hasFlag(modifier, AccessModifier::PROTECTED)) {
			errorMsg = "Cannot have both private and protected modifiers";
			hasError = true;
			break;
		}

		if (detail::hasFlag(currentModifiers_, AccessModifier::PROTECTED) && detail::hasFlag(modifier, AccessModifier::PRIVATE)) {
			errorMsg = "Cannot have both protected and private modifiers";
			hasError = true;
			break;
		}

		if (detail::hasFlag(currentModifiers_, AccessModifier::PUBLIC) && detail::hasFlag(modifier, AccessModifier::PRIVATE)) {
			errorMsg = "Cannot have both public and private modifiers";
			hasError = true;
			break;
		}

		if (detail::hasFlag(currentModifiers_, AccessModifier::PUBLIC) && detail::hasFlag(modifier, AccessModifier::PROTECTED)) {
			errorMsg = "Cannot have both public and protected modifiers";
			hasError = true;
			break;
		}

		if (detail::hasFlag(currentModifiers_, AccessModifier::PROTECTED) && detail::hasFlag(modifier, AccessModifier::PUBLIC)) {
			errorMsg = "Cannot have both protected and public modifiers";
			hasError = true;
			break;
		}

		currentModifiers_ = currentModifiers_ | modifier;
	}

	// Report error if any, and reset modifiers
	if (hasError) {
		symbolTable_->error(node, errorMsg);
		resetModifiers();

		currentModifiers_ = AccessModifier::NONE;
	}
}


void SymbolCollector::visit(NamespaceDeclStmt* node) {
	bindSourceFile(node);

	// Namespaces may only be declared at global scope or within another namespace
	if (currentScope_->kind != SymbolKind::NAMESPACE && 
		symbolTable_->findFirstNonImplicitParent(currentScope_)->kind != SymbolKind::NAMESPACE) {
		symbolTable_->error(node, "Namespace can only be declared at global scope or within another namespace");
		resetModifiers();
		return;
	}

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
	bindSourceFile(node);

	currentDeclSpec_ = node->spec->name;
}

void SymbolCollector::visit(UseStmt* node) {
	bindSourceFile(node);

	// Do we add import symbols?
}

void SymbolCollector::visit(ReturnStmt* node) {
	bindSourceFile(node);

	if (node->value) {
		node->value->accept(*this);
	}
}

void SymbolCollector::visit(EnumDeclStmt* node) {
	bindSourceFile(node);

	// Enums may not exist within a function or an interface
	// Ali is bronze
	if (currentScope_->kind == SymbolKind::FUNCTION || currentScope_->kind == SymbolKind::INTERFACE || 
		symbolTable_->findAncestorOfKind(currentScope_, SymbolKind::FUNCTION | SymbolKind::INTERFACE)) {
		symbolTable_->error(node, "Enums may not exist within a function or an interface");
		resetModifiers();
		return;
	}

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
	bindSourceFile(node);

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
	else if (node->type.lexeme == "interface") {
		typeSymbol = MakeUnique<InterfaceSymbol>(
			node->name->getTypeName(),
			currentScope_,
			node
		);

		typePtr = typeSymbol.get();
	}
	else { // shouldnt happen
		std::_Xruntime_error("Invalid type declaration");
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

void SymbolCollector::bindSourceFile(ast::Node* node) {
	node->sourceFile = currentFile_;
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