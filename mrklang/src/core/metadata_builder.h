#pragma once

#include "common/types.h"
#include "semantic/symbol_table.h"

#define MRKLANG_COMPILER
#include "mrk-metadata.h"

MRK_NS_BEGIN

using namespace semantic;
using namespace runtime::metadata;

class MetadataBuilder {
public:
	MetadataBuilder(const SymbolTable& symbolTable);
	void build();

private:
	const SymbolTable& symbolTable_;
	UniquePtr<Image> image_;
	Dict<Str, Type*> types_;
	Type* globalNamespaceType_;

	Str translateScopeToTypename(const Str& qualifiedName);
	void processNamespace(const NamespaceSymbol* symbol);
	void processSymbol(const Symbol* symbol);
	UniquePtr<Type> createNamespaceType(const NamespaceSymbol* symbol);
};

MRK_NS_END