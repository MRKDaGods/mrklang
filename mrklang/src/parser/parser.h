#pragma once

#include "common/types.h"
#include "lexer/token.h"
#include "lexer/lexer.h"
#include "ast.h"

#include <vector>
#include <string>

MRK_NS_BEGIN

using namespace ast;

struct ParserError : MRK_STD runtime_error {
	Token token;

	ParserError(const Token& token, Str message) 
		: MRK_STD runtime_error(MRK_STD move(message)), token(Token(token)) {
	}
};

class Parser {
public:
	Parser(const Lexer* lexer);
	UniquePtr<Program> parse();

	const Vec<ParserError>& errors() const;

	void reportErrors() const;

private:
	const Lexer* lexer_;
	Vec<Token> tokens_;
	uint32_t currentPos_;
	Token current_;
	Token previous_;
	Vec<ParserError> errors_;

	[[noreturn]] ParserError error(const Token& token, const Str& message);
	void advance();
	bool check(TokenType type) const;
	bool match(TokenType type);
	Token consume(TokenType type, const Str& message);
	void synchronize();
	const Token& previous() const;
	const Token& peekNext() const;

	/// Statements
	UniquePtr<Statement> topLevelDecl();
	UniquePtr<LangBlock> langBlock();
	UniquePtr<VarDecl> varDecl(bool requireSemicolon = true);
	UniquePtr<FunctionDecl> functionDecl();
	UniquePtr<TypeName> parseTypeName();
	UniquePtr<FunctionParamDecl> functionParamDecl();
	UniquePtr<AccessModifier> accessModifier();
	UniquePtr<Statement> statement();
	UniquePtr<Block> block(bool consumeBrace = true);
	UniquePtr<IfStmt> ifStatement();
	UniquePtr<ForStmt> forStatement();
	UniquePtr<ForeachStmt> foreachStatement();
	UniquePtr<WhileStmt> whileStatement();
	UniquePtr<ExprStmt> exprStatement();
	UniquePtr<NamespaceDecl> namespaceDecl();
	UniquePtr<DeclarationSpec> declSpecStatement();
	UniquePtr<UseStmt> useStatement();
	UniquePtr<EnumDecl> enumDecl();
	UniquePtr<TypeDecl> typeDecl();

	/// Expressions
	UniquePtr<Expression> expression();
	UniquePtr<Expression> assignment();
	UniquePtr<Expression> ternary();
	UniquePtr<Expression> logicalOr();
	UniquePtr<Expression> logicalAnd();
	UniquePtr<Expression> equality();
	UniquePtr<Expression> comparison();
	UniquePtr<Expression> term();
	UniquePtr<Expression> factor();
	UniquePtr<Expression> unary();
	UniquePtr<Expression> primary();
	UniquePtr<Expression> functionCall(UniquePtr<Expression> target);
	UniquePtr<Expression> namespaceAccess(UniquePtr<Identifier> identifier);
	UniquePtr<Expression> memberAccess(UniquePtr<Expression> target);
	UniquePtr<Expression> array();
	UniquePtr<Expression> interpolatedString();
};

MRK_NS_END