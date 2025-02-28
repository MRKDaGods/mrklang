#pragma once

#include "common/macros.h"
#include "lexer/token.h"

#include <memory>
#include <vector>
#include <string>
#include <sstream>

MRK_NS_BEGIN_MODULE(ast)

/// Base node for expressions and statements
struct Node {
	/// Mapping ast to source code
	Token&& startToken;

	Node(Token&& startToken) : startToken(Move(startToken)) {
	}

	virtual ~Node() = default;
	virtual Str toString() const = 0;
};

/// Expressions
struct Expression : Node {
	Expression(Token startToken) : Node(Move(startToken)) { // receive a copy(incase of lval) then move it to node
	}

	virtual Str toString() const override = 0;
};

struct Literal : Expression {
	Token value;

	Literal(Token val) : Expression(val), value(Move(val)) {
	}

	Str toString() const override {
		return "Literal(" + value.lexeme + ")";
	}
};

struct InterpolatedString : Expression {
	Vec<UniquePtr<Expression>> parts;

	InterpolatedString(Token&& start, Vec<UniquePtr<Expression>>&& parts) : Expression(Move(start)), parts(Move(parts)) {
	}

	Str toString() const override {
		Str result = "InterpolatedString([";
		for (const auto& part : parts) {
			result += part->toString() + ", ";
		}
		if (!parts.empty()) {
			result.pop_back(); // Remove trailing comma
			result.pop_back(); // Remove trailing space
		}
		result += "])";
		return result;
	}
};

struct InteropCall : Expression {
	Str targetLang;
	UniquePtr<Expression> method;
	Vec<UniquePtr<Expression>> args;

	InteropCall(Token&& start, Str targetLang, UniquePtr<Expression> method, Vec<UniquePtr<Expression>>&& args)
		: Expression(Move(start)), targetLang(Move(targetLang)), method(Move(method)), args(Move(args)) {
	}

	Str toString() const override {
		Str result = "InteropCall(" + targetLang + ", " + method->toString() + ", [";
		for (const auto& arg : args) {
			result += arg->toString() + ", ";
		}
		if (!args.empty()) {
			result.pop_back(); // Remove trailing comma
			result.pop_back(); // Remove trailing space
		}
		result += "])";
		return result;
	}
};

struct Identifier : Expression {
	Str name;

	// Constructor
	Identifier(Token tok) : Expression(tok), name(Move(tok.lexeme)) {
	}

	Str toString() const override {
		return "Identifier(" + name + ")";
	}
};

struct TypeName : Expression { // INT***[][]
	Vec<UniquePtr<Identifier>> identifiers; // ["csharp", "System", "Int32"] where nms=csharp::System
	Vec<UniquePtr<TypeName>> genericArgs; // generics
	int pointerRank;
	int arrayRank;

	TypeName(Token&& start, Vec<UniquePtr<Identifier>>&& identifiers, Vec<UniquePtr<TypeName>>&& genericArgs, int&& pointerRank, int&& arrayRank)
		: Expression(Move(start)), identifiers(Move(identifiers)), genericArgs(Move(genericArgs)), pointerRank(Move(pointerRank)), arrayRank(Move(arrayRank)) {}

	Str toString() const override {
		Str result = "TypeName([";
		for (const auto& id : identifiers) {
			result += id->toString() + ", ";
		}

		if (!identifiers.empty()) {
			result.pop_back(); // Remove trailing comma
			result.pop_back(); // Remove trailing space
		}

		result += "](";

		for (int i = 0; i < pointerRank; i++) {
			result += "*";
		}

		for (int i = 0; i < arrayRank; i++) {
			result += "[]";
		}

		result += "), [";
		for (const auto& arg : genericArgs) {
			result += arg->toString() + ", ";
		}

		if (!genericArgs.empty()) {
			result.pop_back(); // Remove trailing comma
			result.pop_back(); // Remove trailing space
		}

		result += "])";
		return result;
	}
};

struct FunctionCall : Expression {
	UniquePtr<Expression> target;
	Vec<UniquePtr<Expression>> arguments;

	FunctionCall(Token&& start, UniquePtr<Expression>&& target, Vec<UniquePtr<Expression>>&& arguments)
		: Expression(Move(start)), target(Move(target)), arguments(Move(arguments)) {}

	Str toString() const override {
		Str result = "FunctionCall(" + target->toString() + ", [";
		for (const auto& arg : arguments) {
			result += arg->toString() + ", ";
		}
		if (!arguments.empty()) {
			result.pop_back(); // Remove trailing comma
			result.pop_back(); // Remove trailing space
		}
		result += "])";
		return result;
	}
};

struct BinaryExpr : Expression {
	UniquePtr<Expression> left;
	Token op;
	UniquePtr<Expression> right;

	BinaryExpr(Token&& start, UniquePtr<Expression> left, Token op, UniquePtr<Expression> right)
		: Expression(Move(start)), left(Move(left)), op(op), right(Move(right)) {
	}

	Str toString() const override {
		return "BinaryExpr(" + left->toString() + ", " + op.lexeme + ", " + right->toString() + ")";
	}
};

struct UnaryExpr : Expression {
	Token op;
	UniquePtr<Expression> right;

	UnaryExpr(Token&& start, Token op, UniquePtr<Expression> right)
		: Expression(Move(start)), op(op), right(Move(right)) {
	}

	Str toString() const override {
		return "UnaryExpr(" + op.lexeme + ", " + right->toString() + ")";
	}
};

struct TernaryExpr : Expression {
	UniquePtr<Expression> condition;
	UniquePtr<Expression> thenBranch;
	UniquePtr<Expression> elseBranch;

	TernaryExpr(Token&& start, UniquePtr<Expression> condition, UniquePtr<Expression> thenBranch, UniquePtr<Expression> elseBranch)
		: Expression(Move(start)), condition(Move(condition)), thenBranch(Move(thenBranch)), elseBranch(Move(elseBranch)) {
	}

	Str toString() const override {
		return "TernaryExpr(" + condition->toString() + ", " + thenBranch->toString() + ", " + elseBranch->toString() + ")";
	}
};

struct AssignmentExpr : Expression {
	UniquePtr<Expression> target;
	Token op;
	UniquePtr<Expression> value;

	AssignmentExpr(Token&& start, UniquePtr<Expression> target, Token op, UniquePtr<Expression> value)
		: Expression(Move(start)), target(Move(target)), op(op), value(Move(value)) {
	}

	Str toString() const override {
		Str result = "AssignmentExpr(";
		result += target->toString();
		result += ", " + op.lexeme;

		if (op.type != TokenType::OP_INCREMENT && op.type != TokenType::OP_DECREMENT) {
			result += ", " + value->toString();
		}

		result += ")";
		return result;
	}
};

struct NamespaceAccess : Expression {
	Vec<UniquePtr<Expression>> path;

	NamespaceAccess(Token&& start, Vec<UniquePtr<Expression>>&& path)
		: Expression(Move(start)), path(Move(path)) {
	}

	Str toString() const override {
		Str result;
		for (size_t i = 0; i < path.size(); i++) {
			if (i > 0) result += "::";
			result += path[i]->toString();
		}
		return "NamespaceAccess(" + result + ")";
	}
};

struct MemberAccess : Expression {
	UniquePtr<Expression> target;
	Token op;
	UniquePtr<Identifier> member;

	MemberAccess(Token&& start, UniquePtr<Expression> target, Token op, UniquePtr<Identifier> member)
		: Expression(Move(start)), target(Move(target)), op(op), member(Move(member)) {}

	Str toString() const override {
		return "MemberAccess(" + target->toString() + " " + op.lexeme + " " + member->toString() + ")";
	}
};

struct ArrayExpr : Expression { // [expr1, expr2, etc]
	Vec<UniquePtr<Expression>> elements;

	ArrayExpr(Token&& start, Vec<UniquePtr<Expression>>&& elements) 
		: Expression(Move(start)), elements(Move(elements)) {
	}

	Str toString() const override {
		Str result = "ArrayExpr([";
		for (const auto& element : elements) {
			result += element->toString() + ", ";
		}

		if (!elements.empty()) {
			result.pop_back(); // Remove trailing comma
			result.pop_back(); // Remove trailing space
		}

		result += "])";
		return result;
	}
};

/// Statements
struct Statement : Node {
	Statement(Token startToken) : Node(Move(startToken)) { // receive a copy(incase of lval) then move it to node
	}

	virtual Str toString() const override = 0;
};

struct ExprStmt : Statement {
	UniquePtr<Expression> expr;

	ExprStmt(Token&& start, UniquePtr<Expression> expr) 
		: Statement(Move(start)), expr(Move(expr)) {}

	Str toString() const override {
		return "ExprStmt(" + expr->toString() + ")";
	}
};

struct VarDecl : Statement {
	UniquePtr<TypeName> typeName;
	UniquePtr<Identifier> name;
	UniquePtr<Expression> initializer;

	VarDecl(Token&& start, UniquePtr<TypeName>&& typeName, UniquePtr<Identifier>&& name, UniquePtr<Expression>&& initializer)
		: Statement(Move(start)), typeName(Move(typeName)), name(Move(name)), initializer(Move(initializer)) {}

	Str toString() const override {
		Str result = "VarDecl(";
		if (typeName) {
			result += typeName->toString() + " ";
		}

		result += name->toString();

		if (initializer) {
			result += " = " + initializer->toString();
		}

		result += ")";
		return result;
	}
};

struct Block : Statement {
	Vec<UniquePtr<Statement>> statements;

	Block(Token&& start, Vec<UniquePtr<Statement>>&& statements) 
		: Statement(Move(start)), statements(Move(statements)) {}

	Str toString() const override {
		Str result = "Block([\n";
		for (const auto& stmt : statements) {
			result += "  " + stmt->toString() + ",\n";
		}
		result += "])";
		return result;
	}
};

struct FunctionParamDecl : Statement {
	UniquePtr<TypeName> type;
	UniquePtr<Identifier> name;
	UniquePtr<Expression> initializer;
	bool isParams;

	FunctionParamDecl(Token&& start, UniquePtr<TypeName>&& type, UniquePtr<Identifier>&& name, UniquePtr<Expression>&& initializer, bool isParams)
		: Statement(Move(start)), type(Move(type)), name(Move(name)), initializer(Move(initializer)), isParams(isParams) {
	}


	Str toString() const override {
		Str result = "FunctionParamDecl(";

		if (type) {
			result += type->toString() + " ";
		}

		result += name->toString();
		if (initializer) {
			result += " = " + initializer->toString();
		}

		if (isParams) {
			result += ", params";
		}

		result += ")";
		return result;
	}
};

struct FunctionDecl : Statement {
	UniquePtr<Identifier> name;
	Vec<UniquePtr<FunctionParamDecl>> parameters;
	UniquePtr<TypeName> returnType;
	UniquePtr<Block> body;

	FunctionDecl(
		Token&& start,
		UniquePtr<Identifier>&& name,
		Vec<UniquePtr<FunctionParamDecl>>&& parameters,
		UniquePtr<TypeName>&& returnType,
		UniquePtr<Block>&& body)
		: Statement(Move(start)), name(Move(name)), parameters(Move(parameters)), returnType(Move(returnType)), body(Move(body)) {}

	Str toString() const override {
		Str result = "FunctionDecl(" + name->toString() + ", [";
		for (const auto& param : parameters) {
			result += param->toString() + ", ";
		}

		if (!parameters.empty()) {
			result.pop_back(); // Remove trailing comma
			result.pop_back(); // Remove trailing space
		}

		result += "], ";

		if (returnType) {
			result += returnType->toString();
		}
		else {
			result += "void";
		}

		result += ", " + body->toString() + ")";
		return result;
	}
};

struct IfStmt : Statement {
	UniquePtr<Expression> condition;
	UniquePtr<Block> thenBlock;
	UniquePtr<Block> elseBlock;

	IfStmt(Token&& start, UniquePtr<Expression>&& condition, UniquePtr<Block>&& thenBlock, UniquePtr<Block>&& elseBlock)
		: Statement(Move(start)), condition(Move(condition)), thenBlock(Move(thenBlock)), elseBlock(Move(elseBlock)) {}

	Str toString() const override {
		Str result = "IfStmt(" + condition->toString() + ", " + thenBlock->toString();
		if (elseBlock) {
			result += ", " + elseBlock->toString();
		}
		result += ")";
		return result;
	}
};

struct ForStmt : Statement {
	UniquePtr<VarDecl> init;
	UniquePtr<Expression> condition;
	UniquePtr<Expression> increment;
	UniquePtr<Block> body;

	ForStmt(Token&& start, UniquePtr<VarDecl>&& init, UniquePtr<Expression>&& condition, UniquePtr<Expression>&& increment, UniquePtr<Block>&& body)
		: Statement(Move(start)), init(Move(init)), condition(Move(condition)), increment(Move(increment)), body(Move(body)) {}

	Str toString() const override {
		Str result = "ForStmt(";

		if (init) {
			result += init->toString() + "; ";
		}

		if (condition) {
			result += condition->toString() + "; ";
		}

		if (increment) {
			result += increment->toString();
		}

		result += ", " + body->toString() + ")";
		return result;
	}
};

struct ForeachStmt : Statement {
	UniquePtr<VarDecl> variable;
	UniquePtr<Expression> collection;
	UniquePtr<Block> body;

	ForeachStmt(Token&& start, UniquePtr<VarDecl>&& variable, UniquePtr<Expression>&& collection, UniquePtr<Block>&& body)
		: Statement(Move(start)), variable(Move(variable)), collection(Move(collection)), body(Move(body)) {}

	Str toString() const override {
		Str result = "ForeachStmt(";

		if (variable) {
			result += variable->toString() + ", ";
		}

		result += collection->toString() + ", ";
		result += body->toString() + ")";
		return result;
	}
};

struct WhileStmt : Statement {
	UniquePtr<Expression> condition;
	UniquePtr<Block> body;

	WhileStmt(Token&& start, UniquePtr<Expression>&& condition, UniquePtr<Block>&& body)
		: Statement(Move(start)), condition(Move(condition)), body(Move(body)) {}

	Str toString() const override {
		return "WhileStmt(" + condition->toString() + ", " + body->toString() + ")";
	}
};

struct LangBlock : Statement {
	Str language;
	Str rawCode;

	LangBlock(Token&& start, Str&& language, Str&& rawCode)
		: Statement(Move(start)), language(Move(language)), rawCode(Move(rawCode)) {}
	
	Str toString() const override {
		return "LangBlock(" + language + ", " + rawCode + ")";
	}
};

struct AccessModifier : Statement {
	Vec<Token> modifiers;

	AccessModifier(Token&& start, Vec<Token>&& modifiers)
		: Statement(Move(start)), modifiers(Move(modifiers)) {}

	Str toString() const override {
		Str result = "AccessModifier(";

		if (!modifiers.empty()) {
			for (auto& modifier : modifiers) {
				result += modifier.lexeme + ", ";
			}

			result.pop_back();
			result.pop_back();
		}

		result += ")";
		return result;
	}
};

struct NamespaceDecl : Statement {
	Vec<UniquePtr<Identifier>> path;
	UniquePtr<Block> body;

	NamespaceDecl(Token&& start, Vec<UniquePtr<Identifier>>&& path, UniquePtr<Block> body)
		: Statement(Move(start)), path(Move(path)), body(Move(body)) {}

	Str toString() const override {
		Str result = "NamespaceDecl(";

		for (auto& p : path) {
			result += p->toString() + "::";
		}

		result.pop_back();
		result.pop_back();

		result += ", " + body->toString() + ")";
		return result;
	}
};

struct DeclarationSpec : Statement {
	UniquePtr<Identifier> spec;

	DeclarationSpec(Token&& start, UniquePtr<Identifier>&& spec)
		: Statement(Move(start)), spec(Move(spec)) {}

	Str toString() const override {
		return "DeclarationSpec(" + spec->toString() + ")";
	}
};

struct UseStmt : Statement {
	Vec<Vec<UniquePtr<Identifier>>> paths; // use nms1, nms2, nms3; or use nms1, nms2, nms3 from "xyz"
	UniquePtr<Literal> file; // from "stdf.mrk"

	UseStmt(Token&& start, Vec<Vec<UniquePtr<Identifier>>>&& paths, UniquePtr<Literal>&& file)
		: Statement(Move(start)), paths(Move(paths)), file(Move(file)) {}

	Str toString() const override {
		Str result = "UseStmt([";
		for (const auto& path : paths) {
			result += "[";
			for (const auto& id : path) {
				result += id->toString() + ", ";
			}
			if (!path.empty()) {
				result.pop_back(); // Remove trailing comma
				result.pop_back(); // Remove trailing space
			}
			result += "], ";
		}

		if (!paths.empty()) {
			result.pop_back(); // Remove trailing comma
			result.pop_back(); // Remove trailing space
		}

		result += "], ";

		if (file) {
			result += file->toString();
		}
		else {
			result.pop_back(); // Remove trailing comma
			result.pop_back(); // Remove trailing space
		}

		result += ")";
		return result;
	}
};

struct EnumDecl : Statement {
	UniquePtr<Identifier> name;
	UniquePtr<TypeName> type;
	Vec<MRK_STD pair<UniquePtr<Identifier>, UniquePtr<Expression>>> members;

	EnumDecl(
		Token&& start,
		UniquePtr<Identifier>&& name,
		UniquePtr<TypeName>&& type,
		Vec<MRK_STD pair<UniquePtr<Identifier>,
		UniquePtr<Expression>>>&& members)
		: Statement(Move(start)), name(Move(name)), type(Move(type)), members(Move(members)) {}

    Str toString() const override {
        Str result = "EnumDecl(" + name->toString() + ", ";

        if (type) {
            result += type->toString() + ", ";
        } else {
            result += "int, ";
        }

        result += "[";

        for (const auto& member : members) {
            result += "(" + member.first->toString() + ", ";
            if (member.second) {
                result += member.second->toString();
            } else {
                result += "null";
            }
            result += "), ";
        }

        if (!members.empty()) {
            result.pop_back(); // Remove trailing comma
            result.pop_back(); // Remove trailing space
        }

        result += "])";
        return result;
    }
};

struct TypeDecl : Statement {
	Token type; // struct / class
	UniquePtr<TypeName> name;
	Vec<UniquePtr<Identifier>> aliases;
	Vec<UniquePtr<TypeName>> baseTypes;
	UniquePtr<Block> body;

	TypeDecl(Token&& type, UniquePtr<TypeName>&& name, Vec<UniquePtr<Identifier>>&& aliases, Vec<UniquePtr<TypeName>>&& baseTypes, UniquePtr<Block>&& body)
		: Statement(type), type(Move(type)), name(Move(name)), aliases(Move(aliases)), baseTypes(Move(baseTypes)), body(Move(body)) {}


    Str toString() const override {
        Str result = "TypeDecl(" + type.lexeme + ", " + name->toString() + ", [";
        for (const auto& alias : aliases) {
            result += alias->toString() + ", ";
        }

        if (!aliases.empty()) {
            result.pop_back(); // Remove trailing comma
            result.pop_back(); // Remove trailing space
        }

        result += "], [";

        for (const auto& baseType : baseTypes) {
            result += baseType->toString() + ", ";
        }

        if (!baseTypes.empty()) {
            result.pop_back(); // Remove trailing comma
            result.pop_back(); // Remove trailing space
        }

		result += "], [" + body->toString() + "])";
        return result;
    }
};

/// Program structure
struct Program {
	Vec<UniquePtr<Statement>> statements;

	Program() = default;
	Program(Vec<UniquePtr<Statement>>&& statements) : statements(Move(statements)) {}

	Str toString() const {
		Str result = "Program([\n";
		for (const auto& stmt : statements) {
			result += "  " + stmt->toString() + ",\n";
		}
		result += "])";
		return result;
	}
};

MRK_NS_END