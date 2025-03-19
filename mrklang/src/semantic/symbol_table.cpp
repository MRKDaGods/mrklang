#include "symbol_table.h"
#include "common/utils.h"
#include "common/logging.h"
#include "symbol_visitor.h"
#include "expression_resolver.h"
#include "core/error_reporter.h"
#include "common/declspecs.h"

#include <iostream>
#include <format>
#include <algorithm>

MRK_NS_BEGIN_MODULE(semantic)

//				__global
//				/		\
//			File1	  File2
//			  /			  \
//		   mrkstl		 mrkstl
//          /			    \
//		  sym1			    sym2

SymbolTable::SymbolTable(Vec<UniquePtr<ast::Program>>&& programs)
	: programs_(Move(programs)), globalNamespace_(nullptr), globalType_(nullptr), globalFunction_(nullptr) {}

void SymbolTable::build() {
	// Create globals
	setupGlobals();

	// Initialize type system
	typeSystem_ = MakeUnique<TypeSystem>(this);

	SymbolVisitor collector(this);

	for (const auto& program : programs_) {
		// Set current file for error reporting
		ErrorReporter::instance().setCurrentFile(program->sourceFile);

		// Collect symbols
		collector.visit(program.get());
	}

	// Validate imports
	validateImports();

	// Resolve symbols
	resolve();
}

void SymbolTable::dump() const {
	MRK_INFO("Symbol Table:");

	if (globalNamespace_) {
		dumpSymbol(globalNamespace_, 0);
	}
}

void SymbolTable::dumpSymbol(const Symbol* symbol, int indent) const {
	if (!symbol) return;

	Str indentation(indent * 2, ' ');

	// Print symbol type and name
	MRK_INFO("{}[{}]: {}", indentation, toString(symbol->kind), symbol->name);

	MRK_INFO("{}Access Modifiers: [{}]", indentation, detail::formatAccessModifier(symbol->accessModifier));
	MRK_INFO("{}Declaration Spec: [{}]", indentation, symbol->declSpec);

	// Print params if function
	if (detail::hasFlag(symbol->kind, SymbolKind::FUNCTION)) {
		auto fn = dynamic_cast<const FunctionSymbol*>(symbol);
		MRK_INFO("{}Return Type: {}", indentation, fn->returnType);

		auto params = utils::formatCollection(fn->parameters, ", ", [](const auto& param) {
			return std::format("{}{} {}",
				param.second->isParams ? "params " : "", param.second->type, param.second->name);
		});

		MRK_INFO("{}Parameters: ({})", indentation, params);
	}

	// Print members if any exist
	bool hasMembers = !symbol->members.empty();
	if (hasMembers) {
		MRK_INFO("{}Members:", indentation);
		for (const auto& [name, member] : symbol->members) {
			dumpSymbol(member.get(), indent + 2);
		}
	}

	// Print child namespaces incase this is a namespace
	if (detail::hasFlag(symbol->kind, SymbolKind::NAMESPACE)) {
		auto ns = dynamic_cast<const NamespaceSymbol*>(symbol);
		if (!ns->namespaces.empty()) {
			MRK_INFO("{}Namespaces:", indentation);

			for (const auto& [name, childNs] : ns->namespaces) {
				MRK_INFO("{} {}", indentation, name);
				dumpSymbol(childNs, indent + 2);
			}
		}
	}
}

Str SymbolTable::formatAccessModifiers(const Symbol* symbol) const {
	if (symbol->accessModifier == AccessModifier::NONE) {
		return "";
	}

	return std::format("[{}]", detail::formatAccessModifier(symbol->accessModifier));
}

NamespaceSymbol* SymbolTable::declareNamespace(const Str& nsName, NamespaceSymbol* parent, ASTNode* declNode) {
	auto namespaceFullname = nsName;
	if (parent) {
		namespaceFullname = parent->qualifiedName + "::" + nsName;
	}

	auto it = namespaces_.find(namespaceFullname);
	if (it != namespaces_.end())
		return it->second.get();

	auto nsSymbol = MakeUnique<NamespaceSymbol>(nsName, parent, declNode);
	NamespaceSymbol* ptr = nsSymbol.get();
	namespaces_[namespaceFullname] = Move(nsSymbol);

	if (parent) {
		parent->namespaces[ptr->name] = ptr;
	}

	return ptr;
}

void SymbolTable::addType(TypeSymbol* type) {
	types_.push_back(type);

	if (type->declSpec == DECLSPEC_INJECT_GLOBAL) {
		globalType_ = type;
	}
}

void SymbolTable::addVariable(VariableSymbol* variable) {
	variables_.push_back(variable);
}

void SymbolTable::addFunction(FunctionSymbol* function) {
	functions_.push_back(function);

	if (function->declSpec == DECLSPEC_INJECT_GLOBAL) {
		globalFunction_ = function;
	}
}

void SymbolTable::addImport(const SourceFile* file, ImportEntry&& entry) {
	imports_[file].push_back(Move(entry));
}

void SymbolTable::addRigidLanguageBlock(LangBlockStmt* block) {
	rigidLanguageBlocks_.insert(block);
}

void SymbolTable::error(const ASTNode* node, const Str& message) {
	// Doubt if we should throw here
	// Just report for now

	if (node->sourceFile) {
		ErrorReporter::instance().setCurrentFile(node->sourceFile);
	}

	ErrorReporter::instance().semanticError(message, node);
}

const Symbol* SymbolTable::findFirstNonImplicitParent(const Symbol* symbol, bool includeMe) const {
	// Since we have removed file scopes, only block scopes are considered implicit

	if (includeMe && !detail::hasFlag(symbol->kind, SymbolKind::BLOCK)) {
		return symbol;
	}

	auto parent = symbol->parent;
	while (parent && detail::hasFlag(parent->kind, SymbolKind::BLOCK)) {
		parent = parent->parent;
	}

	return parent;
}

Symbol* SymbolTable::findAncestorOfKind(const Symbol* symbol, SymbolKind kind) const {
	auto parent = symbol->parent;
	while (parent && !detail::hasFlag(parent->kind, kind)) {
		parent = parent->parent;
	}

	return parent;
}

bool SymbolTable::isLValue(ast::ExprNode* expr) {
	// Variables are l-values
	if (auto* identExpr = dynamic_cast<ast::IdentifierExpr*>(expr)) {
		auto it = resolvedSymbols_.find(identExpr);
		if (it != resolvedSymbols_.end() &&
			(it->second->kind == SymbolKind::VARIABLE ||
				it->second->kind == SymbolKind::FUNCTION_PARAMETER)) {
			return true;
		}
	}

	// Member access can be l-value if it's a field
	if (auto* memberAccess = dynamic_cast<ast::MemberAccessExpr*>(expr)) {
		auto it = resolvedSymbols_.find(memberAccess->member.get());
		if (it != resolvedSymbols_.end() && it->second->kind == SymbolKind::VARIABLE) {
			return true;
		}
	}

	// Array access
	if (dynamic_cast<ast::ArrayAccessExpr*>(expr)) {
		return true;
	}

	return false;
}

TypeSymbol* SymbolTable::resolveType(ast::TypeReferenceExpr* typeRef, const Symbol* scope) {
	auto fullName = typeRef->getTypeName();
	auto typeSymbol = resolveSymbol(SymbolKind::TYPE, fullName, scope);
	if (!typeSymbol) {
		error(typeRef, std::format("Could not resolve type '{}'", fullName));
		return nullptr;
	}

	return dynamic_cast<TypeSymbol*>(typeSymbol);
}

void SymbolTable::setNodeScope(const ASTNode* node, const Symbol* scope) {
	nodeScopes_[node] = scope;
}

const Symbol* SymbolTable::getNodeScope(const ASTNode* node) {
	auto it = nodeScopes_.find(node);
	return it != nodeScopes_.end() ? it->second : nullptr;
}

void SymbolTable::setNodeResolvedSymbol(const ast::ExprNode* node, Symbol* symbol) {
	resolvedSymbols_[node] = symbol;
}

Symbol* SymbolTable::getNodeResolvedSymbol(const ast::ExprNode* node) const {
	if (!node) return nullptr;

	auto it = resolvedSymbols_.find(node);
	return it != resolvedSymbols_.end() ? it->second : nullptr;
}

void SymbolTable::setupGlobals() {
	// Create global namespace
	globalNamespace_ = declareNamespace("__global");
}

void SymbolTable::validateImport(const ImportEntry& entry) {
	// For now just check if nms exists
	if (!resolveSymbol(SymbolKind::NAMESPACE, entry.path, globalNamespace_, SymbolResolveFlags::ANCESTORS)) {
		error(entry.node, std::format("Could not resolve import '{}'", entry.path));
	}
}

void SymbolTable::validateImports() {
	for (const auto& [_, entries] : imports_) {
		for (const auto& entry : entries) {
			validateImport(entry);
		}
	}
}

Symbol* SymbolTable::resolveSymbol(SymbolKind kind, const Str& symbolText, const Symbol* scope, SymbolResolveFlags flags) {
	// First try to resolve in the scope and its ancestors
	Symbol* symbol = nullptr;

	if (detail::hasFlag(flags, SymbolResolveFlags::ANCESTORS)) {
		symbol = resolveSymbolInternal(kind, symbolText, scope);
		if (symbol) {
			return symbol;
		}
	}

	// Try to resolve in imports by constructing qualified names from imports
	if (detail::hasFlag(flags, SymbolResolveFlags::IMPORTS)) {
		for (const auto& [_, entries] : imports_) {
			for (const auto& entry : entries) {
				auto qualifiedName = utils::concat(entry.path, "::", symbolText);
				auto symbol = resolveSymbolInternal(kind, qualifiedName, globalNamespace_);
				if (symbol) {
					return symbol;
				}
			}
		}
	}

	return nullptr;
}

Symbol* SymbolTable::resolveSymbolInternal(SymbolKind kind, const Str& symbolText, const Symbol* scope, const Symbol* requestor) {
	// Find the symbol in the scope
	// symbolText may be a raw unqualified name or a qualified name(namespace + nested class)
	if (!scope) { // We reached the top
		return nullptr;
	}

	// If scope is global nms, check in our globalType too if the target is a function/var
	if (requestor != globalNamespace_ && scope == globalNamespace_ && detail::hasFlag(kind, SymbolKind::FUNCTION | SymbolKind::VARIABLE)) {
		auto sym = resolveSymbolInternal(kind, symbolText, globalType_, globalNamespace_);
		if (sym) {
			return sym;
		}
	}

	// MRK_INFO("Resolving symbol '{}' in scope '{}'", symbolText, scope->toString());

	// Declare on top because of goto
	Symbol* symbol = nullptr;

	// Check if symbolText is a qualified name
	auto parts = utils::split(symbolText, "::");
	if (parts.size() > 1) {
		// Last part is the actual symbol name
		auto realSymbolName = parts.back();
		parts.pop_back();

		// Traverse namespaces/types
		// If we failed to find nms, try type. If failed, return nullptr
		auto current = scope;
		for (const auto& part : parts) {
			// Try to find next part in current scope
			auto next = current->getMember(part);
			if (!next) {
				goto exit;
			}

			current = next;
		}

		return resolveSymbolInternal(kind, realSymbolName, current, requestor);
	}

	symbol = scope->getMember(symbolText);
	if (symbol && detail::hasFlag(symbol->kind, kind)) {
		return symbol;
	}

exit:
	return resolveSymbolInternal(kind, symbolText, scope->parent, requestor);
}

void SymbolTable::resolve() {
	// Resolve types
	for (auto type : types_) {
		// Resolve base types
		Vec<const TypeSymbol*> resolvedBaseTypes;
		for (auto& baseType : type->baseTypes) {
			auto baseTypeSymbol = resolveSymbol(SymbolKind::TYPE, baseType, type->parent);
			if (!baseTypeSymbol) {
				error(type->declNode, std::format("Could not resolve base type '{}'", baseType));
				continue;
			}

			resolvedBaseTypes.push_back(dynamic_cast<const TypeSymbol*>(baseTypeSymbol));
		}

		type->resolver.resolve(Move(resolvedBaseTypes));
	}

	// Resolve variables
	for (auto variable : variables_) {
		auto typeSymbol = resolveSymbol(SymbolKind::TYPE, variable->type, variable->parent);
		if (!typeSymbol) {
			error(variable->declNode, std::format("Could not resolve variable type '{}'", variable->type));
			continue;
		}

		variable->resolver.resolve(dynamic_cast<const TypeSymbol*>(typeSymbol));
	}


	// Resolve functions
	for (auto function : functions_) {
		// Resolve return type
		auto returnTypeSymbol = resolveSymbol(SymbolKind::TYPE, function->returnType, function);
		if (!returnTypeSymbol) {
			error(function->declNode, std::format("Could not resolve return type '{}'", function->returnType));
			continue;
		}

		function->resolver.resolve(returnTypeSymbol);

		// Resolve parameters
		for (auto& [name, param] : function->parameters) {
			auto paramTypeSymbol = resolveSymbol(SymbolKind::TYPE, param->type, function);
			if (!paramTypeSymbol) {
				error(param->declNode, std::format("Could not resolve parameter type '{}'", param->type));
				continue;
			}

			param->resolver.resolve(paramTypeSymbol);
		}
	}

	// Resolve expressions..
	ExpressionResolver exprResolver(this);
	for (const auto& program : programs_) {
		exprResolver.visit(program.get());
	}
}

MRK_NS_END
