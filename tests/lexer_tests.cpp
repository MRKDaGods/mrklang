#include "CppUnitTest.h"
#include "lexer/lexer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace MRK_NS;

namespace LexerTests {
    TEST_CLASS(LexerTests) {
public:
    TEST_METHOD(TestEmptyInput) {
        Lexer lexer("");
        auto& tokens = lexer.tokenize();

        Assert::AreEqual(1ull, tokens.size());
        Assert::AreEqual((int)TokenType::END_OF_FILE, (int)tokens[0].type);
    }

    TEST_METHOD(TestIntegerLiterals) {
        Lexer lexer("42 123 0");
        auto& tokens = lexer.tokenize();

        Assert::AreEqual(4ull, tokens.size()); // 3 numbers + EOF
        Assert::AreEqual((int)TokenType::LIT_INT, (int)tokens[0].type);
        Assert::AreEqual("42", tokens[0].lexeme.c_str());
        Assert::AreEqual((int)TokenType::LIT_INT, (int)tokens[1].type);
        Assert::AreEqual("123", tokens[1].lexeme.c_str());
        Assert::AreEqual((int)TokenType::LIT_INT, (int)tokens[2].type);
        Assert::AreEqual("0", tokens[2].lexeme.c_str());
    }

    TEST_METHOD(TestFloatLiterals) {
        Lexer lexer("3.14 0.5 42.0");
        auto& tokens = lexer.tokenize();

        Assert::AreEqual(4ull, tokens.size()); // 3 numbers + EOF
        Assert::AreEqual((int)TokenType::LIT_FLOAT, (int)tokens[0].type);
        Assert::AreEqual("3.14", tokens[0].lexeme.c_str());
        Assert::AreEqual((int)TokenType::LIT_FLOAT, (int)tokens[1].type);
        Assert::AreEqual("0.5", tokens[1].lexeme.c_str());
        Assert::AreEqual((int)TokenType::LIT_FLOAT, (int)tokens[2].type);
        Assert::AreEqual("42.0", tokens[2].lexeme.c_str());
    }

    TEST_METHOD(TestHexLiterals) {
        Lexer lexer("0x1f 0xAB 0x0");
        auto& tokens = lexer.tokenize();

        Assert::AreEqual(4ull, tokens.size()); // 3 numbers + EOF
        Assert::AreEqual((int)TokenType::LIT_HEX, (int)tokens[0].type);
        Assert::AreEqual("0x1f", tokens[0].lexeme.c_str());
        Assert::AreEqual((int)TokenType::LIT_HEX, (int)tokens[1].type);
        Assert::AreEqual("0xab", tokens[1].lexeme.c_str());
        Assert::AreEqual((int)TokenType::LIT_HEX, (int)tokens[2].type);
        Assert::AreEqual("0x0", tokens[2].lexeme.c_str());
    }

    TEST_METHOD(TestIdentifiers) {
        Lexer lexer("foo bar_123 @test $var");
        auto& tokens = lexer.tokenize();

        Assert::AreEqual(5ull, tokens.size()); // 4 identifiers + EOF
        for (int i = 0; i < 4; i++) {
            Assert::AreEqual((int)TokenType::IDENTIFIER, (int)tokens[i].type);
        }
        Assert::AreEqual("foo", tokens[0].lexeme.c_str());
        Assert::AreEqual("bar_123", tokens[1].lexeme.c_str());
        Assert::AreEqual("@test", tokens[2].lexeme.c_str());
        Assert::AreEqual("$var", tokens[3].lexeme.c_str());
    }

    TEST_METHOD(TestBoolLiterals) {
        Lexer lexer("true false");
        auto& tokens = lexer.tokenize();

        Assert::AreEqual(3ull, tokens.size()); // 2 bools + EOF
        Assert::AreEqual((int)TokenType::LIT_BOOL, (int)tokens[0].type);
        Assert::AreEqual("true", tokens[0].lexeme.c_str());
        Assert::AreEqual((int)TokenType::LIT_BOOL, (int)tokens[1].type);
        Assert::AreEqual("false", tokens[1].lexeme.c_str());
    }

    TEST_METHOD(TestOperators) {
        Lexer lexer("+ += ++ - -= -- * *= / /= % == => < <= > >= ! != & && | || ~ ^");
        auto& tokens = lexer.tokenize();

        Assert::IsTrue(tokens.size() > 1);
        // Sample checks for a few operators
        Assert::AreEqual("+", tokens[0].lexeme.c_str());
        Assert::AreEqual("+=", tokens[1].lexeme.c_str());
        Assert::AreEqual("++", tokens[2].lexeme.c_str());
    }

    TEST_METHOD(TestPunctuation) {
        Lexer lexer("( ) { } [ ] ; , . : :: ?");
        auto& tokens = lexer.tokenize();

        Assert::IsTrue(tokens.size() > 1);
        // Check a few punctuation marks
        Assert::AreEqual("(", tokens[0].lexeme.c_str());
        Assert::AreEqual(")", tokens[1].lexeme.c_str());
        Assert::AreEqual("{", tokens[2].lexeme.c_str());
    }

    TEST_METHOD(TestMixedInput) {
        Lexer lexer("let x = 42;\nif (true) { print(3.14); }");
        auto& tokens = lexer.tokenize();

        Assert::IsTrue(tokens.size() > 1);
        // Check first few tokens
        Assert::AreEqual("let", tokens[0].lexeme.c_str());
        Assert::AreEqual("x", tokens[1].lexeme.c_str());
        Assert::AreEqual("=", tokens[2].lexeme.c_str());
        Assert::AreEqual("42", tokens[3].lexeme.c_str());
    }
    };
}
