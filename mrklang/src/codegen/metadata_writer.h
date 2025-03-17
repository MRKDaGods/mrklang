#pragma once

#include "common/types.h"
#include "semantic/symbol_table.h"
#include "mrk-metadata.h"

#include <fstream>

MRK_NS_BEGIN_MODULE(codegen)

using namespace semantic;

/// Fed to the cpp gen to handle metadata registration for types, methods & fields
using CompilerMetadataRegistration = 
	runtime::metadata::MetadataRegistration<const TypeSymbol, const VariableSymbol, const FunctionSymbol>;

class MetadataWriter {
public:
	MetadataWriter(const SymbolTable* symbolTable);
	UniquePtr<CompilerMetadataRegistration> writeMetadataFile(const Str& path);

private:
	const SymbolTable* symbolTable_;
	std::ofstream file_;
	Dict<Str, uint32_t> stringHandleMap_;
	UniquePtr<CompilerMetadataRegistration> registration_;

	void generateMetadataHeader();
	void generateStringTable();
	void generateTypeDefintions();
	void generateFieldDefinitions();
	void generateMethodDefinitions();
	void generateParameterDefinitions();
	void generateAssemblyDefinition();
	void generateImageDefinition();

	void generateReferenceTables();
	void generateInterfaceReferences();
	void generateNestedTypeReferences();
	void generateGenericParamReferences();
};

MRK_NS_END