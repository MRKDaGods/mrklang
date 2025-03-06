#include "token_lookup.h"

MRK_NS_BEGIN

const KeywordMap& TokenLookup::keywords() {
	static const auto map = createKeywordMap();
	return map;
}

const OperatorMap& TokenLookup::operators() {
	static const auto map = createOperatorMap();
	return map;
}

KeywordMap TokenLookup::createKeywordMap() {
	#define MAKE_KW(name, literal) { TokenType::##name, #literal }

	return {
		MAKE_KW(BLOCK_CSHARP, __cs),
		MAKE_KW(BLOCK_CPP, __cpp),
		MAKE_KW(BLOCK_DART, __dart),
        MAKE_KW(BLOCK_JS, __js),

		MAKE_KW(KW_FUNC, func),
		MAKE_KW(KW_CLASS, class),
        MAKE_KW(KW_STRUCT, struct),
        MAKE_KW(KW_ENUM, enum),
		MAKE_KW(KW_INTERFACE, interface),
        MAKE_KW(KW_VAR, var),

		MAKE_KW(KW_IF, if),
		MAKE_KW(KW_ELSE, else),
		MAKE_KW(KW_FOR, for),
        MAKE_KW(KW_FOREACH, foreach),
        MAKE_KW(KW_WHILE, while),
		MAKE_KW(KW_RETURN, return),
		MAKE_KW(KW_NEW, new),
        MAKE_KW(KW_DELETE, delete),
        MAKE_KW(KW_IN, in),
        MAKE_KW(KW_AS, as),
        MAKE_KW(KW_PARAMS, params),
        MAKE_KW(KW_NAMESPACE, namespace),
        MAKE_KW(KW_DECLSPEC, __declspec),
        MAKE_KW(KW_USE, use),
        MAKE_KW(KW_FROM, from),
        MAKE_KW(KW_GLOBAL, __global),

		MAKE_KW(KW_PUBLIC, public),
		MAKE_KW(KW_PROTECTED, protected),
		MAKE_KW(KW_PRIVATE, private),
		MAKE_KW(KW_INTERNAL, internal),
		MAKE_KW(KW_STATIC, static),
		MAKE_KW(KW_ABSTRACT, abstract),
		MAKE_KW(KW_SEALED, sealed),
		MAKE_KW(KW_VIRTUAL, virtual),
		MAKE_KW(KW_OVERRIDE, override),
		MAKE_KW(KW_CONST, const),
		MAKE_KW(KW_READONLY, readonly),
		MAKE_KW(KW_EXTERN, extern),
		MAKE_KW(KW_IMPLICIT, implicit),
		MAKE_KW(KW_EXPLICIT, explicit),
		MAKE_KW(KW_ASYNC, async),

        MAKE_KW(LIT_NULL, null)
	};

	#undef MAKE_KW
}

OperatorMap TokenLookup::createOperatorMap() {
	#define MAKE_OP(name, literal) { TokenType::##name, literal }
	
	return {
        // Arithmetic Operators
        MAKE_OP(OP_PLUS, "+"),
        MAKE_OP(OP_MINUS, "-"),
        MAKE_OP(OP_ASTERISK, "*"),
        MAKE_OP(OP_SLASH, "/"),
        MAKE_OP(OP_MOD, "%"),
        MAKE_OP(OP_INCREMENT, "++"),
        MAKE_OP(OP_DECREMENT, "--"),

        // Assignment Operators
        MAKE_OP(OP_EQ, "="),
        MAKE_OP(OP_PLUS_EQ, "+="),
        MAKE_OP(OP_MINUS_EQ, "-="),
        MAKE_OP(OP_MULT_EQ, "*="),
        MAKE_OP(OP_DIV_EQ, "/="),

        // Comparison Operators
        MAKE_OP(OP_EQ_EQ, "=="),
        MAKE_OP(OP_NOT_EQ, "!="),
        MAKE_OP(OP_LT, "<"),
        MAKE_OP(OP_GT, ">"),
        MAKE_OP(OP_LE, "<="),
        MAKE_OP(OP_GE, ">="),

        // Logical Operators
        MAKE_OP(OP_AND, "&&"),
        MAKE_OP(OP_OR, "||"),
        MAKE_OP(OP_NOT, "!"),

        // Bitwise Operators
        MAKE_OP(OP_BAND, "&"),
        MAKE_OP(OP_BOR, "|"),
        MAKE_OP(OP_BNOT, "~"),
        MAKE_OP(OP_BXOR, "^"),

        // Special Operators
        MAKE_OP(OP_DOUBLE_COLON, "::"),
        MAKE_OP(OP_ARROW, "->"),
        MAKE_OP(OP_FAT_ARROW, "=>"),
        MAKE_OP(OP_DOT, "."),
        MAKE_OP(OP_QUESTION, "?"),

        // Punctuation
        MAKE_OP(SEMICOLON, ";"),
        MAKE_OP(COMMA, ","),
        MAKE_OP(COLON, ":"),
        MAKE_OP(LPAREN, "("),
        MAKE_OP(RPAREN, ")"),
        MAKE_OP(LBRACE, "{"),
        MAKE_OP(RBRACE, "}"),
        MAKE_OP(LBRACKET, "["),
        MAKE_OP(RBRACKET, "]"),

        // Comments
        MAKE_OP(COMMENT_SINGLE, "//"),
        MAKE_OP(COMMENT_MULTI_START, "/*"),
        MAKE_OP(COMMENT_MULTI_END, "*/"),

        MAKE_OP(INTERPOLATION, "$")
	};

    #undef MAKE_OP
}

MRK_NS_END