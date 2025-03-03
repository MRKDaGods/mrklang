#pragma once

#include "common/types.h"
#include "lexer/token.h"
#include "lexer/lexer.h"
#include "ast.h"

#include <vector>
#include <string>

MRK_NS_BEGIN

using namespace ast;

struct ParserError : std::runtime_error {
	Token token;

	ParserError(const Token& token, Str message) 
		: std::runtime_error(Move(message)), token(Token(token)) {
	}
};

class Parser {
public:
	Parser(const Str& filename, const Lexer* lexer);
	UniquePtr<Program> parseProgram();
	const Vec<ParserError>& getErrors() const;
	void reportErrors() const;

private:
	const Str filename_;
	const Lexer* lexer_;
	Vec<Token> tokens_;
	uint32_t currentPos_;
	Token current_;
	Token previous_;
	Vec<ParserError> errors_;

	// Error handling
	[[noreturn]] ParserError error(const Token& token, const Str& message);
	void synchronize();

	// Token navigation
	void advance();
	bool check(TokenType type) const;
	bool match(TokenType type);
	Token consume(TokenType type, const Str& message);
	const Token& getPrevious() const;
	const Token& peekNext() const;

	// Top-level declarations / Statements
	UniquePtr<StmtNode> parseTopLevelDecl();
	UniquePtr<LangBlockStmt> parseLangBlock();
	UniquePtr<VarDeclStmt> parseVarDecl(bool requireSemicolon = true);
	UniquePtr<FuncDeclStmt> parseFunctionDecl();
	UniquePtr<TypeReferenceExpr> parseTypeReference();
	UniquePtr<ParamDeclStmt> parseFunctionParamDecl();
	UniquePtr<AccessModifierStmt> parseAccessModifier();

	// Statements
	UniquePtr<StmtNode> parseStatement();
	UniquePtr<BlockStmt> parseBlock(bool consumeBrace = true);
	UniquePtr<IfStmt> parseIfStatement();
	UniquePtr<ForStmt> parseForStatement();
	UniquePtr<ForeachStmt> parseForeachStatement();
	UniquePtr<WhileStmt> parseWhileStatement();
	UniquePtr<ExprStmt> parseExprStatement();
	UniquePtr<NamespaceDeclStmt> parseNamespaceDecl();
	UniquePtr<DeclSpecStmt> parseDeclSpecStatement();
	UniquePtr<UseStmt> parseUseStatement();
	UniquePtr<ReturnStmt> parseReturnStatement();
	UniquePtr<EnumDeclStmt> parseEnumDecl();
	UniquePtr<TypeDeclStmt> parseTypeDecl();

	// Expressions
	UniquePtr<ExprNode> parseExpression();
	UniquePtr<ExprNode> parseAssignment();
	UniquePtr<ExprNode> parseTernary();
	UniquePtr<ExprNode> parseLogicalOr();
	UniquePtr<ExprNode> parseLogicalAnd();
	UniquePtr<ExprNode> parseEquality();
	UniquePtr<ExprNode> parseComparison();
	UniquePtr<ExprNode> parseTerm();
	UniquePtr<ExprNode> parseFactor();
	UniquePtr<ExprNode> parseUnary();
	UniquePtr<ExprNode> parsePrimary();
	UniquePtr<ExprNode> parseFunctionCall(UniquePtr<ExprNode> target);
	UniquePtr<ExprNode> parseNamespaceAccess(UniquePtr<IdentifierExpr> identifier);
	UniquePtr<ExprNode> parseMemberAccess(UniquePtr<ExprNode> target);
	UniquePtr<ExprNode> parseArray();
	UniquePtr<ExprNode> parseInterpolatedString();
};

MRK_NS_END