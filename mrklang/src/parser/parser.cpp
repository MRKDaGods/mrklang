#include "parser.h"
#include "common/logging.h"

#include <sstream>

MRK_NS_BEGIN

Parser::Parser(const Lexer* lexer)
	: lexer_(lexer), tokens_(lexer->tokens()), currentPos_(0) {
	// initialize current, prev is EOF by default
	if (!tokens_.empty()) {
		current_ = tokens_[0];
	}
}

UniquePtr<Program> Parser::parse() {
	auto program = MakeUnique<Program>();

	while (!match(TokenType::END_OF_FILE)) {
		try {
			// try parse top level declaration
			auto stmt = topLevelDecl();
			if (stmt) {
				program->statements.push_back(Move(stmt));
			}
		}
		catch (const ParserError& err) {
			 // recover
			synchronize();

			// save for future analysis
			errors_.push_back(err);
		}
	}

	return program;
}

const Vec<ParserError>& Parser::errors() const {
	return errors_;
}

void Parser::reportErrors() const {
	Vec<Str> lines;
	MRK_STD istringstream sourceStream(lexer_->source());
	Str line;
	while (MRK_STD getline(sourceStream, line)) {
		lines.push_back(line);
	}

	for (const auto& err : errors_) {
		if (err.token.position.line > lines.size()) continue; // Skip invalid line numbers

		// Strip leading whitespace
		Str strippedLine = lines[err.token.position.line - 1];
		strippedLine.erase(0, strippedLine.find_first_not_of(" \t"));

		// Adjust col
		size_t indentation = lines[err.token.position.line - 1].find_first_not_of(" \t");
		if (indentation == Str::npos) {
			indentation = 0;
		}

		MRK_STD cerr << "Line: " << err.token.position.line << ", Col: " << err.token.position.column << "\n";
		MRK_STD cerr << strippedLine << "\n";

		// Squiggles
		int squiggleStart = MRK_STD max(0, (int)(err.token.position.column - 1 - indentation));

		MRK_STD cerr << Str(squiggleStart, ' ')	// Leading spaces
			<< Str(err.token.lexeme.size(), '~')	// Squiggles
			<< "  // Error: " << err.what() << "\n\n";		// Error message
	}
}

ParserError Parser::error(const Token& token, const Str& message) {
	MRK_ERROR("Parser error at {}:{} (length {}) - {}",
		token.position.line, token.position.column, token.lexeme.size(), message);

	return ParserError(token, message);
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

const Token& Parser::previous() const {
	return previous_;
}

const Token& Parser::peekNext() const {
	auto idx = currentPos_ + 1;
	return idx < tokens_.size() ? tokens_[idx] : Token();
}

UniquePtr<Statement> Parser::topLevelDecl() {
	// TODO: only use statement() ?

	if (match(TokenType::KW_USE)) {
		return useStatement();
	}

	return statement();
}

UniquePtr<LangBlock> Parser::langBlock() {
	auto startToken = previous_;
	auto language = previous_.lexeme;

	consume(TokenType::LIT_LANG_BLOCK, "Invalid language block");

	auto rawCode = previous_.lexeme;
	return MakeUnique<LangBlock>(Move(startToken), Move(language), Move(rawCode));
}

UniquePtr<VarDecl> Parser::varDecl(bool requireSemicolon) {
	auto startToken = previous_;

	// var x = 9; <-- inference
	// var<int> x = 9;

	// Check for explicit type
	UniquePtr<TypeName> typeName = nullptr;
	if (match(TokenType::OP_LT)) {
		typeName = parseTypeName();

		consume(TokenType::OP_GT, "Expected '>' after type");
	}

	// Consume var name
	auto name = MakeUnique<Identifier>(
		consume(TokenType::IDENTIFIER, "Expected variable name")
	);

	// Consume initializer if exists
	UniquePtr<Expression> initializer = nullptr;
	if (match(TokenType::OP_EQ)) {
		initializer = expression();
	}

	if (requireSemicolon) {
		consume(TokenType::SEMICOLON, "Expected ';' after declaration");
	}

	return MakeUnique<VarDecl>(Move(startToken), Move(typeName), Move(name), Move(initializer));
}

UniquePtr<FunctionDecl> Parser::functionDecl() {
	// func mrk(int m, string x = "", params string[] xz) -> int {} 
	auto startToken = previous_;

	// Parse name
	auto name = MakeUnique<Identifier>(consume(TokenType::IDENTIFIER, "Expected function name"));

	// Consume (
	consume(TokenType::LPAREN, "Expected '(' after function name");

	// Parse params
	Vec<UniquePtr<FunctionParamDecl>> parameters;

	if (!check(TokenType::RPAREN)) {
		do {
			parameters.push_back(functionParamDecl());
		} while (match(TokenType::COMMA));
	}

	// Consume )
	consume(TokenType::RPAREN, "Expected ')' after parameters");

	// Parse optional return type
	UniquePtr<TypeName> returnType = nullptr;
	if (match(TokenType::OP_ARROW)) {
		returnType = parseTypeName();
	}

	// Parse body
	auto body = block();
	return MakeUnique<FunctionDecl>(Move(startToken), Move(name), Move(parameters), Move(returnType), Move(body));
}

UniquePtr<TypeName> Parser::parseTypeName() {
	auto startToken = current_;

	Vec<UniquePtr<Identifier>> identifiers;

	// Parse identifier
	identifiers.push_back(
		MakeUnique<Identifier>(consume(TokenType::IDENTIFIER, "Expected type identifier"))
	);

	// Parse namespace qualifiers if exist
	while (match(TokenType::OP_DOUBLE_COLON)) {
		identifiers.push_back(
			MakeUnique<Identifier>(consume(TokenType::IDENTIFIER, "Expected type identifier after '::'"))
		);
	}

	Vec<UniquePtr<TypeName>> genericArgs;

	// Parse generic arguments if exist (ex: "mrkstl::boxed<mrkstl::boxed<int32>>")
	if (match(TokenType::OP_LT)) {
		do {
			genericArgs.push_back(parseTypeName());
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

	return MakeUnique<TypeName>(Move(startToken), Move(identifiers), Move(genericArgs), Move(pointerRank), Move(arrayRank));
}

UniquePtr<FunctionParamDecl> Parser::functionParamDecl() {
	// Check whether this parameter has params
	bool isParams = match(TokenType::KW_PARAMS);

	auto startToken = isParams ? previous_ : current_;

	auto type = parseTypeName();
	auto name = MakeUnique<Identifier>(consume(TokenType::IDENTIFIER, "Expected identifier"));

	UniquePtr<Expression> initializer = nullptr;
	if (match(TokenType::OP_EQ)) {
		initializer = expression();
	}

	return MakeUnique<FunctionParamDecl>(Move(startToken), Move(type), Move(name), Move(initializer), Move(isParams));
}

UniquePtr<AccessModifier> Parser::accessModifier() {
	auto startToken = current_;

	Vec<Token> modifiers;

	while (current_.isAccessModifier()) {
		modifiers.push_back(current_);
		advance();
	}

	return MakeUnique<AccessModifier>(Move(startToken), Move(modifiers));
}

UniquePtr<Statement> Parser::statement() {
	// Incase of nested namespaces
	if (match(TokenType::KW_NAMESPACE)) {
		return namespaceDecl();
	}

	// check for access modifiers
	if (current_.isAccessModifier()) {
		return accessModifier();
	}

	if (match(TokenType::KW_ENUM)) {
		return enumDecl();
	}

	if (match(TokenType::KW_CLASS) || match(TokenType::KW_STRUCT)) {
		return typeDecl();
	}

	if (match(TokenType::KW_FUNC)) {
		return functionDecl();
	}

	if (match(TokenType::KW_VAR)) {
		return varDecl();
	}

	if (match(TokenType::LBRACE)) {
		return block(false);
	}

	if (match(TokenType::KW_IF)) {
		return ifStatement();
	}

	if (match(TokenType::KW_FOR)) {
		return forStatement();
	}

	if (match(TokenType::KW_FOREACH)) {
		return foreachStatement();
	}

	if (match(TokenType::KW_WHILE)) {
		return whileStatement();
	}

	if (match(TokenType::KW_DECLSPEC)) {
		return declSpecStatement();
	}

	// lang blocks
	if (match(TokenType::BLOCK_CPP) || match(TokenType::BLOCK_CSHARP) ||
		match(TokenType::BLOCK_DART) || match(TokenType::BLOCK_JS)) {
		return langBlock();
	}

	return exprStatement();
}

UniquePtr<Block> Parser::block(bool consumeBrace) {
	auto startToken = consumeBrace ? current_ : previous_;

	if (consumeBrace) { // Consume brace if not already consumed
		consume(TokenType::LBRACE, "Expected '{' at beginning of block");
	}

	Vec<UniquePtr<Statement>> statements;
	while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
		statements.push_back(statement());
	}

	consume(TokenType::RBRACE, "Expected '}' after block");
	return MakeUnique<Block>(Move(startToken), Move(statements));
}

UniquePtr<IfStmt> Parser::ifStatement() {
	auto startToken = previous_;

	consume(TokenType::LPAREN, "Expected '(' after if");

	auto condition = expression();
	consume(TokenType::RPAREN, "Expected ')' after condition");

	auto thenBlock = block();

	UniquePtr<Block> elseBlock = nullptr;
	if (match(TokenType::KW_ELSE)) {
		elseBlock = block();
	}

	return MakeUnique<IfStmt>(Move(startToken), Move(condition), Move(thenBlock), Move(elseBlock));
}

UniquePtr<ForStmt> Parser::forStatement() { // for (var<int> i = 0; i < 9999; i++)
	auto startToken = previous_;
	consume(TokenType::LPAREN, "Expected '(' after for");

	// Initializer
	UniquePtr<VarDecl> init = nullptr;
	if (match(TokenType::KW_VAR)) {
		init = varDecl();
	}

	// Condition
	UniquePtr<Expression> condition = nullptr;
	if (!check(TokenType::SEMICOLON)) {
		condition = expression();
	}

	consume(TokenType::SEMICOLON, "Expected ';' after for condition");

	// Increment
	UniquePtr<Expression> increment = nullptr;
	if (!check(TokenType::RPAREN)) {
		increment = expression();
	}

	consume(TokenType::RPAREN, "Expected ')' after for clauses");

	auto body = block();
	return MakeUnique<ForStmt>(Move(startToken), Move(init), Move(condition), Move(increment), Move(body));
}

UniquePtr<ForeachStmt> Parser::foreachStatement() { // foreach (var x in expr()) or foreach (expr()) 
	auto startToken = previous_;
	consume(TokenType::LPAREN, "Expected '(' after foreach");

	UniquePtr<VarDecl> variable = nullptr;
	if (match(TokenType::KW_VAR)) {
		variable = varDecl(false);
		consume(TokenType::KW_IN, "Expected 'in' after variable declaration");
	}

	auto collection = expression();
	consume(TokenType::RPAREN, "Expected ')' after foreach clause");

	auto body = block();
	return MakeUnique<ForeachStmt>(Move(startToken), Move(variable), Move(collection), Move(body));
}

UniquePtr<WhileStmt> Parser::whileStatement() {
	auto startToken = previous_;

	consume(TokenType::LPAREN, "Expected '(' after while");

	auto condition = expression();
	consume(TokenType::RPAREN, "Expected ')' after condition");

	auto body = block();
	return MakeUnique<WhileStmt>(Move(startToken), Move(condition), Move(body));
}

UniquePtr<ExprStmt> Parser::exprStatement() {
	auto startToken = current_;
	auto expr = expression();

	// Consume ;
	consume(TokenType::SEMICOLON, "Expected ';' after expression");

	return MakeUnique<ExprStmt>(Move(startToken), Move(expr));
}

UniquePtr<NamespaceDecl> Parser::namespaceDecl() {
	auto startToken = previous_;

	// namespace xxx::xxx::xxx { }
	Vec<UniquePtr<Identifier>> path;

	do {
		// Read identifier
		path.push_back(MakeUnique<Identifier>(
			consume(TokenType::IDENTIFIER, "Expected identifier"))
		);
	} while (match(TokenType::OP_DOUBLE_COLON));

	// Namespace body
	auto body = block();
	return MakeUnique<NamespaceDecl>(Move(startToken), Move(path), Move(body));
}

UniquePtr<DeclarationSpec> Parser::declSpecStatement() {
	auto startToken = previous_;

	// __declspec(xxx)
	consume(TokenType::LPAREN, "Expected '(' after declspec");

	auto identifier = MakeUnique<Identifier>(consume(TokenType::IDENTIFIER, "Expected identifier"));

	consume(TokenType::RPAREN, "Expected ')' after declspec identifier");

	return MakeUnique<DeclarationSpec>(Move(startToken), Move(identifier));
}

UniquePtr<UseStmt> Parser::useStatement() {
	auto startToken = previous_;

	Vec<Vec<UniquePtr<Identifier>>> paths;

	do { // NMS1::x::y, NMS2::x::y
		Vec<UniquePtr<Identifier>> subPath;

		do { // nms1::nms2::...
			subPath.push_back(MakeUnique<Identifier>(
				consume(TokenType::IDENTIFIER, "Expected identifier"))
			);
		} while (match(TokenType::OP_DOUBLE_COLON));

		paths.push_back(Move(subPath));

	} while (match(TokenType::COMMA));

	// Check if an explicit file reference is used
	UniquePtr<Literal> file = nullptr;
	if (match(TokenType::KW_FROM)) {
		file = MakeUnique<Literal>(consume(TokenType::LIT_STRING, "Expected filename"));
	}

	consume(TokenType::SEMICOLON, "Expected ';' after use statement");
	return MakeUnique<UseStmt>(Move(startToken), Move(paths), Move(file));
}

UniquePtr<EnumDecl> Parser::enumDecl() {
	auto startToken = previous_;

	// enum<int> m { }

	// Check if type is given
	UniquePtr<TypeName> type = nullptr;
	if (match(TokenType::OP_LT)) {
		type = parseTypeName();
		consume(TokenType::OP_GT, "Expected '>'");
	}

	auto name = MakeUnique<Identifier>(consume(TokenType::IDENTIFIER, "Expected identifier"));
	consume(TokenType::LBRACE, "Expected '{' after enum declaration");

	// Parse members
	decltype(EnumDecl::members) members; // Vec<MRK_STD pair<UniquePtr<Identifier>, UniquePtr<Expression>>>
	if (!match(TokenType::RBRACE)) {
		do {
			auto memberName = MakeUnique<Identifier>(consume(TokenType::IDENTIFIER, "Expected identifier"));

			// Initializer?
			UniquePtr<Expression> memberInitializer = nullptr;
			if (match(TokenType::OP_EQ)) {
				memberInitializer = expression();
			}

			members.push_back({ Move(memberName), Move(memberInitializer) });
		} while (match(TokenType::COMMA));

		consume(TokenType::RBRACE, "Expected '}' after enum members");
	}

	return MakeUnique<EnumDecl>(Move(startToken), Move(name), Move(type), Move(members));
}

UniquePtr<TypeDecl> Parser::typeDecl() {
	auto type = previous();

	// class xxx<> as xxx : xxx {}
	auto name = parseTypeName();

	// Check for alias(es)
	Vec<UniquePtr<Identifier>> aliases;
	if (match(TokenType::KW_AS)) {
		do {
			aliases.push_back(MakeUnique<Identifier>(consume(TokenType::IDENTIFIER, "Expected identifier")));
		} while (match(TokenType::COMMA));
	}

	// Check for base types
	Vec<UniquePtr<TypeName>> baseTypes;
	if (match(TokenType::COLON)) {
		do {
			baseTypes.push_back(parseTypeName());
		} while (match(TokenType::COMMA));
	}

	auto body = block();
	return MakeUnique<TypeDecl>(Move(type), Move(name), Move(aliases), Move(baseTypes), Move(body));
}

UniquePtr<Expression> Parser::expression() {
	return assignment();
}

UniquePtr<Expression> Parser::assignment() {
	auto expr = ternary();

	// Assignment operators (ex: "=", "+=", "-=")
	if (match(TokenType::OP_EQ) || match(TokenType::OP_PLUS_EQ) ||
		match(TokenType::OP_MINUS_EQ) || match(TokenType::OP_DIV_EQ) || match(TokenType::OP_MULT_EQ) ||
		match(TokenType::OP_INCREMENT) || match(TokenType::OP_DECREMENT)) {
		auto startToken = previous_;

		Token op = previous();
		UniquePtr<Expression> value = nullptr;
		if (op.type != TokenType::OP_INCREMENT && op.type != TokenType::OP_DECREMENT) {
			value = assignment();
		}

		return MakeUnique<AssignmentExpr>(Move(startToken), Move(expr), op, Move(value));
	}

	return expr;
}

UniquePtr<Expression> Parser::ternary() {
	auto expr = logicalOr(); // a

	// a ? b : c"
	if (match(TokenType::OP_QUESTION)) {
		auto startToken = previous_;

		auto thenBranch = expression(); // b
		consume(TokenType::COLON, "Expected ':' in ternary expression");
		auto elseBranch = ternary(); // Right-associative: c
		return MakeUnique<TernaryExpr>(Move(startToken), Move(expr), Move(thenBranch), Move(elseBranch));
	}

	return expr;
}

UniquePtr<Expression> Parser::logicalOr() {
	auto expr = logicalAnd();

	// a || b
	while (match(TokenType::OP_OR)) {
		auto startToken = previous_;

		Token op = previous();
		auto right = logicalAnd();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<Expression> Parser::logicalAnd() {
	auto expr = equality();

	// a && b
	while (match(TokenType::OP_AND)) {
		auto startToken = previous_;

		Token op = previous();
		auto right = equality();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<Expression> Parser::equality() {
	auto expr = comparison();

	// a == b
	while (match(TokenType::OP_EQ_EQ) || match(TokenType::OP_NOT_EQ)) {
		auto startToken = previous_;

		Token op = previous();
		auto right = comparison();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<Expression> Parser::comparison() {
	auto expr = term();

	// a > b
	while (match(TokenType::OP_GT) || match(TokenType::OP_GE) || match(TokenType::OP_LT) || match(TokenType::OP_LE)) {
		auto startToken = previous_;

		Token op = previous();
		auto right = term();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<Expression> Parser::term() {
	auto expr = factor();

	// a + b
	while (match(TokenType::OP_PLUS) || match(TokenType::OP_MINUS)) {
		auto startToken = previous_;

		Token op = previous();
		auto right = factor();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<Expression> Parser::factor() {
	auto expr = unary();

	// "a * b", "a / b", "a % b"
	while (match(TokenType::OP_ASTERISK) || match(TokenType::OP_SLASH) || match(TokenType::OP_MOD)) {
		auto startToken = previous_;

		Token op = previous();
		auto right = unary();
		expr = MakeUnique<BinaryExpr>(Move(startToken), Move(expr), op, Move(right));
	}

	return expr;
}

UniquePtr<Expression> Parser::unary() {
	// !a -a
	if (match(TokenType::OP_NOT) || match(TokenType::OP_MINUS)) {
		auto startToken = previous_;

		Token op = previous();
		auto right = unary(); // Right-associative: operand
		return MakeUnique<UnaryExpr>(Move(startToken), op, Move(right));
	}

	return primary();
}

UniquePtr<Expression> Parser::primary() {
	// Interpolated string
	if (match(TokenType::INTERPOLATION)) {
		return memberAccess(interpolatedString());
	}

	// Literals
	if (match(TokenType::LIT_INT) || match(TokenType::LIT_FLOAT) || match(TokenType::LIT_BOOL)
		|| match(TokenType::LIT_STRING) || match(TokenType::LIT_NULL)) {
		return MakeUnique<Literal>(previous());
	}

	// (a + b)
	if (match(TokenType::LPAREN)) {
		auto expr = expression();
		consume(TokenType::RPAREN, "Expected ')' after expression");
		return memberAccess(Move(expr));
	}

	// Identifiers
	if (match(TokenType::IDENTIFIER)) {
		auto identifier = MakeUnique<Identifier>(previous());

		if (check(TokenType::OP_DOUBLE_COLON)) {
			return namespaceAccess(Move(identifier));
		}

		if (check(TokenType::LPAREN)) {
			return functionCall(Move(identifier));
		}

		return memberAccess(Move(identifier));
	}

	// arrays, objects, etc?
	if (match(TokenType::LBRACKET)) {
		return memberAccess(array());
	}

	throw error(current_, "Expected expression");
}

UniquePtr<Expression> Parser::functionCall(UniquePtr<Expression> target) {
	auto startToken = previous_;

	// Consume (
	consume(TokenType::LPAREN, "Expected '(' after function name");

	// Parse the arguments
	Vec<UniquePtr<Expression>> arguments;
	if (!check(TokenType::RPAREN)) {
		do {
			arguments.push_back(expression()); // Parse each argument
		} while (match(TokenType::COMMA));
	}

	// Consume )
	consume(TokenType::RPAREN, "Expected ')' after arguments");

	return MakeUnique<FunctionCall>(Move(startToken), Move(target), Move(arguments));
}

UniquePtr<Expression> Parser::namespaceAccess(UniquePtr<Identifier> identifier) {
	auto startToken = previous_;

	Vec<UniquePtr<Expression>> path;
	path.push_back(Move(identifier));

	while (match(TokenType::OP_DOUBLE_COLON)) {
		auto identifier = MakeUnique<Identifier>(consume(TokenType::IDENTIFIER, "Expected an identifier"));

		// Is this our last identifier?
		if (check(TokenType::OP_DOUBLE_COLON)) {
			path.push_back(Move(identifier));
		}
		else if (check(TokenType::LPAREN)) {
			path.push_back(functionCall(Move(identifier)));
		}
		else {
			path.push_back(memberAccess(Move(identifier)));
		}
	}

	return MakeUnique<NamespaceAccess>(Move(startToken), Move(path));
}

UniquePtr<Expression> Parser::memberAccess(UniquePtr<Expression> target) {
	while (match(TokenType::OP_DOT) || match(TokenType::OP_ARROW)) {
		auto startToken = previous_;

		Token op = previous();
		auto member = MakeUnique<Identifier>(consume(TokenType::IDENTIFIER, "Expected member name"));

		target = MakeUnique<MemberAccess>(Move(startToken), Move(target), op, Move(member));

		// check if it is a function call
		if (check(TokenType::LPAREN)) {
			target = functionCall(Move(target));
		}
	}

	return target;
}

UniquePtr<Expression> Parser::array() {
	auto startToken = previous_;

	Vec<UniquePtr<Expression>> elements;
	if (!match(TokenType::RBRACKET)) {
		do {
			elements.push_back(expression());
		} while (match(TokenType::COMMA));

		consume(TokenType::RBRACKET, "Expected ']'");
	}

	return MakeUnique<ArrayExpr>(Move(startToken), Move(elements));
}

UniquePtr<Expression> Parser::interpolatedString() {
	auto startToken = previous_;

	// Good ol' c# spec
	auto str = consume(TokenType::LIT_STRING, "Expected string");

	Vec<UniquePtr<Expression>> parts;

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

			Parser exprParser(&exprLexer);
			parts.push_back(exprParser.expression());

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
			parts.push_back(MakeUnique<Literal>(literalToken));

			pos = endPos;
		}
	}

	return MakeUnique<InterpolatedString>(Move(startToken), Move(parts));
}

MRK_NS_END