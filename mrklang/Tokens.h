/*
 * Copyright (c) 2020, Mohamed Ammar <mamar452@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <vector>
#include <string>

namespace MRK
{
	enum TokenKind
	{
		TOKEN_KIND_NONE,
		TOKEN_KIND_WORD,
		TOKEN_KIND_NUMBER,
		TOKEN_KIND_SYMBOL
	};

	enum TokenContextualKind
	{
		TOKEN_CONTEXTUAL_KIND_NONE,
		TOKEN_CONTEXTUAL_KIND_SHORT,
		TOKEN_CONTEXTUAL_KIND_USHORT,
		TOKEN_CONTEXTUAL_KIND_INT,
		TOKEN_CONTEXTUAL_KIND_UINT,
		TOKEN_CONTEXTUAL_KIND_LONG,
		TOKEN_CONTEXTUAL_KIND_ULONG,
		TOKEN_CONTEXTUAL_KIND_STRING,
		TOKEN_CONTEXTUAL_KIND_IDENTIFIER,
		TOKEN_CONTEXTUAL_KIND_CHAR
	};

	struct Token
	{
		TokenKind Kind;
		TokenContextualKind ContextualKind;

		union
		{
			short ShortValue;
			unsigned short UShortValue;
			int IntValue;
			unsigned int UIntValue;
			long LongValue;
			unsigned long ULongValue;
			char *IdentifierValue;
			char *StringValue;
			char CharValue;
		} Value;

		bool HasError; //temp
	};

	class Tokens
	{
	private:
		enum TokenizerState
		{
			TOKENIZER_STATE_NONE,
			TOKENIZER_STATE_WORD,
			TOKENIZER_STATE_NUMBER,
			TOKENIZER_STATE_SYMBOL,
			TOKENIZER_STATE_STRING
		};

		static TokenizerState* ms_Target;

		static void AssignNumber(Token& token, TokenizerState* state = 0);
		static void AssignWord(Token& token, TokenizerState* state = 0);
		static void AssignShort(Token& token, short num);
		static void AssignUShort(Token& token, unsigned short num);
		static void AssignInt(Token& token, int num);
		static void AssignUInt(Token& token, unsigned int num);
		static void AssignLong(Token& token, long num);
		static void AssignULong(Token& token, unsigned long num);
		static void AssignIdentifier(Token& token, _STD string val);
		static void AssignString(Token& token, _STD string val);
		static void AssignChar(Token& token, char val);
		static bool TestUInt(_STD string test, unsigned int* val);
		static bool TestULong(_STD string test, unsigned long* val);
		static bool TestLong(_STD string test, long* val);
		static bool TestInt(_STD string test, int* val);
		static bool IsSkippableCharacter(char character, bool inclSp);
		static void ResetState();
		static void SetExecutor(TokenizerState* state);

	public:
		static _STD vector<Token> Collect(_STD string& text, bool inclSp);
		static _STD string ToValueString(Token& token);
	};
}