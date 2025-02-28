#pragma once

#include "common/types.h"

#include <unordered_map>

MRK_NS_BEGIN

struct Symbol {
	Str name;
	Str qualifiedName;
	Symbol* parent;

	Symbol(Str&& name, Symbol* parent = nullptr)
		: name(Move(name)), parent(parent) {
		qualifiedName = (parent ? (parent->qualifiedName + "::" + name) : name);
	}

	virtual Str toString() const = 0;
};

struct NamespaceSymbol : Symbol {
	MRK_STD unordered_map<Str, UniquePtr<Symbol>> members;

	NamespaceSymbol(MRK_STD unordered_map<Str, UniquePtr<Symbol>>&& members, Str&& name, Symbol* parent)
		: Symbol(Move(name), parent), members(Move(members)) {
	}

	Str toString() const override {
		return "Namespace: " + name;
	}
};

class SymbolTable {
private:
	MRK_STD unordered_map<Str, UniquePtr<NamespaceSymbol>> namespaces;
	MRK_STD unordered_map<Str, NamespaceSymbol*> fileScopes;
};

MRK_NS_END