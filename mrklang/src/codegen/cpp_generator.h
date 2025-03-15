#pragma once

#include "common/types.h"
#include "semantic/symbol_table.h"
#include "common/utils.h"

#include <sstream>

MRK_NS_BEGIN_MODULE(codegen)

using namespace semantic;

class CppGenerator {
public:
	CppGenerator(const SymbolTable* symbolTable);

	Str generateRuntimeCode();
	Str getReferenceTypeName(const TypeSymbol* type) const;
	Str getMappedName(const Symbol* symbol) const;

	template<bool indent = false>
	void write(const Str& line) {
		if constexpr (indent) {
			code_ << Str(indentLevel_ * 4, ' ');
		}

		code_ << line;
	}

	template <bool indent, typename... Args>
	void write(Args... args) {
		write<indent>(utils::concat(args...));
	}

	template<typename... Args>
	void write(Args... args) {
		write<false>(utils::concat(args...));
	}

	template<bool indent = false>
	void writeLine(const Str& line) {
		write<indent>(line, '\n');
	}

	template <bool indent, typename... Args>
	void writeLine(Args... args) {
		writeLine<indent>(utils::concat(args...));
	}

	template <typename... Args>
	void writeLine(Args... args) {
		writeLine<true>(utils::concat(args...));
	}

	void indent() { indentLevel_++; }
	void unindent() { indentLevel_--; }

private:
	const SymbolTable* symbolTable_;
	std::stringstream code_;
	int indentLevel_ = 0;
	Dict<const Symbol*, Str> nameMap_;

	Str translateTypeName(const Str& typeName) const;
	void generateType(const TypeSymbol* type);
	void generateFunction(const FunctionSymbol* function);
	void generateVariable(const VariableSymbol* variable);
};

MRK_NS_END