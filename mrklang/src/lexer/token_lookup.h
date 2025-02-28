#pragma once

#include "common/macros.h"
#include "token.h"

#include <unordered_map>
#include <string>

MRK_NS_BEGIN

using KeywordMap = MRK_STD unordered_map<TokenType, MRK_STD string>;
using OperatorMap = MRK_STD unordered_map<TokenType, MRK_STD string>;

class TokenLookup {
public:
	/**
	  * @brief Retrieves the map of keywords.
	  * 
	  * @returns A constant reference to a map that associates
	  * token types with their corresponding keyword strings.
	  */
	static const KeywordMap& keywords();

	/**
	  * @brief Retrieves the map of operators.
	  * 
	  * @returns A constant reference to a map that associates
	  * token types with their corresponding operator strings.
	  */
	static const OperatorMap& operators();

	// Singleton
	TokenLookup(const TokenLookup&) = delete;
	TokenLookup& operator=(const TokenLookup&) = delete;

private:
	static KeywordMap createKeywordMap();
	static OperatorMap createOperatorMap();
};

MRK_NS_END