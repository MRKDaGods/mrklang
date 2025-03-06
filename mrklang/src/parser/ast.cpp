#include "ast.h"
#include "common/utils.h"

#include <format>

#define Fmt(str, ...) std::format(str __VA_OPT__(,) __VA_ARGS__)

MRK_NS_BEGIN_MODULE(ast)

Str ParamDeclStmt::getSignature() {
	return Fmt("{}{} {}", isParams ? "params " : "", type->getTypeName(), name->name);
}

Str TypeReferenceExpr::getTypeName() const {
	Str result = formatCollection(identifiers, "::", [](const auto& item) { return item->name; });

	for (int i = 0; i < pointerRank; i++) {
		result += "*";
	}

	for (int i = 0; i < arrayRank; i++) {
		result += "[]";
	}

	if (!genericArgs.empty()) {
		result += Fmt("<{}>", formatCollection(genericArgs));
	}

	return result;
}

Str FuncDeclStmt::getSignature() const {
	// name(params) -> returnType
	Str signature = Fmt("{}(", name->name);
	if (!parameters.empty()) {
		signature += formatCollection(parameters,
			[](const auto& param) { return param->getSignature(); });
	}

	signature += Fmt(") -> {}", 
		returnType ? returnType->getTypeName() : "void");

	return signature;
}

// Implementation of toString methods

Str LiteralExpr::toString() const {
	return Fmt("LiteralExpr({})", value.lexeme);
}

Str InterpolatedStringExpr::toString() const {
	return Fmt("InterpolatedStringExpr([{}])",
		formatCollection(parts));
}

Str InteropCallExpr::toString() const {
	return Fmt("InteropCallExpr({}, {}, [{}])",
		targetLang, method->toString(), formatCollection(args));
}

Str IdentifierExpr::toString() const {
	return Fmt("IdentifierExpr({})", name);
}

Str TypeReferenceExpr::toString() const {
	return Fmt("TypeReferenceExpr({})", getTypeName());
}

Str CallExpr::toString() const {
	return Fmt("CallExpr({}, [{}])",
		target->toString(), formatCollection(arguments));
}

Str BinaryExpr::toString() const {
	return Fmt("BinaryExpr({}, {}, {})",
		left->toString(), op.lexeme, right->toString());
}

Str UnaryExpr::toString() const {
	return Fmt("UnaryExpr({}, {})",
		op.lexeme, right->toString());
}

Str TernaryExpr::toString() const {
	return Fmt("TernaryExpr({}, {}, {})",
		condition->toString(), thenBranch->toString(), elseBranch->toString());
}

Str AssignmentExpr::toString() const {
	Str result = Fmt("AssignmentExpr({}, {}",
		target->toString(), op.lexeme);

	if (op.type != TokenType::OP_INCREMENT && op.type != TokenType::OP_DECREMENT) {
		result += Fmt(", {}", value->toString());
	}

	result += ")";
	return result;
}

Str NamespaceAccessExpr::toString() const {
	return Fmt("NamespaceAccessExpr([{}])",
		formatCollection(path, "::"));
}

Str MemberAccessExpr::toString() const {
	return Fmt("MemberAccessExpr({}{}{})",
		target->toString(), op.lexeme, member->toString());
}

Str ArrayExpr::toString() const {
	return Fmt("ArrayExpr([{}])",
		formatCollection(elements));
}

Str ExprStmt::toString() const {
	return Fmt("ExprStmt({})", expr->toString());
}

Str VarDeclStmt::toString() const {
	Str result = Fmt("VarDeclStmt({}, {}",
		typeName ? typeName->toString() : "object",
		name->toString());

	if (initializer) {
		result += Fmt(", {}", initializer->toString());
	}

	result += ")";
	return result;
}

Str BlockStmt::toString() const {
	return Fmt("BlockStmt([\n{}])",
		formatCollection(statements, ";\n"));
}

Str ParamDeclStmt::toString() const {
	Str result = Fmt("ParamDeclStmt(");
	if (type) {
		result += Fmt("{}, ", type->toString());
	}

	result += Fmt("{}, ", name->toString());

	if (initializer) {
		result += Fmt("= {}", initializer->toString());
	}

	if (isParams) {
		result += ", params";
	}

	result += ")";
	return result;
}

Str FuncDeclStmt::toString() const {
	Str result = Fmt("FuncDeclStmt({}, [",
		name->toString());

	if (!parameters.empty()) {
		result += formatCollection(parameters);
	}

	result += Fmt("], ");

	if (returnType) {
		result += Fmt("{}, ", returnType->toString());
	}
	else {
		result += "void, ";
	}

	result += Fmt("{})", body->toString());
	return result;
}

Str IfStmt::toString() const {
	return Fmt("IfStmt({}, {}, {})",
		condition->toString(), thenBlock->toString(), elseBlock ? elseBlock->toString() : "null");
}

Str ForStmt::toString() const {
	return Fmt("ForStmt({}, {}, {}, {})",
		init ? init->toString() : "null",
		condition ? condition->toString() : "null",
		increment ? increment->toString() : "null",
		body->toString());
}

Str ForeachStmt::toString() const {
	return Fmt("ForeachStmt({}, {}, {})",
		variable->toString(), collection->toString(), body->toString());
}

Str WhileStmt::toString() const {
	return Fmt("WhileStmt({}, {})",
		condition->toString(), body->toString());
}

Str LangBlockStmt::toString() const {
	return Fmt("LangBlockStmt({}, {})",
		language, rawCode);
}

Str AccessModifierStmt::toString() const {
	return Fmt("AccessModifierStmt([{}])",
		formatCollection(modifiers));
}

Str NamespaceDeclStmt::toString() const {
	return Fmt("NamespaceDeclStmt({}, {})",
		formatCollection(path, "::"), body->toString());
}

Str DeclSpecStmt::toString() const {
	return Fmt("DeclSpecStmt({})", spec->toString());
}

Str UseStmt::toString() const {
	Str result = Fmt("UseStmt([{}]", formatCollection(paths, ", "));

	if (file) {
		result += Fmt(", {})", file->toString());
	}
	else {
		result += ")";
	}

	return result;
}

Str ReturnStmt::toString() const {
	if (value) {
		return Fmt("ReturnStmt({})", value->toString());
	}

	return "ReturnStmt()";
}

Str EnumDeclStmt::toString() const {
	Str result = Fmt("EnumDeclStmt({}, ", name->toString());

	if (type) {
		result += Fmt("{}, ", type->toString());
	}
	else {
		result += "int, ";
	}

	result += "[";

	if (!members.empty()) {
		bool first = true;
		for (const auto& member : members) {
			if (!first) {
				result += ", ";
			}
			first = false;

			result += Fmt("({}, {})",
				member.first->toString(),
				member.second ? member.second->toString() : "null");
		}
	}

	result += "])";
	return result;
}

Str TypeDeclStmt::toString() const {
	Str result = Fmt("TypeDeclStmt({}, {}, [",
		type.lexeme, name->toString());

	if (!aliases.empty()) {
		result += formatCollection(aliases);
	}

	result += "], [";

	if (!baseTypes.empty()) {
		result += formatCollection(baseTypes);
	}

	result += "], [";

	if (body) {
		result += body->toString();
	}

	result += "])";
	return result;
}

Str Program::toString() const {
	Str result = Fmt("Program({})", sourceFile->filename);

	if (!statements.empty()) {
		result += " [\n";
		for (const auto& stmt : statements) {
			result += stmt->toString();
		}
		result += "]\n";
	}

	return result;
}

// Implementation of accept methods

void LiteralExpr::accept(ASTVisitor& visitor) { visitor.visit(this); }
void InterpolatedStringExpr::accept(ASTVisitor& visitor) { visitor.visit(this); }
void InteropCallExpr::accept(ASTVisitor& visitor) { visitor.visit(this); }
void IdentifierExpr::accept(ASTVisitor& visitor) { visitor.visit(this); }
void TypeReferenceExpr::accept(ASTVisitor& visitor) { visitor.visit(this); }
void CallExpr::accept(ASTVisitor& visitor) { visitor.visit(this); }
void BinaryExpr::accept(ASTVisitor& visitor) { visitor.visit(this); }
void UnaryExpr::accept(ASTVisitor& visitor) { visitor.visit(this); }
void TernaryExpr::accept(ASTVisitor& visitor) { visitor.visit(this); }
void AssignmentExpr::accept(ASTVisitor& visitor) { visitor.visit(this); }
void NamespaceAccessExpr::accept(ASTVisitor& visitor) { visitor.visit(this); }
void MemberAccessExpr::accept(ASTVisitor& visitor) { visitor.visit(this); }
void ArrayExpr::accept(ASTVisitor& visitor) { visitor.visit(this); }

void ExprStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void VarDeclStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void BlockStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void ParamDeclStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void FuncDeclStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void IfStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void ForStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void ForeachStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void WhileStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void LangBlockStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void AccessModifierStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void NamespaceDeclStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void DeclSpecStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void UseStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void ReturnStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void EnumDeclStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }
void TypeDeclStmt::accept(ASTVisitor& visitor) { visitor.visit(this); }

MRK_NS_END