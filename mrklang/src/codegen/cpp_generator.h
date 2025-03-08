#pragma once

#include "common/types.h"
#include "semantic/symbol_table.h"

MRK_NS_BEGIN

using namespace semantic;

class CppGenerator {
public:
	CppGenerator(const SymbolTable& symbolTable);

private:
	const SymbolTable& symbolTable_;
};

MRK_NS_END