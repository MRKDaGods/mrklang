#include "metadata_builder.h"

MRK_NS_BEGIN

using namespace runtime::metadata;

MetadataBuilder::MetadataBuilder(const SymbolTable& symbolTable)
	: symbolTable_(symbolTable), globalNamespaceType_(nullptr) {}

void MetadataBuilder::build() {
	// Create image
	image_ = MakeUnique<Image>();
	image_->name = "mrklang-img-test-v1";
}

Str MetadataBuilder::translateScopeToTypename(const Str& qualifiedName) {
	// Replace all :: with ___
	Str result = qualifiedName;
	size_t pos = 0;
	
	while ((pos = result.find("::", pos)) != Str::npos) {
		result.replace(pos, 2, "___");
		pos += 3;
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
		if (translatedName == "___global") {
			globalNamespaceType_ = typePtr;
		}
	}

	// TODO: use visitor pattern?
	for (const auto& [_, symbol] : symbol->members) {
		processSymbol(symbol.get());
	}
}

void MetadataBuilder::processSymbol(const Symbol* symbol) {
	switch (symbol->kind) {
		case SymbolKind::NAMESPACE:
			processNamespace(dynamic_cast<const NamespaceSymbol*>(symbol));
			break;

		case SymbolKind::FUNCTION:

			break;
	}
}

UniquePtr<Type> MetadataBuilder::createNamespaceType(const NamespaceSymbol* symbol) {
	auto type = MakeUnique<Type>();
	type->name = translateScopeToTypename(symbol->qualifiedName);
	type->parent = nullptr; // TODO: make them a hierarchy?
	type->traits.isNamespace = true;

	return type;
}

MRK_NS_END
