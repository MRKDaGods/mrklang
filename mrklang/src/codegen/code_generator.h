#pragma once

#include "common/types.h"
#include "semantic/symbol_table.h"
#include "common/utils.h"
#include "metadata_writer.h"

#include <sstream>

MRK_NS_BEGIN_MODULE(codegen)

using namespace semantic;

struct StaticFieldInfo {
	const VariableSymbol* variable;
	const TypeSymbol* enclosingType;
	Str nativeInitializerMethod;
};

class CodeGenerator {
public:
	CodeGenerator(const SymbolTable* symbolTable, const CompilerMetadataRegistration* metadataRegistration);

	Str generateRuntimeCode();
	Str getReferenceTypeName(const TypeSymbol* type) const;
	Str getMappedName(const Symbol* symbol);

	void setMappedName(const Symbol* symbol, const Str& name);

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
	const CompilerMetadataRegistration* metadataRegistration_;

	// Static Fields, and their enclosing types
	Vec<StaticFieldInfo> staticFields_;

	Str translateTypeName(const Str& typeName) const;
	void generateForwardDeclarations();
	void generateType(const TypeSymbol* type);
	void generateFunctionDeclaration(const FunctionSymbol* function, bool external, Vec<Str>* paramNames = nullptr);
	void generateFunction(const FunctionSymbol* function);
	void generateVariable(const VariableSymbol* variable, const TypeSymbol* enclosingType);
	void generateStaticFieldInitializers();
	void generateMetadataRegistration();
};

MRK_NS_END