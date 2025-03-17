#include "code_generator.h"
#include "function_generator.h"

#include <algorithm>

MRK_NS_BEGIN_MODULE(codegen)

CodeGenerator::CodeGenerator(const SymbolTable* symbolTable, const CompilerMetadataRegistration* metadataRegistration)
	: symbolTable_(symbolTable), metadataRegistration_(metadataRegistration) {}

Str CodeGenerator::generateRuntimeCode() {
	code_.clear();

	// Generate includes
	writeLine("#include \"runtime.h\"");
	writeLine("#include \"runtime_defines.h\"");

	// Begin namespace
	writeLine("MRK_NS_BEGIN_MODULE(runtime::generated)\n");

	// Write rigid c++ blocks
	for (const auto& block : symbolTable_->getRigidLanguageBlocks()) {
		writeLine("// Rigid block: ", block->language, (uintptr_t)block);
		writeLine(block->rawCode);
	}

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

	// Generate static field initializers
	generateStaticFieldInitializers();

	// Generate metadata registration
	generateMetadataRegistration();

	// End namespace
	writeLine("MRK_NS_END");

	return code_.str();
}

Str CodeGenerator::translateTypeName(const Str& typeName) const {
	// Replace all : with _
	Str result = typeName;
	std::replace(result.begin(), result.end(), ':', '_');
	return result;
}

Str CodeGenerator::getReferenceTypeName(const TypeSymbol* type) const {
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

Str CodeGenerator::getMappedName(const Symbol* symbol) const {
	auto it = nameMap_.find(symbol);
	if (it == nameMap_.end() || !it->first) {
		return "ERROR";
	}

	return it->second;
}

void CodeGenerator::generateType(const TypeSymbol* type) {
	auto generatedTypeName = utils::concat(translateTypeName(type->qualifiedName), "_", (uintptr_t)type);
	nameMap_[type] = generatedTypeName;

	// Generate type declaration
	writeLine("// Type: ", type->qualifiedName, ", Token: ", metadataRegistration_->typeTokenMap.at(type));
	writeLine("struct ", generatedTypeName, " {");

	// Generate members
	indentLevel_++;
	for (const auto& [_, member] : type->members) {
		// Generate only variables
		if (member->kind != SymbolKind::VARIABLE) {
			continue;
		}

		generateVariable(static_cast<const VariableSymbol*>(member.get()), type);
	}
	indentLevel_--;

	writeLine("};");
}

void CodeGenerator::generateFunction(const FunctionSymbol* function) {
	auto generatedName = utils::concat(function->name, "_", (uintptr_t)function);
	nameMap_[function] = generatedName;

	// Generate function declaration
	writeLine("// Function: ", function->qualifiedName, ", Token: ", metadataRegistration_->methodTokenMap.at(function));

	Str params = utils::formatCollection(function->parameters, ", ", [&](const auto& param) {
		auto generatedParamName = utils::concat(param.second->name, "_", (uintptr_t)param.second.get());
		nameMap_[param.second.get()] = generatedParamName;

		return utils::concat(getReferenceTypeName(param.second->resolver.type), ' ', generatedParamName);
	});

	// Should we include an instance parameter?
	// Add an instance parameter for the enclosing type
	if (!function->isGlobal && !detail::isSTATIC(function->accessModifier)) {
		auto enclosingType = static_cast<TypeSymbol*>(symbolTable_->findAncestorOfKind(function, SymbolKind::TYPE));

		auto instanceParam = utils::concat(getReferenceTypeName(enclosingType), " __instance");
		params = params.empty() ? instanceParam : utils::concat(instanceParam, params);
	}

	writeLine(getReferenceTypeName(function->resolver.returnType), ' ', generatedName, '(', params, ") {");

	// Generate function body
	indentLevel_++;

	FunctionGenerator generator(this, symbolTable_);
	generator.generateFunctionBody(function);

	indentLevel_--;

	writeLine("}");
}

void CodeGenerator::generateVariable(const VariableSymbol* variable, const TypeSymbol* enclosingType) {
	auto generatedName = utils::concat(variable->name, "_", (uintptr_t)variable);
	nameMap_[variable] = generatedName;

	// Generate variable declaration
	writeLine("// Variable: ", variable->name, ", Token: ", metadataRegistration_->fieldTokenMap.at(variable));

	if (detail::isSTATIC(variable->accessModifier)) {
		write("static ");

		// Add to static fields
		staticFields_.emplace_back(variable, enclosingType, "");
	}

	writeLine(getReferenceTypeName(variable->resolver.type), ' ', generatedName, ";");
}

void CodeGenerator::generateStaticFieldInitializers() {
	for (auto& [staticField, enclosingType, nativeInitializerMethod] : staticFields_) {
		writeLine("// Static field initializer: ", staticField->qualifiedName);
		
		nativeInitializerMethod = utils::concat("staticFieldInit_", (uintptr_t)staticField);
		writeLine("void ", nativeInitializerMethod,"() {");
		indentLevel_++;

		// Generate initializer code
		FunctionGenerator generator(this, symbolTable_);
		generator.generateFieldInitializer(staticField, enclosingType);

		indentLevel_--;
		writeLine("}");
	}
}

void CodeGenerator::generateMetadataRegistration() {
	writeLine("\n// Metadata registration");
	writeLine("void registerMetadata() {");
	indentLevel_++;

	// Register native methods
	writeLine("// Register native methods");
	for (const auto& [method, token] : metadataRegistration_->methodTokenMap) {
		writeLine("MRK_RUNTIME_REGISTER_CODE(", token, ", ", getMappedName(method), ");");
	}

	// Register types
	writeLine("// Register types");

	for (const auto& [type, token] : metadataRegistration_->typeTokenMap) {
		writeLine("MRK_RUNTIME_REGISTER_TYPE(", token, ", ", getMappedName(type), ");");
	}

	// Register static fields
	writeLine("// Register static fields");
	for (const auto& staticField : staticFields_) {
		auto token = metadataRegistration_->fieldTokenMap.at(staticField.variable);
		auto mappedTypeName = getMappedName(staticField.enclosingType);
		auto mappedFieldName = getMappedName(staticField.variable);

		// Token - Native Field - Static Init
		writeLine("MRK_RUNTIME_REGISTER_STATIC_FIELD(", token, ", ", mappedTypeName, "::", mappedFieldName, ", ", staticField.nativeInitializerMethod, ");");
	}

	indentLevel_--;

	writeLine("}");
}

MRK_NS_END
