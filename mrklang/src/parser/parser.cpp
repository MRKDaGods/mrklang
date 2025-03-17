#include "parser.h"
#include "common/logging.h"

#include <sstream>

MRK_NS_BEGIN

Parser::Parser(Vec<Token>&& tokens) : tokens_(Move(tokens)), currentPos_(0) {
	// initialize current, prev is EOF by default
	if (!tokens_.empty()) {
		current_ = tokens_[0];
	}
}

UniquePtr<Program> Parser::parseProgram(const SourceFile* sourceFile) {
	auto program = MakeUnique<Program>();
	program->sourceFile = sourceFile;

	// https://stackoverflow.com/questions/61809337/orphan-range-crash-when-using-static-vector
	program->statements = Vec<UniquePtr<StmtNode>>();

	while (!match(TokenType::END_OF_FILE)) {
		try {
			// try parse top level declaration
			auto stmt = parseTopLevelDecl();
			if (stmt) {
				program->statements.push_back(Move(stmt));
			}
		}
		catch (const CompilerError*& err) {
			 // recover
			synchronize();
		}
		catch (const std::exception& e) {
			// unrecoverable error
			MRK_ERROR("Unrecoverable error: {}", e.what());
			break;
		}
	}

	return program;
}

CompilerError* Parser::error(const Token& token, const Str& message) {
	CompilerError* err;
	ErrorReporter::instance().parserError(message, token, &err);

	return err;
}

void Parser::synchronize() {
	// Recover gracefully
	advance();

	while (!check(TokenType::END_OF_FILE)) {
		if (previous_.type == TokenType::SEMICOLON) return;

		switch (current_.type) {
			case TokenType::KW_FUNC:
			case TokenType::KW_VAR:
			case TokenType::KW_IF:
			case TokenType::KW_FOR:
			case TokenType::KW_FOREACH:
			case TokenType::KW_WHILE:
			case TokenType::KW_NAMESPACE:
			case TokenType::LBRACE:
				return;
		}

		advance();
	}
}

void Parser::advance() {
	previous_ = current_;

	if (currentPos_ + 1 < tokens_.size()) {
		current_ = tokens_[++currentPos_];
	}
	else {
		current_ = Token();
	}
}

bool Parser::check(TokenType type) const {
	return current_.type == type;
}

bool Parser::match(TokenType type) {
	if (!check(type)) {
		return false;
	}

	advance();
	return true;
}

Token Parser::consume(TokenType type, const Str& message) {
	if (check(type)) {
		advance();
		return previous_;
	}

	throw error(current_, message);
}

const Token& Parser::getPrevious() const {
	return previous_;
}

const Token& Parser::peekNext() const {
	auto idx = currentPos_ + 1;
	return idx < tokens_.size() ? tokens_[idx] : Token();
}

UniquePtr<StmtNode> Parser::parseTopLevelDecl() {
	// TODO: only use statement() ?

	if (match(TokenType::KW_USE)) {
		return parseUseStatement();
	}

	return parseStatement();
}

UniquePtr<LangBlockStmt> Parser::parseLangBlock() {
	auto startToken = previous_;
	auto language = previous_.lexeme;

	consume(TokenType::LIT_LANG_BLOCK, "Invalid language block");

	auto rawCode = previous_.lexeme;
	// Remove the curly braces
	rawCode = rawCode.substr(1, rawCode.size() - 2);

	return MakeUnique<LangBlockStmt>(Move(startToken), Move(language), Move(rawCode));
}

UniquePtr<VarDeclStmt> Parser::parseVarDecl(bool requireSemicolon) {
	auto startToken = previous_;

	// var x = 9; <-- inference
	// var<int> x = 9;

	// Check for explicit type
	UniquePtr<TypeReferenceExpr> typeName = nullptr;
	if (match(TokenType::OP_LT)) {
		typeName = parseTypeReference();

		consume(TokenType::OP_GT, "Expected '>' after type");
	}

	// Consume var name
	auto name = MakeUnique<IdentifierExpr>(
		consume(TokenType::IDENTIFIER, "Expected variable name")
	);

	// Consume nativeInitializerMethod if exists
	UniquePtr<ExprNode> initializer = nullptr;
	if (match(TokenType::OP_EQ)) {
		initializer = parseExpression();
	}

	if (requireSemicolon) {
		consume(TokenType::SEMICOLON, "Expected ';' after declaration");
	}

	return MakeUnique<VarDeclStmt>(Move(startToken), Move(typeName), Move(name), Move(initializer));
}

UniquePtr<FuncDeclStmt> Parser::parseFunctionDecl() {
	// func mrk(int m, string x = "", params string[] xz) -> int {} 
	auto startToken = previous_;

	// Parse name
	auto name = MakeUnique<IdentifierExpr>(consume(TokenType::IDENTIFIER, "Expected function name"));

	// Consume (
	consume(TokenType::LPAREN, "Expected '(' after function name");

	// Parse params
	Vec<UniquePtr<ParamDeclStmt>> parameters;

	if (!check(TokenType::RPAREN)) {
		do {
			parameters.push_back(parseFunctionParamDecl());
		} while (match(TokenType::COMMA));
	}

	// Consume )
	consume(TokenType::RPAREN, "Expected ')' after parameters");

	// Parse optional return type
	UniquePtr<TypeReferenceExpr> returnType = nullptr;
	if (match(TokenType::OP_ARROW)) {
		returnType = parseTypeReference();
	}

	// Parse body
	auto body = parseBlock();
	return MakeUnique<FuncDeclStmt>(Move(startToken), Move(name), Move(parameters), Move(returnType), Move(body));
}

UniquePtr<TypeReferenceExpr> Parser::parseTypeReference() {
	auto startToken = current_;

	Vec<UniquePtr<IdentifierExpr>> identifiers;

	// Parse identifier
	identifiers.push_back(
		MakeUnique<IdentifierExpr>(consume(TokenType::IDENTIFIER, "Expected type identifier"))
	);

	// Parse namespace qualifiers if exist
	while (match(TokenType::OP_DOUBLE_COLON)) {
		identifiers.push_back(
			MakeUnique<IdentifierExpr>(consume(TokenType::IDENTIFIER, "Expected type identifier after '::'"))
		);
	}

	Vec<UniquePtr<TypeReferenceExpr>> genericArgs;

	// Parse generic arguments if exist (ex: "mrkstl::boxed<mrkstl::boxed<int32>>")
	if (match(TokenType::OP_LT)) {
		do {
			genericArgs.push_back(parseTypeReference());
		} while (match(TokenType::COMMA));

		consume(TokenType::OP_GT, "Expected '>' after generic arguments");
	}

	// pointers
	int pointerRank = 0;
	while (match(TokenType::OP_ASTERISK)) {
		pointerRank++;
	}

	// arrays
	int arrayRank = 0;
	while (match(TokenType::LBRACKET)) {
		consume(TokenType::RBRACKET, "Expected ']'");
		arrayRank++;
	}

	return MakeUnique<TypeReferenceExpr>(Move(startToken), Move(identifiers), Move(genericArgs), Move(pointerRank), Move(arrayRank));
}

UniquePtr<ParamDeclStmt> Parser::parseFunctionParamDecl() {
	// Check whether this parameter has params
	bool isParams = match(TokenType::KW_PARAMS);

	auto startToken = isParams ? previous_ : current_;

	auto type = parseTypeReference();
	auto name = MakeUnique<IdentifierExpr>(consume(TokenType::IDENTIFIER, "Expected identifier"));

	UniquePtr<ExprNode> initializer = nullptr;
	if (match(TokenType::OP_EQ)) {
		initializer = parseExpression();
	}

	return MakeUnique<ParamDeclStmt>(Move(startToken), Move(type), Move(name), Move(initializer), Move(isParams));
}

UniquePtr<AccessModifierStmt> Parser::parseAccessModifier() {
	auto startToken = current_;

	Vec<Token> modifiers;

	while (current_.isAccessModifier()) {
		modifiers.push_back(current_);
		advance();
	}

	return MakeUnique<AccessModifierStmt>(Move(startToken), Move(modifiers));
}

UniquePtr<StmtNode> Parser::parseStatement() {
	// Incase of nested namespaces
	if (match(TokenType::KW_NAMESPACE)) {
		return parseNamespaceDecl();
	}

	// check for access modifiers
	if (current_.isAccessModifier()) {
		return parseAccessModifier();
	}

	if (match(TokenType::KW_ENUM)) {
		return parseEnumDecl();
	}

	if (match(TokenType::KW_CLASS) || match(TokenType::KW_STRUCT)) {
		return parseTypeDecl();
	}

	if (match(TokenType::KW_FUNC)) {
		return parseFunctionDecl();
	}

	if (match(TokenType::KW_VAR)) {
		return parseVarDecl();
	}

	if (match(TokenType::LBRACE)) {
		return parseBlock(false);
	}

	if (match(TokenType::KW_IF)) {
		return parseIfStatement();
	}

	if (match(TokenType::KW_FOR)) {
		return parseForStatement();
	}

	if (match(TokenType::KW_FOREACH)) {
		return parseForeachStatement();
	}

	if (match(TokenType::KW_WHILE)) {
		return parseWhileStatement();
	}

	if (match(TokenType::KW_DECLSPEC)) {
		return parseDeclSpecStatement();
	}

	if (match(TokenType::KW_RETURN)) {
		return parseReturnStatement();
	}

	// lang blocks
	if (match(TokenType::BLOCK_CPP) || match(TokenType::BLOCK_CSHARP) ||
		match(TokenType::BLOCK_DART) || match(TokenType::BLOCK_JS)) {
		return parseLangBlock();
	}

	return parseExprStatement();
}

UniquePtr<BlockStmt> Parser::parseBlock(bool consumeBrace) {
	auto startToken = consumeBrace ? current_ : previous_;

	if (consumeBrace) { // Consume brace if not already consumed
		consume(TokenType::LBRACE, "Expected '{' at beginning of block");
	}

	Vec<UniquePtr<StmtNode>> statements;
	while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
		statements.push_back(parseStatement());
	}

	consume(TokenType::RBRACE, "Expected '}' after block");
	return MakeUnique<BlockStmt>(Move(startToken), Move(statements));
}

UniquePtr<IfStmt> Parser::parseIfStatement() {
	auto startToken = previous_;

	consume(TokenType::LPAREN, "Expected '(' after if");

	auto condition = parseExpression();
	consume(TokenType::RPAREN, "Expected ')' after condition");

	auto thenBlock = parseBlock();

	UniquePtr<BlockStmt> elseBlock = nullptr;
	if (match(TokenType::KW_ELSE)) {
		elseBlock = parseBlock();
	}

	return MakeUnique<IfStmt>(Move(startToken), Move(condition), Move(thenBlock), Move(elseBlock));
}

UniquePtr<ForStmt> Parser::parseForStatement() { // for (var<int> i = 0; i < 9999; i++)
	auto startToken = previous_;
	consume(TokenType::LPAREN, "Expected '(' after for");

	// Initializer
	UniquePtr<VarDeclStmt> init = nullptr;
	if (match(TokenType::KW_VAR)) {
		init = parseVarDecl();
	}

	// Condition
	UniquePtr<ExprNode> condition = nullptr;
	if (!check(TokenType::SEMICOLON)) {
		condition = parseExpression();
	}

	consume(TokenType::SEMICOLON, "Expected ';' after for condition");

	// Increment
	UniquePtr<ExprNode> increment = nullptr;
	if (!check(TokenType::RPAREN)) {
		increment = parseExpression();
	}

	consume(TokenType::RPAREN, "Expected ')' after for clauses");

	auto body = parseBlock();
	return MakeUnique<ForStmt>(Move(startToken), Move(init), Move(condition), Move(increment), Move(body));
}

UniquePtr<ForeachStmt> Parser::parseForeachStatement() { // foreach (var x in expr()) or foreach (expr()) 
	auto startToken = previous_;
	consume(TokenType::LPAREN, "Expected '(' after foreach");

	UniquePtr<VarDeclStmt> variable = nullptr;
	if (match(TokenType::KW_VAR)) {
		variable = parseVarDecl(false);
		consume(TokenType::KW_IN, "Expected 'in' after variable declaration");
	}

	auto collection = parseExpression();
	consume(TokenType::RPAREN, "Expected ')' after foreach clause");

	auto body = parseBlock();
	return MakeUnique<ForeachStmt>(Move(startToken), Move(variable), Move(collection), Move(body));
}

UniquePtr<WhileStmt> Parser::parseWhileStatement() {
	auto startToken = previous_;

	consume(TokenType::LPAREN, "Expected '(' after while");

	auto condition = parseExpression();
	consume(TokenType::RPAREN, "Expected ')' after condition");

	auto body = parseBlock();
	return MakeUnique<WhileStmt>(Move(startToken), Move(condition), Move(body));
}

UniquePtr<ExprStmt> Parser::parseExprStatement() {
	auto startToken = current_;
	auto expr = parseExpression();

	// Consume ;
	consume(TokenType::SEMICOLON, "Expected ';' after expression");

	return MakeUnique<ExprStmt>(Move(startToken), Move(expr));
}

UniquePtr<NamespaceDeclStmt> Parser::parseNamespaceDecl() {
	auto startToken = previous_;

	// namespace xxx::xxx::xxx { }
	Vec<UniquePtr<IdentifierExpr>> path;

	do {
		// Read identifier
		path.push_back(MakeUnique<IdentifierExpr>(
			consume(TokenType::IDENTIFIER, "Expected identifier"))
		);
	} while (match(TokenType::OP_DOUBLE_COLON));

	// Namespace body
	auto body = parseBlock();
	return MakeUnique<NamespaceDeclStmt>(Move(startToken), Move(path), Move(body));
}

UniquePtr<DeclSpecStmt> Parser::parseDeclSpecStatement() {
	auto startToken = previous_;

	// __declspec(xxx)
	consume(TokenType::LPAREN, "Expected '(' after declspec");

	auto identifier = MakeUnique<IdentifierExpr>(consume(TokenType::IDENTIFIER, "Expected identifier"));

	consume(TokenType::RPAREN, "Expected ')' after declspec identifier");

	return MakeUnique<DeclSpecStmt>(Move(startToken), Move(identifier));
}

UniquePtr<UseStmt> Parser::parseUseStatement() {
	auto startToken = previous_;

	Vec<Vec<UniquePtr<IdentifierExpr>>> paths;

	do { // NMS1::x::y, NMS2::x::y
		Vec<UniquePtr<IdentifierExpr>> subPath;

		do { // nms1::nms2::...
			subPath.push_back(MakeUnique<IdentifierExpr>(
				consume(TokenType::IDENTIFIER, "Expected identifier"))
			);
		} while (match(TokenType::OP_DOUBLE_COLON));

		paths.push_back(Move(subPath));

	} while (match(TokenType::COMMA));

	// Check if an explicit file reference is used
	UniquePtr<LiteralExpr> file = nullptr;
	if (match(TokenType::KW_FROM)) {
		file = MakeUnique<LiteralExpr>(consume(TokenType::LIT_STRING, "Expected filename"));
	}

	consume(TokenType::SEMICOLON, "Expected ';' after use parseStatement");
	return MakeUnique<UseStmt>(Move(startToken), Move(paths), Move(file));
}

UniquePtr<ReturnStmt> Parser::parseReturnStatement() {
	auto startToken = previous_;

	UniquePtr<ExprNode> value = nullptr;
	if (!match(TokenType::SEMICOLON)) {
		value = parseExpression();
		consume(TokenType::SEMICOLON, "Expected ';' after return value");
	}

	return MakeUnique<ReturnStmt>(Move(startToken), Move(value));
}

UniquePtr<EnumDeclStmt> Parser::parseEnumDecl() {
	auto startToken = previous_;

	// enum<int> m { }

	// Check if type is given
	UniquePtr<TypeReferenceExpr> type = nullptr;
	if (match(TokenType::OP_LT)) {
		type = parseTypeReference();
		consume(TokenType::OP_GT, "Expected '>'");
	}

	auto name = MakeUnique<IdentifierExpr>(consume(TokenType::IDENTIFIER, "Expected identifier"));
	consume(TokenType::LBRACE, "Expected '{' after enum declaration");

	// Parse members
	decltype(EnumDeclStmt::members) members; // Vec<std::pair<UniquePtr<Identifier>, UniquePtr<Expression>>>
	if (!match(TokenType::RBRACE)) {
		do {
			auto memberName = MakeUnique<IdentifierExpr>(consume(TokenType::IDENTIFIER, "Expected identifier"));

			// Initializer?
			UniquePtr<ExprNode> memberInitializer = nullptr;
			if (match(TokenType::OP_EQ)) {
				memberInitializer = parseExpression();
			}

			members.push_back({ Move(memberName), Move(memberInitializer) });
		} while (match(TokenType::COMMA));

		consume(TokenType::RBRACE, "Expected '}' after enum members");
	}

	return MakeUnique<EnumDeclStmt>(Move(startToken), Move(name), Move(type), Move(members));
}

UniquePtr<TypeDeclStmt> Parser::parseTypeDecl() {
	auto type = getPrevious();

	// class xxx<> as xxx : xxx {}
	auto name = parseTypeReference();

	// Check for alias(es)
	Vec<UniquePtr<IdentifierExpr>> aliases;
	if (match(TokenType::KW_AS)) {
		do {
			aliases.push_back(MakeUnique<IdentifierExpr>(consume(TokenType::IDENTIFIER, "Expected identifier")));
		} while (match(TokenType::COMMA));
	}

	// Check for base types
	Vec<UniquePtr<TypeReferenceExpr>> baseTypes;
	if (match(TokenType::COLON)) {
		do {
			baseTypes.push_back(parseTypeReference());
		} while (match(TokenType::COMMA));
	}

	auto body = parseBlock();
	return MakeUnique<TypeDeclStmt>(Move(type), Move(name), Move(aliases), Move(baseTypes), Move(body));
}

UniquePtr<ExprNode> Parser::parseExpression() {
	return parseAssignment();
}

UniquePtr<ExprNode> Parser::parseAssignment() {
	auto expr = parseTernary();

	// Assignment operators (ex: "=", "+=", "-=")
	if (match(TokenType::OP_EQ) || match(TokenType::OP_PLUS_EQ) ||
		match(TokenType::OP_MINUS_EQ) || match(TokenType::OP_DIV_EQ) || match(TokenType::OP_MULT_EQ) ||
		match(TokenType::OP_INCREMENT) || match(TokenType::OP_DECREMENT)) {
		auto startToken = previous_;

		Token op = getPrevious();
		UniquePtr<ExprNode> value = nullptr;
		if (op.type != TokenType::OP_INCREMENT && op.type != TokenType::OP_DECREMENT) {
			value = parseAssignment();
		}

		return MakeUnique<AssignmentExpr>(Move(startToken), Move(expr), op, Move(value));
	}

	return expr;
}

UniquePtr<ExprNode> Parser::parseTernary() {
	auto expr = parseLogicalOr(); // a

	// a ? b : c"
	if (match(TokenType::OP_QUESTION)) {
		auto startToken = previous_;

		auto thenBranch = parseExpression(); // b
		consume(TokenType::COLON, "Expected ':' in parseTernary expression");
		auto elseBranch = parseTernary(); // Right-associative: c
		return MakeUnique<TernaryExpr>(Move(startToken), Move(expr), Move(thenBranch), Move(elseBranch));
	}

	return expr;
}

UniquePtr<ExprNode> Parser::parseLogicalOr() {
	auto expr = parseLogicalAnd();

	// a || b
	while (match(TokenType::OP_OR)) {
		auto startToken = previous_;

		Token op = getPrevious();
		auto right = parseLogicalAnd();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<ExprNode> Parser::parseLogicalAnd() {
	auto expr = parseBitwiseOr();

	// a && b
	while (match(TokenType::OP_AND)) {
		auto startToken = previous_;

		Token op = getPrevious();
		auto right = parseBitwiseOr();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<ExprNode> Parser::parseBitwiseOr() {
	auto expr = parseBitwiseXor();

	while (match(TokenType::OP_BOR)) {
		auto startToken = previous_;
		Token op = getPrevious();
		auto right = parseBitwiseXor();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<ExprNode> Parser::parseBitwiseXor() {
	auto expr = parseBitwiseAnd();

	while (match(TokenType::OP_BXOR)) {
		auto startToken = previous_;
		Token op = getPrevious();
		auto right = parseBitwiseAnd();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<ExprNode> Parser::parseBitwiseAnd() {
	auto expr = parseEquality();

	while (match(TokenType::OP_BAND)) {
		auto startToken = previous_;
		Token op = getPrevious();
		auto right = parseEquality();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<ExprNode> Parser::parseEquality() {
	auto expr = parseComparison();

	// a == b
	while (match(TokenType::OP_EQ_EQ) || match(TokenType::OP_NOT_EQ)) {
		auto startToken = previous_;

		Token op = getPrevious();
		auto right = parseComparison();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<ExprNode> Parser::parseComparison() {
	auto expr = parseShift();

	// a > b
	while (match(TokenType::OP_GT) || match(TokenType::OP_GE) || match(TokenType::OP_LT) || match(TokenType::OP_LE)) {
		auto startToken = previous_;

		Token op = getPrevious();
		auto right = parseShift();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<ExprNode> Parser::parseShift() {
	auto expr = parseTerm();

	while (match(TokenType::OP_SHL) || match(TokenType::OP_SHR)) {
		auto startToken = previous_;
		Token op = getPrevious();
		auto right = parseTerm();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<ExprNode> Parser::parseTerm() {
	auto expr = parseFactor();

	// a + b
	while (match(TokenType::OP_PLUS) || match(TokenType::OP_MINUS)) {
		auto startToken = previous_;

		Token op = getPrevious();
		auto right = parseFactor();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<ExprNode> Parser::parseFactor() {
	auto expr = parseUnary();

	// "a * b", "a / b", "a % b"
	while (match(TokenType::OP_ASTERISK) || match(TokenType::OP_SLASH) || match(TokenType::OP_MOD)) {
		auto startToken = previous_;

		Token op = getPrevious();
		auto right = parseUnary();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<ExprNode> Parser::parseUnary() {
	// !a -a
	if (match(TokenType::OP_NOT) || match(TokenType::OP_MINUS) || match(TokenType::OP_BNOT)) {
		auto startToken = previous_;

		Token op = getPrevious();
		auto right = parseUnary(); // Right-associative: operand
		return MakeUnique<UnaryExpr>(Move(startToken), op, Move(right));
	}

	return parsePrimary();
}

UniquePtr<ExprNode> Parser::parsePrimary() {
	// Interpolated string
	if (match(TokenType::INTERPOLATION)) {
		return parseMemberAccess(parseInterpolatedString());
	}

	// Literals
	if (match(TokenType::LIT_INT) || match(TokenType::LIT_FLOAT) || match(TokenType::LIT_BOOL)
		|| match(TokenType::LIT_STRING) || match(TokenType::LIT_NULL)) {
		return MakeUnique<LiteralExpr>(getPrevious());
	}

	// (a + b)
	if (match(TokenType::LPAREN)) {
		auto expr = parseExpression();
		consume(TokenType::RPAREN, "Expected ')' after expression");
		return parseMemberAccess(Move(expr));
	}

	// Identifiers
	if (match(TokenType::IDENTIFIER)) {
		auto identifier = MakeUnique<IdentifierExpr>(getPrevious());

		if (check(TokenType::OP_DOUBLE_COLON)) {
			return parseNamespaceAccess(Move(identifier));
		}

		if (check(TokenType::LPAREN)) {
			return parseFunctionCall(Move(identifier));
		}

		return parseMemberAccess(Move(identifier));
	}

	// arrays, objects, etc?
	if (match(TokenType::LBRACKET)) {
		return parseMemberAccess(parseArray());
	}

	throw error(current_, "Expected expression");
}

UniquePtr<ExprNode> Parser::parseFunctionCall(UniquePtr<ExprNode> target) {
	auto startToken = previous_;

	// Consume (
	consume(TokenType::LPAREN, "Expected '(' after function name");

	// Parse the arguments
	Vec<UniquePtr<ExprNode>> arguments;
	if (!check(TokenType::RPAREN)) {
		do {
			arguments.push_back(parseExpression()); // Parse each argument
		} while (match(TokenType::COMMA));
	}

	// Consume )
	consume(TokenType::RPAREN, "Expected ')' after arguments");

	return MakeUnique<CallExpr>(Move(startToken), Move(target), Move(arguments));
}

UniquePtr<ExprNode> Parser::parseNamespaceAccess(UniquePtr<IdentifierExpr> identifier) {
	auto startToken = previous_;

	Vec<UniquePtr<ExprNode>> path;
	path.push_back(Move(identifier));

	while (match(TokenType::OP_DOUBLE_COLON)) {
		auto identifier = MakeUnique<IdentifierExpr>(consume(TokenType::IDENTIFIER, "Expected an identifier"));

		// Is this our last identifier?
		if (check(TokenType::OP_DOUBLE_COLON)) {
			path.push_back(Move(identifier));
		}
		else if (check(TokenType::LPAREN)) {
			path.push_back(parseFunctionCall(Move(identifier)));
		}
		else {
			path.push_back(parseMemberAccess(Move(identifier)));
		}
	}

	return MakeUnique<NamespaceAccessExpr>(Move(startToken), Move(path));
}

UniquePtr<ExprNode> Parser::parseMemberAccess(UniquePtr<ExprNode> target) {
	while (true) {
		if (match(TokenType::OP_DOT) || match(TokenType::OP_ARROW)) {
			auto startToken = previous_;

			Token op = getPrevious();
			auto member = MakeUnique<IdentifierExpr>(consume(TokenType::IDENTIFIER, "Expected member name"));

			target = MakeUnique<MemberAccessExpr>(Move(startToken), Move(target), op, Move(member));

			// check if it is a function call
			if (check(TokenType::LPAREN)) {
				target = parseFunctionCall(Move(target));
			}
		}
		else if (match(TokenType::LBRACKET)) {
			target = parseArrayAccess(Move(target));
		}
		else {
			break;
		}
	}

	return target;
}

UniquePtr<ExprNode> Parser::parseArrayAccess(UniquePtr<ExprNode> target) {
	auto startToken = previous_; // '['
	auto index = parseExpression();
	consume(TokenType::RBRACKET, "Expected ']' after index");
	return MakeUnique<ArrayAccessExpr>(Move(startToken), Move(target), Move(index));
}

UniquePtr<ExprNode> Parser::parseArray() {
	auto startToken = previous_;

	Vec<UniquePtr<ExprNode>> elements;
	if (!match(TokenType::RBRACKET)) {
		do {
			elements.push_back(parseExpression());
		} while (match(TokenType::COMMA));

		consume(TokenType::RBRACKET, "Expected ']'");
	}

	return MakeUnique<ArrayExpr>(Move(startToken), Move(elements));
}

UniquePtr<ExprNode> Parser::parseInterpolatedString() {
	auto startToken = previous_;

	// Good ol' c# spec
	auto str = consume(TokenType::LIT_STRING, "Expected string");

	Vec<UniquePtr<ExprNode>> parts;

	auto rawString = str.lexeme;
	size_t pos = 0;

	while (pos < rawString.size()) {
		if (rawString[pos] == '{') {
			// Handle expression inside {}
			size_t endPos = rawString.find('}', pos);
			if (endPos == Str::npos) {
				throw error(current_, "Unterminated interpolation expression");
			}

			auto exprStr = rawString.substr(pos + 1, endPos - pos - 1);

			// Create a temporary lexer for the expression
			Lexer exprLexer(exprStr);
			exprLexer.tokenize();

			Parser exprParser(Move(exprLexer.moveTokens()));
			parts.push_back(exprParser.parseExpression());

			pos = endPos + 1;
		}
		else {
			// Handle literal part
			size_t endPos = rawString.find('{', pos);
			if (endPos == Str::npos) {
				endPos = rawString.size();
			}

			auto literalStr = rawString.substr(pos, endPos - pos);
			Token literalToken(TokenType::LIT_STRING, literalStr, { 1, 1, 1 });
			parts.push_back(MakeUnique<LiteralExpr>(literalToken));

			pos = endPos;
		}
	}

	return MakeUnique<InterpolatedStringExpr>(Move(startToken), Move(parts));
}

MRK_NS_END