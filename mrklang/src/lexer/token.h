#pragma once

#include "common/types.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <format>

MRK_NS_BEGIN

/**
 * @struct LexerPosition
 * @brief Represents the position of a token in the source code.
 */
	struct LexerPosition {
	uint32_t index;
	uint32_t line;
	uint32_t column;

	LexerPosition operator-(const LexerPosition& other) {
		return LexerPosition{ index - other.index, line - other.line, column - other.column };
	}

	Str toString() {
		return std::format("LexerPosition(index={}, line={}, column={})", index, line, column);
	}
};

/**
 * @enum TokenType
 * @brief Represents the different types of tokens that can be identified by the lexer.
 */
enum class TokenType : uint16_t {
	#define TOKEN(x) x,
	#include "token_defs.inc"
	#undef TOKEN

	COUNT
};

/**
  * @brief Converts a TokenType to its corresponding string representation.
  */
constexpr std::string_view toString(const TokenType& type) {
	switch (type) {
		#define TOKEN(x) case TokenType::x: return #x;
		#include "token_defs.inc"
		#undef TOKEN

		default:
			return "UNKNOWN";
	}
}

/**
 * @struct Token
 * @brief Represents a token identified by the lexer.
 */
struct Token {
	TokenType type;
	Str lexeme;
	LexerPosition position;

	Token(TokenType type, Str lexeme, LexerPosition position)
		: type(type), lexeme(std::move(lexeme)), position(position) {}

	Token() : Token(TokenType::END_OF_FILE, "", { 1u, 1u, 1u }) {}

	bool isAccessModifier() {
		switch (type) {
			case TokenType::KW_PUBLIC:
			case TokenType::KW_PROTECTED:
			case TokenType::KW_PRIVATE:
			case TokenType::KW_INTERNAL:
			case TokenType::KW_STATIC:
			case TokenType::KW_ABSTRACT:
			case TokenType::KW_SEALED:
			case TokenType::KW_VIRTUAL:
			case TokenType::KW_OVERRIDE:
			case TokenType::KW_CONST:
			case TokenType::KW_READONLY:
			case TokenType::KW_EXTERN:
			case TokenType::KW_IMPLICIT:
			case TokenType::KW_EXPLICIT:
			case TokenType::KW_NEW:
			case TokenType::KW_ASYNC:
				return true;

			default:
				return false;
		}
	}
};

MRK_NS_END