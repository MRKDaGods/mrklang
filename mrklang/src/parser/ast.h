#pragma once

#include "common/macros.h"
#include "core/source_file.h"
#include "lexer/token.h"

#include <memory>
#include <vector>
#include <string>
#include <sstream>

MRK_NS_BEGIN_MODULE(ast)

#define AST_NODE_TYPES \
	X(Program) \
    X(LiteralExpr) \
    X(InterpolatedStringExpr) \
    X(InteropCallExpr) \
    X(IdentifierExpr) \
    X(TypeReferenceExpr) \
    X(CallExpr) \
    X(BinaryExpr) \
    X(UnaryExpr) \
    X(TernaryExpr) \
    X(AssignmentExpr) \
    X(NamespaceAccessExpr) \
    X(MemberAccessExpr) \
    X(ArrayExpr) \
    X(ExprStmt) \
    X(VarDeclStmt) \
    X(BlockStmt) \
    X(ParamDeclStmt) \
    X(FuncDeclStmt) \
    X(IfStmt) \
    X(ForStmt) \
    X(ForeachStmt) \
    X(WhileStmt) \
    X(LangBlockStmt) \
    X(AccessModifierStmt) \
    X(NamespaceDeclStmt) \
    X(DeclSpecStmt) \
    X(UseStmt) \
    X(ReturnStmt) \
    X(EnumDeclStmt) \
    X(TypeDeclStmt)

// Forward declare all node types
#define X(type) struct type;
AST_NODE_TYPES
#undef X

class ASTVisitor {
public:
	virtual ~ASTVisitor() = default;

	// Visitor interface
	#define X(type) virtual void visit(type* node) {}
	AST_NODE_TYPES
	#undef X
};

#undef AST_NODE_TYPES

/// Base node for expressions and statements
struct Node {
	/// Mapping ast to source code
	Token startToken;

	/// Source file where this node is located
	/// Set by SymbolCollector
	const SourceFile* sourceFile;

	Node(Token&& startToken) : startToken(Move(startToken)), sourceFile(nullptr) {}
	virtual ~Node() = default;
	virtual Str toString() const = 0;
	virtual void accept(ASTVisitor& visitor) = 0;
};

//==============================================================================
// Expression nodes
//==============================================================================

/// Base class for all expression nodes
struct ExprNode : Node {
	ExprNode(Token startToken) : Node(Move(startToken)) {}
};

/// Literal value expression (numbers, strings, etc)
struct LiteralExpr : ExprNode {
	Token value;

	LiteralExpr(Token val) : ExprNode(val), value(Move(val)) {}
	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// String with embedded expressions: $"Hello {name}"
struct InterpolatedStringExpr : ExprNode {
	Vec<UniquePtr<ExprNode>> parts;

	InterpolatedStringExpr(Token&& start, Vec<UniquePtr<ExprNode>>&& parts)
		: ExprNode(Move(start)), parts(Move(parts)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Language interoperability call
struct InteropCallExpr : ExprNode {
	Str targetLang;
	UniquePtr<ExprNode> method;
	Vec<UniquePtr<ExprNode>> args;

	InteropCallExpr(Token&& start, Str targetLang, UniquePtr<ExprNode> method, Vec<UniquePtr<ExprNode>>&& args)
		: ExprNode(Move(start)), targetLang(Move(targetLang)), method(Move(method)), args(Move(args)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Identifier expression (variable names, function names, etc)
struct IdentifierExpr : ExprNode {
	Str name;

	IdentifierExpr(Token tok) : ExprNode(tok), name(Move(tok.lexeme)) {}
	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Type reference expression (int, string, MyClass, etc)
struct TypeReferenceExpr : ExprNode { // INT***[][]
	Vec<UniquePtr<IdentifierExpr>> identifiers; // ["csharp", "System", "Int32"] where nms=csharp::System
	Vec<UniquePtr<TypeReferenceExpr>> genericArgs; // generics
	int pointerRank;
	int arrayRank;

	TypeReferenceExpr(Token&& start, Vec<UniquePtr<IdentifierExpr>>&& identifiers, Vec<UniquePtr<TypeReferenceExpr>>&& genericArgs, int&& pointerRank, int&& arrayRank)
		: ExprNode(Move(start)), identifiers(Move(identifiers)), genericArgs(Move(genericArgs)), pointerRank(Move(pointerRank)), arrayRank(Move(arrayRank)) {}

	Str getTypeName() const;
	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Function call expression: func(arg1, arg2)
struct CallExpr : ExprNode {
	UniquePtr<ExprNode> target;
	Vec<UniquePtr<ExprNode>> arguments;

	CallExpr(Token&& start, UniquePtr<ExprNode>&& target, Vec<UniquePtr<ExprNode>>&& arguments)
		: ExprNode(Move(start)), target(Move(target)), arguments(Move(arguments)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Binary expression: a + b, a * b, etc
struct BinaryExpr : ExprNode {
	UniquePtr<ExprNode> left;
	Token op;
	UniquePtr<ExprNode> right;

	BinaryExpr(Token&& start, UniquePtr<ExprNode>&& left, Token op, UniquePtr<ExprNode>&& right)
		: ExprNode(Move(start)), left(Move(left)), op(op), right(Move(right)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Unary expression: !a, -b, etc
struct UnaryExpr : ExprNode {
	Token op;
	UniquePtr<ExprNode> right;

	UnaryExpr(Token&& start, Token op, UniquePtr<ExprNode>&& right)
		: ExprNode(Move(start)), op(op), right(Move(right)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Ternary expression: a ? b : c
struct TernaryExpr : ExprNode {
	UniquePtr<ExprNode> condition;
	UniquePtr<ExprNode> thenBranch;
	UniquePtr<ExprNode> elseBranch;

	TernaryExpr(Token&& start, UniquePtr<ExprNode>&& condition, UniquePtr<ExprNode>&& thenBranch, UniquePtr<ExprNode>&& elseBranch)
		: ExprNode(Move(start)), condition(Move(condition)), thenBranch(Move(thenBranch)), elseBranch(Move(elseBranch)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Assignment expression: a = b, a += b, etc
struct AssignmentExpr : ExprNode {
	UniquePtr<ExprNode> target;
	Token op;
	UniquePtr<ExprNode> value;

	AssignmentExpr(Token&& start, UniquePtr<ExprNode>&& target, Token op, UniquePtr<ExprNode>&& value)
		: ExprNode(Move(start)), target(Move(target)), op(op), value(Move(value)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Namespace access expression: a::b::c
struct NamespaceAccessExpr : ExprNode {
	Vec<UniquePtr<ExprNode>> path;

	NamespaceAccessExpr(Token&& start, Vec<UniquePtr<ExprNode>>&& path)
		: ExprNode(Move(start)), path(Move(path)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Member access expression: a.b, a->b
struct MemberAccessExpr : ExprNode {
	UniquePtr<ExprNode> target;
	Token op;
	UniquePtr<IdentifierExpr> member;

	MemberAccessExpr(Token&& start, UniquePtr<ExprNode>&& target, Token op, UniquePtr<IdentifierExpr>&& member)
		: ExprNode(Move(start)), target(Move(target)), op(op), member(Move(member)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Array literal expression: [a, b, c]
struct ArrayExpr : ExprNode { // [expr1, expr2, etc]
	Vec<UniquePtr<ExprNode>> elements;

	ArrayExpr(Token&& start, Vec<UniquePtr<ExprNode>>&& elements)
		: ExprNode(Move(start)), elements(Move(elements)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

//==============================================================================
// Statement nodes
//==============================================================================

/// Base class for all statement nodes
struct StmtNode : Node {
	StmtNode(Token startToken) : Node(Move(startToken)) {}
};

/// Expression statement: expr;
struct ExprStmt : StmtNode {
	UniquePtr<ExprNode> expr;

	ExprStmt(Token&& start, UniquePtr<ExprNode>&& expr)
		: StmtNode(Move(start)), expr(Move(expr)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Variable declaration: var x = 5;
struct VarDeclStmt : StmtNode {
	UniquePtr<TypeReferenceExpr> typeName;
	UniquePtr<IdentifierExpr> name;
	UniquePtr<ExprNode> initializer;

	VarDeclStmt(Token&& start, UniquePtr<TypeReferenceExpr>&& typeName, UniquePtr<IdentifierExpr>&& name, UniquePtr<ExprNode>&& initializer)
		: StmtNode(Move(start)), typeName(Move(typeName)), name(Move(name)), initializer(Move(initializer)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Block statement: { stmt1; stmt2; }
struct BlockStmt : StmtNode {
	Vec<UniquePtr<StmtNode>> statements;

	BlockStmt(Token&& start, Vec<UniquePtr<StmtNode>>&& statements)
		: StmtNode(Move(start)), statements(Move(statements)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Function parameter declaration
struct ParamDeclStmt : StmtNode {
	UniquePtr<TypeReferenceExpr> type;
	UniquePtr<IdentifierExpr> name;
	UniquePtr<ExprNode> initializer;
	bool isParams;

	ParamDeclStmt(Token&& start, UniquePtr<TypeReferenceExpr>&& type, UniquePtr<IdentifierExpr>&& name, UniquePtr<ExprNode>&& initializer, bool isParams)
		: StmtNode(Move(start)), type(Move(type)), name(Move(name)), initializer(Move(initializer)), isParams(isParams) {}

	Str getSignature();
	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Function declaration: func name(params) -> returnType { body }
struct FuncDeclStmt : StmtNode {
	UniquePtr<IdentifierExpr> name;
	Vec<UniquePtr<ParamDeclStmt>> parameters;
	UniquePtr<TypeReferenceExpr> returnType;
	UniquePtr<BlockStmt> body;

	FuncDeclStmt(
		Token&& start,
		UniquePtr<IdentifierExpr>&& name,
		Vec<UniquePtr<ParamDeclStmt>>&& parameters,
		UniquePtr<TypeReferenceExpr>&& returnType,
		UniquePtr<BlockStmt>&& body)
		: StmtNode(Move(start)), name(Move(name)), parameters(Move(parameters)), returnType(Move(returnType)), body(Move(body)) {}

	Str getSignature() const;
	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// If statement: if (condition) { thenBlock } else { elseBlock }
struct IfStmt : StmtNode {
	UniquePtr<ExprNode> condition;
	UniquePtr<BlockStmt> thenBlock;
	UniquePtr<BlockStmt> elseBlock;

	IfStmt(Token&& start, UniquePtr<ExprNode>&& condition, UniquePtr<BlockStmt>&& thenBlock, UniquePtr<BlockStmt>&& elseBlock)
		: StmtNode(Move(start)), condition(Move(condition)), thenBlock(Move(thenBlock)), elseBlock(Move(elseBlock)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// For statement: for (init; condition; increment) { body }
struct ForStmt : StmtNode {
	UniquePtr<VarDeclStmt> init;
	UniquePtr<ExprNode> condition;
	UniquePtr<ExprNode> increment;
	UniquePtr<BlockStmt> body;

	ForStmt(Token&& start, UniquePtr<VarDeclStmt>&& init, UniquePtr<ExprNode>&& condition, UniquePtr<ExprNode>&& increment, UniquePtr<BlockStmt>&& body)
		: StmtNode(Move(start)), init(Move(init)), condition(Move(condition)), increment(Move(increment)), body(Move(body)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Foreach statement: foreach (var in collection) { body }
struct ForeachStmt : StmtNode {
	UniquePtr<VarDeclStmt> variable;
	UniquePtr<ExprNode> collection;
	UniquePtr<BlockStmt> body;

	ForeachStmt(Token&& start, UniquePtr<VarDeclStmt>&& variable, UniquePtr<ExprNode>&& collection, UniquePtr<BlockStmt>&& body)
		: StmtNode(Move(start)), variable(Move(variable)), collection(Move(collection)), body(Move(body)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// While statement: while (condition) { body }
struct WhileStmt : StmtNode {
	UniquePtr<ExprNode> condition;
	UniquePtr<BlockStmt> body;

	WhileStmt(Token&& start, UniquePtr<ExprNode>&& condition, UniquePtr<BlockStmt>&& body)
		: StmtNode(Move(start)), condition(Move(condition)), body(Move(body)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Language-specific block: __cpp{ ... }, __cs{ ... }, etc.
struct LangBlockStmt : StmtNode {
	Str language;
	Str rawCode;

	LangBlockStmt(Token&& start, Str&& language, Str&& rawCode)
		: StmtNode(Move(start)), language(Move(language)), rawCode(Move(rawCode)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Access modifier: public, private, protected, etc.
struct AccessModifierStmt : StmtNode {
	Vec<Token> modifiers;

	AccessModifierStmt(Token&& start, Vec<Token>&& modifiers)
		: StmtNode(Move(start)), modifiers(Move(modifiers)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Namespace declaration: namespace name { body }
struct NamespaceDeclStmt : StmtNode {
	Vec<UniquePtr<IdentifierExpr>> path;
	UniquePtr<BlockStmt> body;

	NamespaceDeclStmt(Token&& start, Vec<UniquePtr<IdentifierExpr>>&& path, UniquePtr<BlockStmt>&& body)
		: StmtNode(Move(start)), path(Move(path)), body(Move(body)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Declaration specifier: __declspec(xxx)
struct DeclSpecStmt : StmtNode {
	UniquePtr<IdentifierExpr> spec;

	DeclSpecStmt(Token&& start, UniquePtr<IdentifierExpr>&& spec)
		: StmtNode(Move(start)), spec(Move(spec)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Use statement: use nms1, nms2, nms3; or use nms1, nms2, nms3 from "xyz"
struct UseStmt : StmtNode {
	Vec<Vec<UniquePtr<IdentifierExpr>>> paths;
	UniquePtr<LiteralExpr> file; // from "stdf.mrk"

	UseStmt(Token&& start, Vec<Vec<UniquePtr<IdentifierExpr>>>&& paths, UniquePtr<LiteralExpr>&& file)
		: StmtNode(Move(start)), paths(Move(paths)), file(Move(file)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Return statement: return value;
struct ReturnStmt : StmtNode {
	UniquePtr<ExprNode> value;

	ReturnStmt(Token&& start, UniquePtr<ExprNode>&& value)
		: StmtNode(Move(start)), value(Move(value)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Enum declaration: enum<type> Name { Member1, Member2 = value, ... }
struct EnumDeclStmt : StmtNode {
	UniquePtr<IdentifierExpr> name;
	UniquePtr<TypeReferenceExpr> type;
	Vec<std::pair<UniquePtr<IdentifierExpr>, UniquePtr<ExprNode>>> members;

	EnumDeclStmt(
		Token&& start,
		UniquePtr<IdentifierExpr>&& name,
		UniquePtr<TypeReferenceExpr>&& type,
		Vec<std::pair<UniquePtr<IdentifierExpr>,
		UniquePtr<ExprNode>>>&& members)
		: StmtNode(Move(start)), name(Move(name)), type(Move(type)), members(Move(members)) {}

	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Type declaration: class/struct/interface Name<T> as Alias : Base1, Base2 { body }
struct TypeDeclStmt : StmtNode {
	Token type; // struct / class / interface
	UniquePtr<TypeReferenceExpr> name;
	Vec<UniquePtr<IdentifierExpr>> aliases;
	Vec<UniquePtr<TypeReferenceExpr>> baseTypes;
	UniquePtr<BlockStmt> body;

	TypeDeclStmt(
		Token&& type,
		UniquePtr<TypeReferenceExpr>&& name,
		Vec<UniquePtr<IdentifierExpr>>&& aliases,
		Vec<UniquePtr<TypeReferenceExpr>>&& baseTypes,
		UniquePtr<BlockStmt>&& body)
		: StmtNode(type), type(Move(type)), name(Move(name)), aliases(Move(aliases)), baseTypes(Move(baseTypes)), body(Move(body)) {}


	Str toString() const override;
	void accept(ASTVisitor& visitor) override;
};

/// Program structure
struct Program {
	const SourceFile* sourceFile;
	Vec<UniquePtr<StmtNode>> statements;

	Program() = default;
	Program(const SourceFile* sourceFile, Vec<UniquePtr<StmtNode>>&& statements)
		: sourceFile(sourceFile), statements(Move(statements)) {}

	Str toString() const;
};

MRK_NS_END