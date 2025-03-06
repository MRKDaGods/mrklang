#include "lexer.h"
#include "token_lookup.h"
#include "lexer_logging.h"
#include "core/error_reporter.h"

#include <algorithm>

#define INVALID_CHAR ((char)-1)

#define START_POSITION __startPos
#define END_POSITION __endPos
#define DELTA_POSITION (__endPos - __startPos)

/// Stores the starting position without pushing into tree
#define MARK_START() auto START_POSITION = position_

#define BEGIN_READ_CONTEXT() auto START_POSITION = positionTree_.pushPosition()
#define END_READ_CONTEXT() auto END_POSITION = position_; \
						   positionTree_.popPosition()

MRK_NS_BEGIN

Lexer::Lexer(const Str& source, uint32_t maxErrors)
	: source_(source), position_({ 0, 1, 1 }), maxErrors_(maxErrors), positionTree_(this) {
}

const Vec<Token>& Lexer::tokenize() {
	while (!isAtEnd()) {
		// Skip till a valid character is found
		skip();

		// check again
		if (isAtEnd()) {
			break;
		}

		BEGIN_READ_CONTEXT();

		// Test the character
		char ch = peek();

		if (isdigit(ch)) { // number literal
			readNumberLiteral();
		}
		else if (isIdentifierCharacter(ch)) { // keyword/identifier/(bool)literal
			readIdentifierOrKeyword();
		}
		else if (isOperatorOrPunctuation(ch)) {
			readOperatorOrPunctuation();
		}
		else if (isCharOrStringCharacter(ch)) {
			readCharOrStringLiteral();
		}
		else {
			//MRK_ERROR_LEX("Unknown char '{}'", ch);
			error("Unknown character", START_POSITION);
			advance();
		}

		END_READ_CONTEXT();
	}

	// Add EOF
	addToken(TokenType::END_OF_FILE, position_);

	return tokens_;
}

const LexerPosition& Lexer::getPosition() const {
	return position_;
}

const Vec<Token>& Lexer::getTokens() const {
	return tokens_;
}

Vec<Token>&& Lexer::moveTokens() {
	return Move(tokens_);
}

const Str& Lexer::getSource() const {
	return source_;
}

void Lexer::addToken(TokenType type, Str& lexeme, const LexerPosition& position) {
	tokens_.push_back({ type, std::move(lexeme), position });
}

void Lexer::addToken(TokenType type, const LexerPosition& position) {
	tokens_.push_back({ type, "", position });
}

void Lexer::error(const Str& message, const LexerPosition& position, uint32_t length) {
	auto& errorReporter = ErrorReporter::instance();
	if (errorReporter.errorCount() > maxErrors_) return;

	errorReporter.lexicalError(message, position, length);
}

bool Lexer::isAtEnd() {
	return position_.index >= source_.size();
}

char Lexer::advance(const size_t increment) {
	const size_t endPos = std::min(position_.index + increment, source_.size());

	for (size_t i = position_.index; i < endPos; i++) {
		if (source_[i] == '\n') {
			position_.line++;
			position_.column = 1;
		}
		else {
			position_.column++;
		}
	}

	char ch = peek();
	position_.index = endPos;
	return ch;
}

char Lexer::advance(const Str& str) {
	return advance(str.size());
}

void Lexer::readNumberLiteral() {
	MARK_START();

	bool isFloatingPoint = false;
	bool isHex = false;
	Str numberBuffer;

	while (!isAtEnd()) {
		char ch = tolower(peek());

		if (isdigit(ch)) {
			numberBuffer += ch;
		}
		else if (ch == '.') { // Check for floating point number
			// Check if we already have a floating point number or in a hex context
			if (isFloatingPoint || isHex) {
				break;
			}

			isFloatingPoint = true;
			numberBuffer += ch;
		}
		else if (ch == 'x') { // Check for hex number
			// Check if we're in a floating point context or in an invalid hex context
			// numberBuffer must be equal to "0"
			if (isFloatingPoint || numberBuffer != "0") {
				break;
			}

			isHex = true;
			numberBuffer += ch;
		}
		else if (isHex && isHexCharacter(ch)) { // Check for hex characters
			numberBuffer += ch;					// valid only when isHex is true
		}
		else { // Any other invalid character
			break;
		}

		// Advance char
		advance();
	}

	TokenType type = isFloatingPoint ? TokenType::LIT_FLOAT
		: isHex ? TokenType::LIT_HEX
		: TokenType::LIT_INT;

	addToken(type, numberBuffer, START_POSITION);
}

void Lexer::readIdentifierOrKeyword() {
	BEGIN_READ_CONTEXT();

	while (!isAtEnd() && isIdentifierCharacter(peek())) {
		advance();
	}

	END_READ_CONTEXT();

	// extract lexeme
	Str lexeme = source_.substr(START_POSITION.index, static_cast<size_t>(DELTA_POSITION.index));

	// Assign type, IDENTIFIER by default
	TokenType type = TokenType::IDENTIFIER;

	// Test for bool literal
	if (lexeme == "true" || lexeme == "false") {
		type = TokenType::LIT_BOOL;
	}
	else { // Test for keyword
		auto& keywords = TokenLookup::keywords();
		for (auto& [kwType, kw] : keywords) {
			if (lexeme == kw) {
				type = kwType;
				break;
			}
		}
	}

	addToken(type, lexeme, START_POSITION);

	// check for language blocks
	if (isLanguageBlockType(type)) {
		readLanguageBlock();
	}
}

void Lexer::readLanguageBlock() {
	skip();

	if (peek() != '{') {
		error("Expected '{'", position_);
		return;
	}

	BEGIN_READ_CONTEXT();

	advance();

	uint32_t depth = 1;
	while (!isAtEnd()) {
		char ch = advance();
		if (ch == '{') {
			depth++;
		}
		else if (ch == '}') {
			depth--;
		}

		if (depth == 0) {
			break;
		}
	}

	END_READ_CONTEXT();

	if (depth > 0) {
		error("Invalid language parseBlock", START_POSITION, DELTA_POSITION.index);
		return;
	}

	Str parseBlock = source_.substr(START_POSITION.index, DELTA_POSITION.index);
	addToken(TokenType::LIT_LANG_BLOCK, parseBlock, START_POSITION);

	// __declspec(SKIP) __csharp {
	// }
}

void Lexer::readOperatorOrPunctuation() {
	MARK_START();

	TokenType type = TokenType::ERROR;
	Str lexeme;

	// We read chars at cur+0 and cur+1 for potential operators like + or ++
	char buf[3];
	sprintf_s(buf, 3, "%c%c", peek(), peek(1));

	// Priority is given to a valid 2 char operator
	auto& operators = TokenLookup::operators();
	for (auto& [opType, op] : operators) {
		if (std::string_view(buf, op.size()) == op) {
			type = opType;
			lexeme = op;

			if (op.size() == 2) {
				advance();
				break;
			}
		}
	}

	advance();

	if (type == TokenType::COMMENT_SINGLE || type == TokenType::COMMENT_MULTI_START) {
		readComment(type);
	}
	else {
		addToken(type, lexeme, START_POSITION);
	}
}

void Lexer::readComment(TokenType commentType) {
	bool multiLineEndFound = false;

	BEGIN_READ_CONTEXT();

	while (!isAtEnd()) {
		char ch = advance();

		if (commentType == TokenType::COMMENT_SINGLE && ch == '\n') {
			break;
		}
		
		if (commentType == TokenType::COMMENT_MULTI_START && ch == '*' && peek() == '/') {
			advance(); // Skip '/'

			multiLineEndFound = true;
			break;
		}
	}

	END_READ_CONTEXT();

	if (commentType == TokenType::COMMENT_MULTI_START && !multiLineEndFound) {
		error("Unclosed multiline comment block", START_POSITION, DELTA_POSITION.index);
	}
}

void Lexer::readCharOrStringLiteral() {
	BEGIN_READ_CONTEXT();

	const char strChar = peek(); // ' or "
	advance(); // Skip strchar

	Str buf;
	char ch;
	while (!isAtEnd() && (ch = peek()) != strChar) {
		// Check for an escape sequence
		if (ch == '\\') {
			positionTree_.pushPosition();
			{
				advance(); // Skip backlash
				buf += readEscapeSequence();
			}
			positionTree_.popPosition();
		}
		else {
			buf += ch;
			advance();
		}
	}

	END_READ_CONTEXT();

	if (isAtEnd()) {
		error("Unterminated string literal", START_POSITION, DELTA_POSITION.index);
		return;
	}

	advance(); // strChar again

	TokenType type = strChar == '\'' ? TokenType::LIT_CHAR : TokenType::LIT_STRING;
	if (type == TokenType::LIT_CHAR && buf.size() != 1) {
		error("Expecting char", START_POSITION, DELTA_POSITION.index + 1); // compensate for strChar
		return;
	}

	addToken(type, buf, START_POSITION);
}

char Lexer::readEscapeSequence() {
	if (isAtEnd()) {
		error("Invalid escape sequence", positionTree_.currentPosition());
		return INVALID_CHAR;
	}

	char ch = advance();
	switch (ch) {
		// Common escape sequences
		case 'n':
			return '\n';

		case 't':
			return '\t';

		case '\\':
			return '\\';

		case '"':
			return '"';

		case '\'':
			return '\'';

		case 'r':
			return '\r';

		case 'b':
			return '\b';

		case 'f':
			return '\f';

		case 'v':
			return '\v';

		case 'a':
			return '\a';


		// Unicode/hex escape sequence
		case 'u': // \uXXXX
			return readCodepointEscape(4);

		case 'U': // \UXXXXXXXX
			return readCodepointEscape(8);

		case 'x': // \xXX 
			return readCodepointEscape(2);


		// Octal escape sequences
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
			return readOctalEscape(ch);


		// Invalid escape sequence
		default:
			error("Invalid escape sequence", positionTree_.currentPosition());
			return INVALID_CHAR;
	}
}

char Lexer::readCodepointEscape(int len) {
	// Advance if we're still at xuU
	char ch = peek();
	if (ch == 'x' || ch == 'u' || ch == 'U') {
		advance();
	}

	Str hex;

	for (int i = 0; i < len; i++) {
		ch = peek();

		if (!isHexCharacter(ch)) {
			error("Invalid escape sequence", positionTree_.currentPosition(), positionTree_.deltaPosition().index);
			return INVALID_CHAR;
		}

		hex += ch;
		advance();
	}

	// Convert hex to codepoint
	auto codepoint = std::stoul(hex, nullptr, 16);
	return static_cast<char>(codepoint);
}

char Lexer::readOctalEscape(char first) {
	Str octal(1, first);

	for (int i = 0; i < 2; i++) {
		char ch = peek();
		if (ch < '0' || ch > '7') {
			break;
		}

		octal += ch;
		advance();
	}

	// Convert octal to codepoint
	unsigned long code = std::stoul(octal, nullptr, 8);
	if (code > 255) {
		error("Invalid octal escape sequence", positionTree_.currentPosition(), positionTree_.deltaPosition().index);
		return INVALID_CHAR;
	}

	return static_cast<char>(code);
}

void Lexer::skip() {
	while (!isAtEnd() && isWhitespace(peek())) {
		advance();
	}
}

char Lexer::peek(size_t offset) {
	size_t targetIdx = position_.index + offset;
	return targetIdx >= source_.size() ? INVALID_CHAR : source_[targetIdx];
}

bool Lexer::isWhitespace(const char& c) {
	switch (c) {
		case ' ':
		case '\t':
		case '\n':
		case '\r':
		case '\v':
		case '\f':
		case '\b':
		case '\a':
			return true;

		default:
			return false;
	}
}

bool Lexer::isHexCharacter(const char& c) {
	return (c >= '0' && c <= '9') ||
		(c >= 'a' && c <= 'f') ||
		(c >= 'A' && c <= 'F');
}

bool Lexer::isIdentifierCharacter(const char& c) {
	if (isalnum(c)) {
		return true;
	}

	switch (c) {
		case '_':
		case '@':
			return true;

		default:
			return false;
	}
}

bool Lexer::isOperatorOrPunctuation(const char& c) {
	switch (c) {
		// Arithmetic operators
		case '+':      // +, +=, ++
		case '-':      // -, -=, --
		case '*':      // *, *=
		case '/':      // /, /=
		case '%':      // %

		// Assignment and comparison operators
		case '=':      // =, ==, =>
		case '<':      // <, <=
		case '>':      // >, >=
		case '!':      // !, !=

		// Logical and bitwise operators
		case '&':      // &, &&
		case '|':      // |, ||
		case '~':      // ~
		case '^':      // ^

		// Punctuation
		case ':':      // :, ::
		case ';':      // ;
		case ',':      // ,
		case '.':      // .
		case '?':      // ?

		// Brackets and parentheses
		case '(':      // (
		case ')':      // )
		case '{':      // {
		case '}':      // }
		case '[':      // [
		case ']':      // ]

		case '$':
			return true;

		default:
			return false;
	}
}

bool Lexer::isCharOrStringCharacter(const char& c) {
	return c == '\'' || c == '"';
}

bool Lexer::isLanguageBlockType(const TokenType& type) {
	switch (type) {
		case TokenType::BLOCK_CSHARP:
		case TokenType::BLOCK_CPP:
		case TokenType::BLOCK_DART:
		case TokenType::BLOCK_JS:
			return true;

		default:
			return false;
	}
}

MRK_NS_END
