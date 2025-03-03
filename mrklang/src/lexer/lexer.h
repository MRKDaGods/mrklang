#pragma once

#include "common/types.h"
#include "token.h"
#include "lexer_position_tree.h"

#include <vector>
#include <string>
#include <stack>

MRK_NS_BEGIN

struct LexerError {
	Str message;
	LexerPosition getPosition;
	uint32_t length;
};

/// A lexical analyzer that converts a source string into a sequence of tokens.
class Lexer {
public:
	/// Constructs a Lexer with the given source string.
	/// @param source The source string to be tokenized.
	Lexer(const Str& getSource, uint32_t maxErrors = 10u);

	/// Tokenizes the source string.
	/// @return A vector of tokens extracted from the source string.
	const Vec<Token>& tokenize();

    /// Returns the list of errors encountered during lexing.
    const Vec<LexerError>& getErrors() const;

    /// Returns the current position of the lexer in the source string.
    const LexerPosition& getPosition() const;

	/// Returns the tokens extracted from the source string.
	const Vec<Token>& getTokens() const;

	/// Returns the source string being tokenized.
	const Str& getSource() const;

	/// Reports all errors encountered during lexing.
	void reportErrors() const;

private:
	/// The source string to be tokenized.
	Str source_;

	/// The current lexer position
	LexerPosition position_;

	/// The vector of tokens extracted from the source string.
	Vec<Token> tokens_;

	/// The maximum number of errors allowed before the lexer stops processing.
	const uint32_t maxErrors_;

	/// The errors encountered during lexing.
	Vec<LexerError> errors_;

	/// Keeps track of the start positions of tokens in the current tree.
	LexerPositionTree positionTree_;

	/// Adds a token to the list of tokens.
	/// @param type The type of the token to add.
	/// @param lexeme The lexeme (text) of the token to add.
	void addToken(TokenType type, Str& lexeme, const LexerPosition& getPosition);

	/// Adds a token to the list of tokens.
	/// @param type The type of the token to add.
	void addToken(TokenType type, const LexerPosition& getPosition);

    /// Records an error encountered during lexing.
    /// @param message The error message describing the issue.
    /// @param position The position in the source string where the error occurred.
    /// @param length The length of the erroneous text. Defaults to 1.
    void error(const Str& message, const LexerPosition& getPosition, uint32_t length = 1);

	/// Checks if the lexer has reached the end of the source string.
	bool isAtEnd();

	/// Advances the current position in the source string by a given increment.
	/// @param increment The number of characters to advance. Defaults to 1.
	char advance(const size_t increment = 1);

	/// Advances the current position in the source string by the length of a given string.
	/// @param str The string whose length determines the number of characters to advance.
	char advance(const Str& str);

	/// Reads a number literal at the current position from the source string.
	void readNumberLiteral();

	/// Reads an identifier or keyword at the current position from the source string.
	void readIdentifierOrKeyword();

	/// Reads a language-specific block of code.
	void readLanguageBlock();

	/// Reads an operator or punctuation at the current position from the source string.
	void readOperatorOrPunctuation();

	/// Reads a comment from the source string.
	/// @param commentType The type of comment (single-line or multi-line).
	void readComment(TokenType commentType);

    /// Reads a character or string literal at the current position from the source string.
    void readCharOrStringLiteral();

    /// Reads an escape sequence at the current position from the source string.
    char readEscapeSequence();

    /// Reads a codepoint escape sequence of a given length at the current position from the source string.
    char readCodepointEscape(int len);

    /// Reads an octal escape sequence starting with a given character at the current position from the source string.
    char readOctalEscape(char first);

	/// Skips characters until a valid character is found.
	void skip();

	/// Peeks at the character at a given offset from the current position.
	char peek(size_t offset = 0);

	/// Checks if a character is a whitespace character.
	bool isWhitespace(const char& c);

	/// Checks if a character is a valid hexadecimal character.
	bool isHexCharacter(const char& c);

	/// Checks if a character is a valid identifier character.
	bool isIdentifierCharacter(const char& c);

	/// Checks if a character is an operator or punctuation.
	bool isOperatorOrPunctuation(const char& c);

    /// Checks if a character is a valid character for a character or string literal.
    bool isCharOrStringCharacter(const char& c);

	/// Checks if a token type is a language block type.
	bool isLanguageBlockType(const TokenType& type);
};

MRK_NS_END