#include "code_generator.h"
#include "function_generator.h"
#include "common/declspecs.h"

#include <algorithm>
#include <ranges>

MRK_NS_BEGIN_MODULE(codegen)

CodeGenerator::CodeGenerator(const SymbolTable* symbolTable, const CompilerMetadataRegistration* metadataRegistration)
	: symbolTable_(symbolTable), metadataRegistration_(metadataRegistration) {}

Str CodeGenerator::generateRuntimeCode() {
	code_.clear();

	// Generate includes
	writeLine("#include \"runtime.h\"");
	writeLine("#include \"runtime_defines.h\"");

	// Write rigid c++ blocks
	for (const auto& block : symbolTable_->getRigidLanguageBlocks()) {
		writeLine("// Rigid block: ", block->language, (uintptr_t)block);
		writeLine(block->rawCode);
	}

	// Begin namespace
	writeLine("MRK_NS_BEGIN_MODULE(runtime::generated)\n");

	// Forward declare all types
	generateForwardDeclarations();

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

void CodeGenerator::generateForwardDeclarations() {
	writeLine("// Forward declarations");

	for (const auto& type : symbolTable_->getTypes()) {
		if (symbolTable_->getTypeSystem()->isPrimitiveType(type)) {
			continue;
		}

		auto generatedTypeName = utils::concat(translateTypeName(type->qualifiedName), "_", (uintptr_t)type);
		nameMap_[type] = generatedTypeName;

		writeLine("struct ", generatedTypeName, ";");
	}
}

Str CodeGenerator::getReferenceTypeName(const TypeSymbol* type) const {
	auto it = nameMap_.find(type);
	if (it == nameMap_.end() || !it->first) {
		return "ERROR";
	}

	// Ptr for reference types
	// Normal for value types
	Str result = it->second;
	if (!type->isGenericParameter && detail::hasFlag(type->kind, SymbolKind::CLASS | SymbolKind::INTERFACE)) {
		result += "*";
	}

	return result;
}

Str CodeGenerator::getMappedName(const Symbol* symbol) {
	if (!symbol) {
		return "ERROR";
	}
	
	auto it = nameMap_.find(symbol);
	if (it == nameMap_.end()) {
		auto generatedName = utils::concat(symbol->name, "_", (uintptr_t)symbol);
		nameMap_[symbol] = generatedName;
		return generatedName;
	}

	return it->second;
}

void CodeGenerator::setMappedName(const Symbol* symbol, const Str& name) {
	nameMap_[symbol] = name;
}

void CodeGenerator::generateType(const TypeSymbol* type) {
	// Skip primitives
	if (symbolTable_->getTypeSystem()->isPrimitiveType(type)) {
		nameMap_[type] = utils::concat("__mrkprimitive_", type->name);
		return;
	}

	// Generate type declaration
	writeLine("// Type: ", type->qualifiedName, ", Token: ", metadataRegistration_->typeTokenMap.at(type));
	writeLine("struct ", getMappedName(type), " {");

	// Generate members
	indentLevel_++;
	for (const auto& [_, member] : type->members) {
		// Generate only variables
		if (member->kind == SymbolKind::VARIABLE) {
			generateVariable(static_cast<const VariableSymbol*>(member.get()), type);
		}
		else if (member->kind == SymbolKind::FUNCTION) {
			// Generate function declaration
			generateFunctionDeclaration(static_cast<const FunctionSymbol*>(member.get()), false);
			writeLine<false>(";");
		}
	}
	indentLevel_--;

	writeLine("};");
}

void CodeGenerator::generateFunctionDeclaration(const FunctionSymbol* function, bool external, Vec<Str>* paramNames) {
	// Generate function declaration
	writeLine("// Function: ", function->qualifiedName, ", Token: ", metadataRegistration_->methodTokenMap.at(function));

	if (!function->genericParameters.empty()) {
		write("template<");
		write(utils::formatCollection(function->genericParameters, ", ", [&](const auto& param) {
			auto unmangledName = param->name;
			nameMap_[param.get()] = unmangledName;

			return utils::concat("typename ", unmangledName);
		}));

		writeLine<false>(">");
	}


	Vec<FunctionParameterSymbol*> params;
	std::transform(function->parameters.begin(), function->parameters.end(), std::back_inserter(params),
		[](const auto& pair) { return pair.second.get(); });

	Str paramsStr = utils::formatCollection(params, ", ", [&](const auto& param) {
		// Do not mangle param names if they start with __
		auto generatedParamName = param->name.starts_with("__") ? 
			param->name : utils::concat(param->name, "_", (uintptr_t)param);

		nameMap_[param] = generatedParamName;

		if (paramNames) {
			paramNames->push_back(generatedParamName);
		}

		// if param type is generic, pass as is
		auto typeStr = getReferenceTypeName(param->resolver.type);
		if (param->resolver.type->isGenericParameter) {
			typeStr = param->resolver.type->name;
		}

		return utils::concat(typeStr, ' ', generatedParamName);
	});

	auto enclosingType = static_cast<TypeSymbol*>(symbolTable_->findAncestorOfKind(function, SymbolKind::TYPE));

	// Should we include an instance parameter?
	// Add an instance parameter for the enclosing type
	if (!function->isGlobal && !detail::isSTATIC(function->accessModifier)) {
		
		auto instanceParam = utils::concat(getReferenceTypeName(enclosingType), " __instance");
		paramsStr = paramsStr.empty() ? instanceParam : utils::concat(instanceParam, ", ", paramsStr);
	}
	
	auto generatedName = getMappedName(function);
	if (external) {
		// include owner
		generatedName = utils::concat(getMappedName(enclosingType), "::", generatedName);
	}
	else {
		write("static ");
	}

	auto returnTypeName = getReferenceTypeName(function->resolver.returnType);
	if (function->resolver.returnType->isGenericParameter) {
		returnTypeName = function->resolver.returnType->name;
	}

	write(returnTypeName, ' ', generatedName, '(', paramsStr, ")");
}

void CodeGenerator::generateFunction(const FunctionSymbol* function) {
	Vec<Str> paramNames;
	generateFunctionDeclaration(function, true, &paramNames);
	writeLine(" {");

	// Mangle variable names
	for (const auto& [_, var] : function->members) {
		if (var->kind != SymbolKind::VARIABLE) {
			continue;
		}

		auto* variable = static_cast<const VariableSymbol*>(var.get());
		auto generatedVarName = variable->declSpec == DECLSPEC_MAPPED ? 
			variable->name : utils::concat(variable->name, "_", (uintptr_t)variable);
		nameMap_[variable] = generatedVarName;
	}

	// Generate function body
	indentLevel_++;

	if (function->declSpec == DECLSPEC_NATIVE) {
		writeLine("// Native function: ", function->qualifiedName);
		
		if (function->resolver.returnType->name != "void") {
			write("return ");

			// Gotta cast to the return type
			write('(', getReferenceTypeName(function->resolver.returnType), ')');
		}

		write("MRK_INVOKE_ICALL(", metadataRegistration_->methodTokenMap.at(function));

		if (!paramNames.empty()) {
			write(", ", utils::formatCollection(paramNames, ", ", [](const auto& param) { return param; }));
		}

		writeLine<false>(");");
	}
	else {
		// Generate function body
		FunctionGenerator generator(this, symbolTable_);
		generator.generateFunctionBody(function);
	}
	indentLevel_--;

	writeLine("}");
}

void CodeGenerator::generateVariable(const VariableSymbol* variable, const TypeSymbol* enclosingType) {
	auto generatedName = utils::concat(variable->name, "_", (uintptr_t)variable);
	nameMap_[variable] = generatedName;

	// Generate variable declaration
	writeLine("\n// Variable: ", variable->name, ", Token: ", metadataRegistration_->fieldTokenMap.at(variable));

	if (detail::isSTATIC(variable->accessModifier)) {
		write("static ");

		// Add to static fields
		staticFields_.emplace_back(variable, enclosingType, "");
	}

	writeLine(getReferenceTypeName(variable->resolver.type), ' ', generatedName, ";");
}

void CodeGenerator::generateStaticFieldInitializers() {
	for (auto& [staticField, enclosingType, nativeInitializerMethod] : staticFields_) {
		writeLine("\n// Static field initializer: ", staticField->qualifiedName);

		auto mappedTypeName = getReferenceTypeName(staticField->resolver.type);
		
		nativeInitializerMethod = utils::concat("staticFieldInit_", (uintptr_t)staticField);
		writeLine(mappedTypeName, ' ', nativeInitializerMethod, "() {");
		indentLevel_++;

		// Generate initializer code
		FunctionGenerator generator(this, symbolTable_);
		generator.generateFieldInitializer(staticField, enclosingType);

		indentLevel_--;
		writeLine("}");

		writeLine(mappedTypeName, ' ', getMappedName(enclosingType), "::", getMappedName(staticField), " = ", nativeInitializerMethod, "();");
	}
}

void CodeGenerator::generateMetadataRegistration() {
	writeLine("\n// Metadata registration");
	writeLine("void registerMetadata() {");
	indentLevel_++;

	// Register native methods
	writeLine("// Register native methods");
	for (const auto& [method, token] : metadataRegistration_->methodTokenMap) {
		// For not, dont register method if it has generic parameters
		if (!method->genericParameters.empty()) {
			continue;
		}

		auto enclosingType = static_cast<TypeSymbol*>(symbolTable_->findAncestorOfKind(method, SymbolKind::TYPE));
		writeLine("MRK_RUNTIME_REGISTER_CODE(", token, ", ", 
			getMappedName(enclosingType), "::", getMappedName(method), ");");
	}

	// Register types
	writeLine("\n// Register types");

	for (const auto& [type, token] : metadataRegistration_->typeTokenMap) {
		writeLine("MRK_RUNTIME_REGISTER_TYPE(", token, ", ", getMappedName(type), ");");
	}

	writeLine("\n// Register fields");
	// Register fields
	for (const auto& [field, token] : metadataRegistration_->fieldTokenMap) {
		if (detail::isSTATIC(field->accessModifier)) continue;

		auto enclosingType = static_cast<TypeSymbol*>(symbolTable_->findAncestorOfKind(field, SymbolKind::TYPE));
		writeLine("MRK_RUNTIME_REGISTER_FIELD(", token, ", offsetof(", getMappedName(enclosingType), ", ", getMappedName(field), "));");
	}

	// Register static fields

	// Until we actually implement a full object model
	// lets just initialize static fields in the global function

	writeLine("\n// Register static fields");
	for (const auto& staticField : staticFields_) {
		auto token = metadataRegistration_->fieldTokenMap.at(staticField.variable);
		auto mappedTypeName = getMappedName(staticField.enclosingType);
		auto mappedFieldName = getMappedName(staticField.variable);

		// Token - Native Field - Static Init
		writeLine("MRK_RUNTIME_REGISTER_STATIC_FIELD(", token, ", ", mappedTypeName, "::", mappedFieldName, ", ", staticField.nativeInitializerMethod, ");");
		writeLine(mappedTypeName, "::", mappedFieldName, " = ", staticField.nativeInitializerMethod, "();");
	}

	indentLevel_--;

	writeLine("}");
}

MRK_NS_END
