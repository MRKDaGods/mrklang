#include "symbol_table.h"
#include "common/utils.h"
#include "common/logging.h"
#include "symbol_visitor.h"
#include "core/error_reporter.h"

#include <iostream>
#include <format>
#include <algorithm>

MRK_NS_BEGIN_MODULE(semantic)

void NamespaceSymbolDescriptor::addVariant(const NamespaceSymbol* variant) {
	isImplicit |= variant->isImplicit;
	variants.push_back(variant);
}

//				__global
//				/		\
//			File1	  File2
//			  /			  \
//		   mrkstl		 mrkstl
//          /			    \
//		  sym1			    sym2

SymbolTable::SymbolTable(Vec<UniquePtr<ast::Program>>&& programs)
	: programs_(Move(programs)), globalNamespace_(nullptr) {}

void SymbolTable::build() {
	// Create global namespace
	globalNamespace_ = declareNamespace("__global");

	SymbolVisitor collector(this);

	for (const auto& program : programs_) {
		// Set current file for error reporting
		ErrorReporter::instance().setCurrentFile(program->sourceFile);

		// Collect symbols
		collector.visit(program.get());
	}
}

void SymbolTable::dump() const {
	MRK_INFO("Symbol Table:");

	if (globalNamespace_) {
		dumpSymbol(globalNamespace_, 0);
	}

	// Print file scopes
	MRK_INFO("File Scopes:");
	for (const auto& [filename, scope] : fileScopes_) {
		MRK_INFO("\tFile: {}", filename);
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
			// First print all implicit namespaces' members as if theyre ours
			for (const auto& [_, childNs] : ns->namespaces) {
				if (childNs.isImplicit) {
					for (const auto& variant : childNs.variants) {
						for (const auto& variantMember : variant->members) {
							if (hasMembers) { // Print members header if not printed yet
								MRK_INFO("{}Members:", indentation);
								hasMembers = false;
							}

							dumpSymbol(variantMember.second.get(), indent + 2);
						}
					}
				}
			}

			// Then process actual namespaces
			MRK_INFO("{}Namespaces:", indentation);

			for (const auto& [name, childNs] : ns->namespaces) {
				MRK_INFO("{}[{}] {}", indentation, childNs.isImplicit ? "IMPLICIT" : "-", name);

				if (!childNs.isImplicit) {
					for (const auto& variant : childNs.variants) {
						dumpSymbol(variant, indent + 2);
					}
				}
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

	auto nsSymbol = MakeUnique<NamespaceSymbol>(nsName, false, parent, declNode);
	NamespaceSymbol* ptr = nsSymbol.get();
	namespaces_[namespaceFullname] = Move(nsSymbol);

	// Non implicit tracking
	ptr->nonImplicitQualifiedName = getNonImplicitSymbolName(ptr);
	namespaceDescriptors_[ptr->nonImplicitQualifiedName].addVariant(ptr);

	if (parent) {
		parent->namespaces[ptr->name].addVariant(ptr);

		// Check if parent is implicit, if so add to nearest non-implicit ancestor (namespace)
		if (parent->isImplicit) {
			auto ancestor = dynamic_cast<NamespaceSymbol*>(parent->parent);
			while (ancestor && ancestor->isImplicit) {
				ancestor = dynamic_cast<NamespaceSymbol*>(ancestor->parent);
			}

			if (ancestor) {
				ancestor->namespaces[ptr->name].addVariant(ptr);
			}
		}
	}

	return ptr;
}

NamespaceSymbol* SymbolTable::declareFileScope(const Str& filename) {
	if (fileScopes_.find(filename) != fileScopes_.end()) {
		return fileScopes_[filename];
	}

	// Create implicit namespace
	auto fileNamespace = MakeUnique<NamespaceSymbol>(filename, true, globalNamespace_, nullptr);
	NamespaceSymbol* ptr = fileNamespace.get();
	fileScopes_[filename] = ptr;

	namespaceDescriptors_[ptr->nonImplicitQualifiedName].addVariant(ptr);

	// Add to global namespace
	globalNamespace_->namespaces[filename].addVariant(ptr);

	// Also register it in the global namespaces for later linking
	auto fileQualifiedNms = "__global::" + filename;
	namespaces_[fileQualifiedNms] = Move(fileNamespace);
	return ptr;
}

void SymbolTable::addType(TypeSymbol* type) {
	types_.push_back(type);
}

void SymbolTable::addFunction(FunctionSymbol* function) {
	functions_.push_back(function);
}

void SymbolTable::addImport(const SourceFile* file, ImportEntry&& entry) {
	imports_[file].push_back(Move(entry));
}

void SymbolTable::error(const ASTNode* node, const Str& message) {
	// Doubt if we should throw here
	// Just report for now
	ErrorReporter::instance().semanticError(message, node);
}

Symbol* SymbolTable::findFirstNonImplicitParent(const Symbol* symbol, bool includeMe) const {
	if (includeMe && detail::hasFlag(symbol->kind, SymbolKind::NAMESPACE) && 
		!dynamic_cast<const NamespaceSymbol*>(symbol)->isImplicit) {
		return const_cast<Symbol*>(symbol);
	}

	auto parent = symbol->parent;
	while (parent && (detail::hasFlag(parent->kind, SymbolKind::NAMESPACE) || detail::hasFlag(parent->kind, SymbolKind::BLOCK))) {
		if (detail::hasFlag(parent->kind, SymbolKind::NAMESPACE) && !dynamic_cast<const NamespaceSymbol*>(parent)->isImplicit) {
			return parent;
		}

		parent = parent->parent;
	}

	return parent;
}

Symbol* SymbolTable::findAncestorOfKind(const Symbol* symbol, const SymbolKind& kind) const {
	auto parent = symbol->parent;
	while (parent && !detail::hasFlag(parent->kind, kind)) {
		parent = parent->parent;
	}

	return parent;
}

NamespaceSymbol* SymbolTable::getGlobalNamespace() const {
	return globalNamespace_;
}

void SymbolTable::resolve() {
	// Resolve types
	for (auto& type : types_) {
		Vec<TypeSymbol*> baseTypes;

		// If we allow structs to have nested types later on, add SymbolKind::STRUCT to the mask
		auto scope = findAncestorOfKind(type, SymbolKind::NAMESPACE | SymbolKind::CLASS);
		for (auto& base : type->baseTypes) {
			auto resolvedBaseType = findTypeSymbol(type->declNode->sourceFile, base, scope);
			if (!resolvedBaseType) {
				error(type->declNode, utils::concat("Cannot resolve base type '", base, "'"));
			}
			else {
				MRK_INFO("Resolved base type: {}", resolvedBaseType->qualifiedName);
			}
		}
	}
}

Str SymbolTable::getNonImplicitSymbolName(const Symbol* symbol) const {
	auto components = utils::split(symbol->qualifiedName, "::");

	Vec<Str> nonImplicitPath;
	for (const auto& component : components) {
		if (component.starts_with("<file>")) continue;

		nonImplicitPath.push_back(component);
	}

	return utils::formatCollection(nonImplicitPath, "::", nullptr);
}

TypeSymbol* SymbolTable::lookupInSymbol(const Symbol* context, const Str& name) const {
	if (!context) return nullptr;

	// Check if context is a namespace or type
	if (auto ns = dynamic_cast<const NamespaceSymbol*>(context)) {
		auto it = ns->members.find(name);
		if (it != ns->members.end() && detail::hasFlag(it->second->kind, SymbolKind::TYPE)) {
			return dynamic_cast<TypeSymbol*>(it->second.get());
		}
	}
	else if (auto type = dynamic_cast<const TypeSymbol*>(context)) {
		auto it = type->members.find(name);
		if (it != type->members.end() && detail::hasFlag(it->second->kind, SymbolKind::TYPE)) {
			return dynamic_cast<TypeSymbol*>(it->second.get());
		}
	}

	return nullptr;
}

NamespaceSymbol* SymbolTable::resolveNamespaceComponent(const Symbol* context, const Str& component) const {
	Str path = utils::concat(context ? getNonImplicitSymbolName(context) : "__global", "::", component);

	auto it = namespaceDescriptors_.find(path);
	if (it != namespaceDescriptors_.end()) {
		// Return first non-implicit variant
		for (const auto& variant : it->second.variants) {
			if (!variant->isImplicit) {
				return const_cast<NamespaceSymbol*>(variant);
			}
		}
		return const_cast<NamespaceSymbol*>(it->second.variants.front());
	}
	return nullptr;
}

TypeSymbol* SymbolTable::resolveTypeComponent(const Symbol* context, const Str& component) const {
	const Symbol* current = context;
	while (current) {
		if (TypeSymbol* type = lookupInSymbol(current, component)) {
			return type;
		}
		current = current->parent;
	}
	return nullptr;
}

TypeSymbol* SymbolTable::checkCurrentScopeHierarchy(const Symbol* scope, const Str& name) const {
	while (scope) {
		MRK_INFO("Checking scope: '{}'", scope->qualifiedName);

		if (scope->kind == SymbolKind::NAMESPACE) {
			// Go through variants
			auto variants = namespaceDescriptors_.at(getNonImplicitSymbolName(scope)).variants;
			for (const auto& v : variants) {
				auto memberIt = v->members.find(name);
				if (memberIt != v->members.end() &&
					detail::hasFlag(memberIt->second->kind, SymbolKind::TYPE)) {
					return dynamic_cast<TypeSymbol*>(memberIt->second.get());
				}
			}
		}
		else {
			auto memberIt = scope->members.find(name);
			if (memberIt != scope->members.end() &&
				detail::hasFlag(memberIt->second->kind, SymbolKind::TYPE)) {
				return dynamic_cast<TypeSymbol*>(memberIt->second.get());
			}
		}

		scope = scope->parent;
	}
	return nullptr;
}

TypeSymbol* SymbolTable::checkQualifiedPath(const Vec<Str>& components) const {
	const Symbol* context = globalNamespace_;
	size_t componentsProcessed = 0;

	for (const auto& component : components) {
		// First try to resolve as namespace
		if (auto ns = resolveNamespaceComponent(context, component)) {
			context = ns;
			componentsProcessed++;
			continue;
		}

		// Then try to resolve as type
		if (TypeSymbol* type = resolveTypeComponent(context, component)) {
			// Handle remaining components as nested types
			for (size_t i = componentsProcessed + 1; i < components.size(); i++) {
				type = lookupInSymbol(type, components[i]);
				if (!type) return nullptr;
			}
			return type;
		}

		// Component not found in either namespace or type
		return nullptr;
	}

	// If all components were namespaces
	return dynamic_cast<TypeSymbol*>(const_cast<Symbol*>(context));
}

TypeSymbol* SymbolTable::checkFileScope(const SourceFile* fromFile, const Str& symbolText) const {
	auto fileIt = fileScopes_.find(fromFile->filename);
	if (fileIt != fileScopes_.end()) {
		auto memberIt = fileIt->second->members.find(symbolText);
		if (memberIt != fileIt->second->members.end() &&
			detail::hasFlag(memberIt->second->kind, SymbolKind::CLASS | SymbolKind::INTERFACE)) {
			return dynamic_cast<TypeSymbol*>(memberIt->second.get());
		}
	}
	return nullptr;
}

TypeSymbol* SymbolTable::checkImports(const SourceFile* fromFile, const Str& symbolText) const {
	auto importIt = imports_.find(fromFile);
	if (importIt != imports_.end()) {
		for (const auto& import : importIt->second) {
			Str importPath = "__global";
			if (!import.path.empty()) {
				importPath += "::" + import.path;
			}

			// Check both namespace and type imports
			if (TypeSymbol* type = checkQualifiedPath(utils::split(importPath + "::" + symbolText, "::"))) {
				return type;
			}
		}
	}
	return nullptr;
}

TypeSymbol* SymbolTable::checkGlobalNamespace(const Str& symbolText) const {
	for (const auto& [_, descriptor] : globalNamespace_->namespaces) {
		for (const auto& variant : descriptor.variants) {
			auto memberIt = variant->members.find(symbolText);
			if (memberIt != variant->members.end() &&
				detail::hasFlag(memberIt->second->kind, SymbolKind::TYPE)) {
				return dynamic_cast<TypeSymbol*>(memberIt->second.get());
			}
		}
	}
	return nullptr;
}

TypeSymbol* SymbolTable::findTypeSymbol(const SourceFile* fromFile, const Str& symbolText, const Symbol* currentScope) {
	MRK_INFO("Searching for type symbol: '{}'{}", symbolText,
		fromFile ? std::format(" in file '{}'", fromFile->filename) : "");

	// 1. Check current scope hierarchy (including implicit parents)
	if (currentScope) {
		MRK_INFO("Starting search from current scope: '{}'", currentScope->qualifiedName);
		if (TypeSymbol* type = checkCurrentScopeHierarchy(currentScope, symbolText)) {
			return type;
		}
	}

	// 2. Handle qualified names
	auto components = utils::split(symbolText, "::");
	if (components.size() > 1) {
		MRK_INFO("Processing qualified name with {} components", components.size());
		if (TypeSymbol* type = checkQualifiedPath(components)) {
			return type;
		}
	}

	// 3. Check file scope
	if (fromFile) {
		MRK_INFO("Checking file scope for '{}'", fromFile->filename);
		if (TypeSymbol* type = checkFileScope(fromFile, symbolText)) {
			return type;
		}
	}

	// 4. Check imports
	if (fromFile && components.size() == 1) {
		MRK_INFO("Checking imports for '{}'", symbolText);
		if (TypeSymbol* type = checkImports(fromFile, symbolText)) {
			return type;
		}
	}

	// 5. Check global namespace
	MRK_INFO("Checking global namespace as last resort");
	return checkGlobalNamespace(symbolText);
}


MRK_NS_END
