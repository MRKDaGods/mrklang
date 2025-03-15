#include "cpp_generator.h"
#include "function_generator.h"

#include <algorithm>

MRK_NS_BEGIN_MODULE(codegen)

CppGenerator::CppGenerator(const SymbolTable* symbolTable) : symbolTable_(symbolTable) {}

Str CppGenerator::generateRuntimeCode() {
	code_.clear();

	// Generate includes
	code_ << "#include \"runtime.h\"\n";

	// Begin namespace
	code_ << "MRK_NS_BEGIN_MODULE(runtime::generated)\n\n";

	// Generate types
	for (const auto& type : symbolTable_->getTypes()) {
		generateType(type);
	}

	// Generate functions
	auto globalFunction = symbolTable_->getGlobalFunction();
	for (const auto& function : symbolTable_->getFunctions()) {
		// Defer global function
		if (function == globalFunction) continue;

		generateFunction(function);
	}

	// Generate global function
	generateFunction(globalFunction);

	// End namespace
	code_ << "MRK_NS_END\n";

	return code_.str();
}

Str CppGenerator::translateTypeName(const Str& typeName) const {
	// Replace all : with _
	Str result = typeName;
	std::replace(result.begin(), result.end(), ':', '_');
	return result;
}

Str CppGenerator::getReferenceTypeName(const TypeSymbol* type) const {
	auto it = nameMap_.find(type);
	if (it == nameMap_.end() || !it->first) {
		return "ERROR";
	}

	// Ptr for reference types
	// Normal for value types
	Str result = it->second;
	if (detail::hasFlag(type->kind, SymbolKind::CLASS | SymbolKind::INTERFACE)) {
		result += "*";
	}

	return result;
}

Str CppGenerator::getMappedName(const Symbol* symbol) const {
	auto it = nameMap_.find(symbol);
	if (it == nameMap_.end() || !it->first) {
		return "ERROR";
	}

	return it->second;
}

void CppGenerator::generateType(const TypeSymbol* type) {
	auto generatedTypeName = utils::concat(translateTypeName(type->qualifiedName), "_", (uintptr_t)type);
	nameMap_[type] = generatedTypeName;

	// Generate type declaration
	writeLine("// Type: ", type->qualifiedName);
	writeLine("struct ", generatedTypeName, " {");

	// Generate members
	indentLevel_++;
	for (const auto& [_, member] : type->members) {
		// Generate only variables
		if (member->kind != SymbolKind::VARIABLE) {
			continue;
		}

		generateVariable(static_cast<const VariableSymbol*>(member.get()));
	}
	indentLevel_--;

	writeLine("};");
}

void CppGenerator::generateFunction(const FunctionSymbol* function) {
	auto generatedName = utils::concat(function->name, "_", (uintptr_t)function);
	nameMap_[function] = generatedName;

	// Generate function declaration
	writeLine("// Function: ", function->qualifiedName);

	Str params = utils::formatCollection(function->parameters, ", ", [&](const auto& param) {
		auto generatedParamName = utils::concat(param.second->name, "_", (uintptr_t)param.second.get());
		nameMap_[param.second.get()] = generatedParamName;

		return utils::concat(getReferenceTypeName(param.second->resolver.type), ' ', generatedParamName);
	});

	// Should we include an instance parameter?
	// Add an instance parameter for the enclosing type
	if (!function->isGlobal && !detail::isSTATIC(function->accessModifier)) {
		auto enclosingType = static_cast<TypeSymbol*>(symbolTable_->findAncestorOfKind(function, SymbolKind::TYPE));

		auto instanceParam = utils::concat(getReferenceTypeName(enclosingType), " instance");
		params = params.empty() ? instanceParam : utils::concat(instanceParam, params);
	}
	else {
		// Add static keyword
		write<true>("static ");
	}

	writeLine(getReferenceTypeName(function->resolver.returnType), ' ', generatedName, '(', params, ") {");

	// Generate function body
	indentLevel_++;

	FunctionGenerator generator(this, symbolTable_);
	generator.generateFunctionBody(function);

	indentLevel_--;

	writeLine("}");
}

void CppGenerator::generateVariable(const VariableSymbol* variable) {
	auto generatedName = utils::concat(variable->name, "_", (uintptr_t)variable);
	nameMap_[variable] = generatedName;

	// Generate variable declaration
	writeLine("// Variable: ", variable->name);
	writeLine(getReferenceTypeName(variable->resolver.type), ' ', generatedName, ";");
}

MRK_NS_END
