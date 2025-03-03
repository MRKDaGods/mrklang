#pragma once

#include "common/macros.h"
#include "token.h"

#include <unordered_map>
#include <string>

MRK_NS_BEGIN

using KeywordMap = std::unordered_map<TokenType, std::string>;
using OperatorMap = std::unordered_map<TokenType, std::string>;

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