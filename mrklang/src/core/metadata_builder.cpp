#include "metadata_builder.h"

MRK_NS_BEGIN

using namespace runtime::metadata;

MetadataBuilder::MetadataBuilder(const SymbolTable& symbolTable)
	: symbolTable_(symbolTable), globalNamespaceType_(nullptr) {}

void MetadataBuilder::build() {
	// Create image
	image_ = MakeUnique<Image>();
	image_->name = "mrklang-img-test-v1";

	// Process global namespace
	auto globalNamespace = symbolTable_.getGlobalNamespace();
	processNamespace(globalNamespace);

	int xzv = 0;
}

Str MetadataBuilder::translateScopeToTypename(const Str& qualifiedName) {
	// Replace all :: with ___
	Str result = qualifiedName;
	size_t pos = 0;
	
	while ((pos = result.find("::", pos)) != Str::npos) {
		result.replace(pos, 2, "__");
		pos += 2;
	}

	return result;
}

void MetadataBuilder::processNamespace(const NamespaceSymbol* symbol) {
	// Ensure our namespace has its own Type
	auto translatedName = translateScopeToTypename(symbol->qualifiedName);
	
	Type* typePtr = nullptr;
	auto type = types_.find(translatedName);
	if (type != types_.end()) {
		typePtr = type->second;
	}
	else {
		// Create new nms type
		auto nmsType = createNamespaceType(symbol);
		typePtr = nmsType.get();
		types_[translatedName] = typePtr;

		// Add to image
		image_->types.push_back(Move(nmsType));

		// Is this our global namespace?
		if (translatedName == "__global") {
			globalNamespaceType_ = typePtr;
		}
	}

	// TODO: use visitor pattern?
	for (const auto& [_, symbol] : symbol->members) {
		processSymbol(symbol.get());
	}

	// Recurse into child namespaces
	for (const auto& [_, childNs] : symbol->namespaces) {
		processNamespace(childNs);
	}
}

void MetadataBuilder::processSymbol(const Symbol* symbol) {
	switch (symbol->kind) {
		case SymbolKind::NAMESPACE:
			processNamespace(dynamic_cast<const NamespaceSymbol*>(symbol));
			break;

		case SymbolKind::TYPE:
		case SymbolKind::CLASS:
		case SymbolKind::STRUCT:
		case SymbolKind::INTERFACE:
			processType(dynamic_cast<const TypeSymbol*>(symbol));
			break;

		case SymbolKind::FUNCTION:

			break;
	}
}

void MetadataBuilder::processType(const TypeSymbol* symbol) {
	auto type = MakeUnique<Type>();
	type->name = translateScopeToTypename(symbol->qualifiedName);

	if (detail::hasFlag(symbol->kind, SymbolKind::CLASS)) {
		type->traits.isClass = true;
	}
	else if (detail::hasFlag(symbol->kind, SymbolKind::STRUCT)) {
		type->traits.isStruct = true;
	}
	else if (detail::hasFlag(symbol->kind, SymbolKind::INTERFACE)) {
		type->traits.isInterface = true;
	}

	// Add to image
	image_->types.push_back(Move(type));
}

UniquePtr<Type> MetadataBuilder::createNamespaceType(const NamespaceSymbol* symbol) {
	auto type = MakeUnique<Type>();
	type->name = translateScopeToTypename(symbol->qualifiedName);
	type->traits.isNamespace = true;

	return type;
}

MRK_NS_END
