#include "symbol_table.h"
#include "common/utils.h"
#include "common/logging.h"
#include "symbol_collector.h"

#include <iostream>
#include <format>
#include <algorithm>

MRK_NS_BEGIN

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

	SymbolCollector collector(this);

	for (const auto& program : programs_) {
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
	if (symbol->kind == SymbolKind::FUNCTION) {
		auto fn = dynamic_cast<const FunctionSymbol*>(symbol);
		MRK_INFO("{}Return Type: {}", indentation, fn->returnType);

		auto params = formatCollection(fn->parameters, ", ", [](const auto& param) {
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
	if (symbol->kind == SymbolKind::NAMESPACE) {
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

	if (parent) {
		parent->namespaces[ptr->name].addVariant(ptr);

		// Check if parent is implicit, if so add to nearest non-implicit ancestor
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

	// Add to global namespace
	globalNamespace_->namespaces[filename].addVariant(ptr);

	// Also register it in the global namespaces for later linking
	auto fileQualifiedNms = "__global::" + filename;
	namespaces_[fileQualifiedNms] = Move(fileNamespace);
	return ptr;
}

MRK_NS_END
