#include "expression_resolver.h"
#include "symbol_table.h"
#include "core/error_reporter.h"

MRK_NS_BEGIN_MODULE(semantic)

ExpressionResolver::ExpressionResolver(SymbolTable* symbolTable)
	: symbolTable_(symbolTable), currentFile_(nullptr) {}

void ExpressionResolver::resolve(ast::Program* program) {
	visit(program);
}

void ExpressionResolver::visit(Program* node) {
	currentFile_ = node->sourceFile;

	for (const auto& stmt : node->statements) {
		stmt->accept(*this);
	}
}

void ExpressionResolver::visit(LiteralExpr* node) {
	symbolTable_->setNodeResolvedSymbol(node,
		symbolTable_->getTypeSystem()->resolveTypeFromLiteral(node));
}

void ExpressionResolver::visit(InterpolatedStringExpr* node) {
	for (const auto& part : node->parts) {
		part->accept(*this);
	}

	symbolTable_->setNodeResolvedSymbol(node,
		symbolTable_->getTypeSystem()->getBuiltinType(TypeKind::STRING));
}

void ExpressionResolver::visit(InteropCallExpr* node) {
	// None
}

void ExpressionResolver::visit(IdentifierExpr* node) {
	// This is safe for call expressions, as we only resolve identifiers
	// Wont be though when we implement function overloading

	Symbol* symbol = symbolTable_->resolveSymbol(
		SymbolKind::IDENTIFIER,
		node->name,
		symbolTable_->getNodeScope(node)
	);

	if (!symbol) {
		symbolTable_->error(node, std::format("Undefined identifier: '{}'", node->name));
		setNodeAsError(node);
		return;
	}

	symbolTable_->setNodeResolvedSymbol(node, symbol);
}

void ExpressionResolver::visit(TypeReferenceExpr* node) {
	if (symbolTable_->getNodeResolvedSymbol(node)) {
		return;
	}

	// Check that this refers to a valid type
	TypeSymbol* type = symbolTable_->resolveType(node, symbolTable_->getNodeScope(node));
	if (!type) {
		symbolTable_->error(node, std::format("Unknown type: '{}'", node->getTypeName()));
		setNodeAsError(node);
		return;
	}

	// Check generic arguments if any
	for (const auto& genericArg : node->genericArgs) {
		genericArg->accept(*this);

		auto symbol = symbolTable_->getNodeResolvedSymbol(genericArg.get());
		if (!symbol || !detail::hasFlag(symbol->kind, SymbolKind::TYPE)) {
			symbolTable_->error(genericArg.get(), "Generic argument must be a type");
		}
	}

	// Set the resolved type to the TypeSymbol itself
	symbolTable_->setNodeResolvedSymbol(node, type);
}

void ExpressionResolver::visit(CallExpr* node) {
	node->target->accept(*this);

	for (const auto& arg : node->arguments) {
		arg->accept(*this);
	}

	// Validate that the target if given is callable
	if (!symbolTable_->getNodeResolvedSymbol(node->target.get())) {
		// Already reported error in target
		setNodeAsError(node);
		return;
	}

	// no function overloading for now
	// so we can safely resolve our call target by querying an identifier

	// Check if it's a direct function call or a method call
	if (auto identifier = dynamic_cast<IdentifierExpr*>(node->target.get())) {
		auto symbol = symbolTable_->getNodeResolvedSymbol(identifier);
		if (symbol && symbol->kind == SymbolKind::FUNCTION) {
			auto funcSymbol = static_cast<FunctionSymbol*>(symbol);

			// Check argument count
			if (funcSymbol->parameters.size() != node->arguments.size()) {
				symbolTable_->error(
					node,
					std::format("Function '{}' expects {} arguments but got {}",
						identifier->name, funcSymbol->parameters.size(), node->arguments.size()));
			}
			else {
					// TODO: Validate argument types
			}

			// Set return type
			symbolTable_->setNodeResolvedSymbol(node, const_cast<TypeSymbol*>(funcSymbol->resolver.returnType));
			return;
		}
	}
	else if (auto* memberAccess = dynamic_cast<MemberAccessExpr*>(node->target.get())) {
		// Handle method calls on objects (target.method())
		auto symbol = symbolTable_->getNodeResolvedSymbol(memberAccess);
		if (symbol && symbol->kind == SymbolKind::FUNCTION) {
			auto methodSymbol = static_cast<FunctionSymbol*>(symbol);

			// Similar checks for method arguments
			if (methodSymbol->parameters.size() != node->arguments.size()) {
				symbolTable_->error(
					node,
					std::format("Method '{}' expects {} arguments but got {}",
						methodSymbol->name, methodSymbol->parameters.size(), node->arguments.size()));
			}
			else {
				// TODO: Validate argument types
			}

			// Set return type for the method call
			symbolTable_->setNodeResolvedSymbol(node, const_cast<TypeSymbol*>(methodSymbol->resolver.returnType));
			return;
		}
	}

	// If we get here, the target isn't callable
	symbolTable_->error(
		node->target.get(),
		"Expression is not callable"
	);

	setNodeAsError(node);
}

void ExpressionResolver::visit(BinaryExpr* node) {
	node->left->accept(*this);
	node->right->accept(*this);

	// Check if the operator is valid for the types
	const auto leftType = getSymbolType(symbolTable_->getNodeResolvedSymbol(node->left.get()));
	const auto rightType = getSymbolType(symbolTable_->getNodeResolvedSymbol(node->right.get()));

	if (!leftType || !rightType) {
		setNodeAsError(node);
		return;
	}

	// Result?
	symbolTable_->setNodeResolvedSymbol(node,
		const_cast<TypeSymbol*>(symbolTable_->getTypeSystem()->getBinaryExpressionType(node->op.type, leftType, rightType)));

	if (isErrorNode(node)) {
		symbolTable_->error(
			node,
			std::format("Cannot apply operator '{}' to operands of type '{}' and '{}'",
				node->op.lexeme, leftType->qualifiedName, rightType->qualifiedName)
		);
	}
}

void ExpressionResolver::visit(UnaryExpr* node) {
	node->right->accept(*this);

	const auto rightType = getSymbolType(symbolTable_->getNodeResolvedSymbol(node->right.get()));
	if (!rightType) {
		setNodeAsError(node);
		return;
	}

	// Result?
	symbolTable_->setNodeResolvedSymbol(node,
		const_cast<TypeSymbol*>(symbolTable_->getTypeSystem()->getUnaryExpressionType(node->op.type, rightType)));

	if (isErrorNode(node)) {
		symbolTable_->error(
			node,
			std::format("Cannot apply operator '{}' to operand of type '{}'",
				node->op.lexeme, rightType->qualifiedName)
		);
	}
}

void ExpressionResolver::visit(TernaryExpr* node) {
	node->condition->accept(*this);
	node->thenBranch->accept(*this);
	node->elseBranch->accept(*this);

	// Check if the condition is a boolean
	const auto conditionType = getSymbolType(symbolTable_->getNodeResolvedSymbol(node->condition.get()));

	if (!conditionType ||
		!symbolTable_->getTypeSystem()->isAssignable(
			symbolTable_->getTypeSystem()->getBuiltinType(TypeKind::BOOL), conditionType)) {
		symbolTable_->error(node->condition.get(), "Condition must be a boolean expression");
		setNodeAsError(node);
		return;
	}

	// Set the resolved type to the common type of the then and else branches
	const auto thenType = getSymbolType(symbolTable_->getNodeResolvedSymbol(node->thenBranch.get()));
	const auto elseType = getSymbolType(symbolTable_->getNodeResolvedSymbol(node->elseBranch.get()));

	symbolTable_->setNodeResolvedSymbol(node,
		const_cast<TypeSymbol*>(symbolTable_->getTypeSystem()->getCommonType(thenType, elseType)));
}

void ExpressionResolver::visit(AssignmentExpr* node) {
	node->target->accept(*this);

	if (node->value) {
		node->value->accept(*this);
	}

	// Check target is assignable
	if (!symbolTable_->isLValue(node->target.get())) {
		symbolTable_->error(
			node->target.get(),
			"Left side of assignment must be a variable, property, or indexer"
		);
	}

	// Check if the types are compatible
	if (node->value) {
		const auto targetType = getSymbolType(symbolTable_->getNodeResolvedSymbol(node->target.get()));
		const auto valueType = getSymbolType(symbolTable_->getNodeResolvedSymbol(node->value.get()));

		if (targetType && valueType) {
			if (!symbolTable_->getTypeSystem()->isAssignable(targetType, valueType)) {
				symbolTable_->error(
					node,
					std::format("Cannot assign value of type '{}' to target of type '{}'",
						valueType->qualifiedName, targetType->qualifiedName)
				);
			}
		}
		else {
			setNodeAsError(node);
		}
	}

	// Set the resolved type to the target type
	symbolTable_->setNodeResolvedSymbol(node, symbolTable_->getNodeResolvedSymbol(node->target.get()));
}

void ExpressionResolver::visit(NamespaceAccessExpr* node) {
	// Visit all path segments
	Symbol* currentSymbol = nullptr;

	for (size_t i = 0; i < node->path.size(); i++) {
		node->path[i]->accept(*this);

		// First segment should resolve to a namespace
		if (i == 0) {
			if (auto* ident = dynamic_cast<IdentifierExpr*>(node->path[i].get())) {
				currentSymbol = symbolTable_->resolveSymbol(
					SymbolKind::NAMESPACE,
					ident->name,
					symbolTable_->getNodeScope(node)
				);

				if (!currentSymbol) {
					symbolTable_->error(
						node->path[i].get(),
						std::format("'{}' is not a namespace", ident->name)
					);
					setNodeAsError(node);
					return;
				}

				symbolTable_->setNodeResolvedSymbol(ident, currentSymbol);
			}
		}
		else {
			// Subsequent segments should resolve to members of the current namespace
			if (auto* ident = dynamic_cast<IdentifierExpr*>(node->path[i].get())) {
				if (currentSymbol && currentSymbol->kind == SymbolKind::NAMESPACE) {
					auto* ns = static_cast<NamespaceSymbol*>(currentSymbol);
					currentSymbol = ns->getMember(ident->name);

					if (!currentSymbol) {
						symbolTable_->error(
							node->path[i].get(),
							std::format("'{}' not found in namespace '{}'",
								ident->name, ns->qualifiedName)
						);
						setNodeAsError(node);
						return;
					}

					symbolTable_->setNodeResolvedSymbol(ident, currentSymbol);
				}
				else {
					symbolTable_->error(
						node->path[i - 1].get(),
						"Left side of '::' must be a namespace"
					);
					setNodeAsError(node);
					return;
				}
			}
		}
	}

	// Set the final resolved symbol and type for the namespace access expression
	symbolTable_->setNodeResolvedSymbol(node, currentSymbol);
}

void ExpressionResolver::visit(MemberAccessExpr* node) {
	// Visit target first
	node->target->accept(*this);

	// Check if target is valid and has a type
	auto targetSymbol = symbolTable_->getNodeResolvedSymbol(node->target.get());
	if (!targetSymbol) {
		setNodeAsError(node);
		return;
	}

	const auto* targetType = getSymbolType(targetSymbol);
	if (!targetType) {
		symbolTable_->error(
			node->target.get(),
			"Expression does not have a type and cannot have members"
		);
		setNodeAsError(node);
		return;
	}

	// Find the member in the target type
	Symbol* memberSymbol = nullptr;

	// For types, look up in the type's members
	if (targetType->kind == SymbolKind::TYPE) {
		memberSymbol = targetType->getMember(node->member->name);
	}
	// For namespaces, look up in the namespace's members
	else if (targetSymbol->kind == SymbolKind::NAMESPACE) {
		auto* ns = static_cast<const NamespaceSymbol*>(targetSymbol);
		memberSymbol = ns->getMember(node->member->name);
	}

	if (!memberSymbol) {
		symbolTable_->error(
			node->member.get(),
			std::format("'{}' does not contain a definition for '{}'",
				targetType->qualifiedName, node->member->name)
		);
		setNodeAsError(node);
		return;
	}

	// Check accessibility
	// TODO: Implement proper accessibility checking

	// Set resolved symbol and type
	symbolTable_->setNodeResolvedSymbol(node->member.get(), memberSymbol);
	symbolTable_->setNodeResolvedSymbol(node, memberSymbol);
}

void ExpressionResolver::visit(ArrayExpr* node) {
	TypeSymbol* commonElementType = nullptr;

	for (const auto& elem : node->elements) {
		elem->accept(*this);

		// Get element type
		auto elemSymbol = symbolTable_->getNodeResolvedSymbol(elem.get());
		const auto* elemType = getSymbolType(elemSymbol);

		if (!elemType) {
			continue; // Skip invalid elements
		}

		// Try to find common type for all elements
		if (!commonElementType) {
			commonElementType = const_cast<TypeSymbol*>(elemType);
		}
		else {
			commonElementType = const_cast<TypeSymbol*>(
				symbolTable_->getTypeSystem()->getCommonType(commonElementType, elemType)
				);

			if (!commonElementType) {
				symbolTable_->error(
					elem.get(),
					"Cannot determine common type for array elements"
				);

				setNodeAsError(node);
				return;
			}
		}
	}

	// If we couldn't determine a common type, default to object
	if (!commonElementType) {
		commonElementType = const_cast<TypeSymbol*>(
			symbolTable_->getTypeSystem()->getBuiltinType(TypeKind::OBJECT)
			);
	}

	// Create array type symbol
	// TODO: Create proper array type based on element type
	// For now, we'll just set the common element type as the resolved type
	symbolTable_->setNodeResolvedSymbol(node, commonElementType);
}

void ExpressionResolver::visit(ArrayAccessExpr* node) {
	// Visit target and index
	node->target->accept(*this);
	node->index->accept(*this);

	// Check if target is an array type
	auto targetSymbol = symbolTable_->getNodeResolvedSymbol(node->target.get());
	const auto* targetType = getSymbolType(targetSymbol);

	if (!targetType) {
		setNodeAsError(node);
		return;
	}

	// TODO: Implement proper array type checking
	// For now, just check if it's marked as an array in some way

	// Check if index is a numeric type
	auto indexSymbol = symbolTable_->getNodeResolvedSymbol(node->index.get());
	const auto* indexType = getSymbolType(indexSymbol);

	if (!indexType || !symbolTable_->getTypeSystem()->isIntegralType(indexType)) {
		symbolTable_->error(
			node->index.get(),
			std::format("Cannot use '{}' as array index, integer expected",
				indexType ? indexType->qualifiedName : "unknown type")
		);
	}

	// Set the resolved type to the element type of the array
	// TODO: Get actual element type from array type
	// For now, we'll just set it to the same type as the target
	symbolTable_->setNodeResolvedSymbol(node, const_cast<TypeSymbol*>(targetType));
}

void ExpressionResolver::visit(ExprStmt* node) {
	node->expr->accept(*this);
}

void ExpressionResolver::visit(VarDeclStmt* node) {
	if (node->typeName) {
		node->typeName->accept(*this);
	}

	// Visit and validate the nativeInitializerMethod expression
	if (node->initializer) {
		node->initializer->accept(*this);

		// Check if the nativeInitializerMethod type is assignable to the variable type
		auto varSymbol = symbolTable_->resolveSymbol(
			SymbolKind::VARIABLE,
			node->name->name,
			symbolTable_->getNodeScope(node)
		);

		if (varSymbol && varSymbol->kind == SymbolKind::VARIABLE) {
			auto& varType = static_cast<VariableSymbol*>(varSymbol)->resolver.type;
			auto initType = getSymbolType(symbolTable_->getNodeResolvedSymbol(node->initializer.get()));

			if (varType && initType && !symbolTable_->getTypeSystem()->isAssignable(varType, initType)) {
				symbolTable_->error(
					node->initializer.get(),
					std::format("Cannot implicitly convert type '{}' to '{}'",
						initType->qualifiedName, varType->qualifiedName)
				);
			}

			// Infer the variable type if not explicitly declared
			if (!varType || 
				varType == symbolTable_->getTypeSystem()->getBuiltinType(TypeKind::OBJECT)) {
				varType = initType;

				// Update decl node too
				// HACK: set empty, but manually resolve
				node->typeName = MakeUnique<TypeReferenceExpr>(node->startToken);

				// Resolve the type again
				symbolTable_->setNodeResolvedSymbol(node->typeName.get(), const_cast<TypeSymbol*>(initType));
			}
		}
	}
}

void ExpressionResolver::visit(BlockStmt* node) {
	for (const auto& stmt : node->statements) {
		stmt->accept(*this);
	}
}

void ExpressionResolver::visit(ParamDeclStmt* node) {
	if (node->type) {
		node->type->accept(*this);
	}

	if (node->name) {
		node->name->accept(*this);
	}

	if (node->initializer) {
		node->initializer->accept(*this);

		// Check if default value is assignable to parameter type
		auto* paramSymbol = symbolTable_->resolveSymbol(
			SymbolKind::FUNCTION_PARAMETER,
			node->name->name,
			symbolTable_->getNodeScope(node)
		);

		if (paramSymbol && paramSymbol->kind == SymbolKind::FUNCTION_PARAMETER) {
			auto* paramType = static_cast<FunctionParameterSymbol*>(paramSymbol)->resolver.type;
			auto* initType = getSymbolType(symbolTable_->getNodeResolvedSymbol(node->initializer.get()));

			if (paramType && initType && !symbolTable_->getTypeSystem()->isAssignable(paramType, initType)) {
				symbolTable_->error(
					node->initializer.get(),
					std::format("Cannot implicitly convert type '{}' to '{}'",
						initType->qualifiedName, paramType->qualifiedName)
				);
			}
		}
	}
}

void ExpressionResolver::visit(FuncDeclStmt* node) {
	if (node->body) {
		node->body->accept(*this);
	}
}

void ExpressionResolver::visit(IfStmt* node) {
	node->condition->accept(*this);
	node->thenBlock->accept(*this);

	if (node->elseBlock) {
		node->elseBlock->accept(*this);
	}
}

void ExpressionResolver::visit(ForStmt* node) {
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

void ExpressionResolver::visit(ForeachStmt* node) {
	node->variable->accept(*this);
	node->collection->accept(*this);
	node->body->accept(*this);
}

void ExpressionResolver::visit(WhileStmt* node) {
	node->condition->accept(*this);
	node->body->accept(*this);
}

void ExpressionResolver::visit(LangBlockStmt* node) {}

void ExpressionResolver::visit(AccessModifierStmt* node) {}

void ExpressionResolver::visit(NamespaceDeclStmt* node) {}

void ExpressionResolver::visit(DeclSpecStmt* node) {}

void ExpressionResolver::visit(UseStmt* node) {}

void ExpressionResolver::visit(ReturnStmt* node) {
	if (node->value) {
		node->value->accept(*this);

		// Find the enclosing function
		auto* currentScope = symbolTable_->getNodeScope(node);
		auto* funcScope = currentScope;

		while (funcScope && funcScope->kind != SymbolKind::FUNCTION) {
			funcScope = funcScope->parent;
		}

		// Check if return type matches the function's return type
		if (funcScope && funcScope->kind == SymbolKind::FUNCTION) {
			auto* funcSymbol = static_cast<const FunctionSymbol*>(funcScope);
			auto* returnType = funcSymbol->resolver.returnType;
			auto* valueType = getSymbolType(symbolTable_->getNodeResolvedSymbol(node->value.get()));

			if (returnType && valueType) {
				// If return type is void, we shouldn't have a return value
				if (returnType == symbolTable_->getTypeSystem()->getBuiltinType(TypeKind::VOID)) {
					symbolTable_->error(
						node->value.get(),
						"Cannot return a value from a function with a void return type"
					);
				}
				// Otherwise, check if return value is assignable to return type
				else if (!symbolTable_->getTypeSystem()->isAssignable(returnType, valueType)) {
					symbolTable_->error(
						node->value.get(),
						std::format("Cannot implicitly convert type '{}' to '{}'",
							valueType->qualifiedName, returnType->qualifiedName)
					);
				}
			}
		}
	}
	else {
		// If no return value provided, check if function expects a return value
		auto* currentScope = symbolTable_->getNodeScope(node);
		auto* funcScope = currentScope;

		while (funcScope && funcScope->kind != SymbolKind::FUNCTION) {
			funcScope = funcScope->parent;
		}

		if (funcScope && funcScope->kind == SymbolKind::FUNCTION) {
			auto funcSymbol = static_cast<const FunctionSymbol*>(funcScope);
			auto returnType = funcSymbol->resolver.returnType;

			if (returnType && returnType != symbolTable_->getTypeSystem()->getBuiltinType(TypeKind::VOID)) {
				symbolTable_->error(
					node,
					std::format("'return' statement must return a value of type '{}'",
						returnType->qualifiedName)
				);
			}
		}
	}
}

void ExpressionResolver::visit(EnumDeclStmt* node) {
	if (node->name) {
		node->name->accept(*this);
	}

	if (node->type) {
		node->type->accept(*this);
	}

	auto* enumSymbol = symbolTable_->resolveSymbol(
		SymbolKind::ENUM,
		node->name->name,
		symbolTable_->getNodeScope(node)
	);

	// Visit enum members and their values
	for (const auto& [memberName, memberValue] : node->members) {
		if (memberName) {
			memberName->accept(*this);
		}

		if (memberValue) {
			memberValue->accept(*this);

			if (enumSymbol && enumSymbol->kind == SymbolKind::ENUM) {
				auto* enumType = static_cast<EnumSymbol*>(enumSymbol);
				auto* valueType = getSymbolType(symbolTable_->getNodeResolvedSymbol(memberValue.get()));
				if (enumType && valueType && !symbolTable_->getTypeSystem()->isAssignable(enumType, valueType)) {
					symbolTable_->error(
						memberValue.get(),
						std::format("Cannot implicitly convert type '{}' to '{}'",
							valueType->qualifiedName, enumType->qualifiedName)
					);
				}
			}
		}
	}
}

void ExpressionResolver::visit(TypeDeclStmt* node) {
	if (node->body) {
		node->body->accept(*this);
	}
}

void ExpressionResolver::setNodeAsError(const ExprNode* node) {
	symbolTable_->setNodeResolvedSymbol(node, symbolTable_->getTypeSystem()->getErrorType());
}

const TypeSymbol* ExpressionResolver::getSymbolType(const Symbol* symbol) {
	if (symbol) {
		if (detail::hasFlag(symbol->kind, SymbolKind::TYPE)) {
			return static_cast<const TypeSymbol*>(symbol);
		}

		if (detail::hasFlag(symbol->kind, SymbolKind::FUNCTION)) {
			return static_cast<const FunctionSymbol*>(symbol)->resolver.returnType;
		}

		if (detail::hasFlag(symbol->kind, SymbolKind::VARIABLE)) {
			return static_cast<const VariableSymbol*>(symbol)->resolver.type;
		}

		if (detail::hasFlag(symbol->kind, SymbolKind::FUNCTION_PARAMETER)) {
			return static_cast<const FunctionParameterSymbol*>(symbol)->resolver.type;
		}
	}

	return symbolTable_->getTypeSystem()->getErrorType();
}

bool ExpressionResolver::isErrorNode(const ExprNode* node) const {
	auto sym = symbolTable_->getNodeResolvedSymbol(node);
	return sym == nullptr || sym == symbolTable_->getTypeSystem()->getErrorType();
}

MRK_NS_END
