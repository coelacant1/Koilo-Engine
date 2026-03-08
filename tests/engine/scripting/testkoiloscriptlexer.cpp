// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testkoiloscriptlexer.cpp
 * @brief Implementation of KoiloScriptLexer unit tests.
 */

#include "testkoiloscriptlexer.hpp"

using namespace koilo;
using namespace koilo::scripting;

// ========== Constructor Tests ==========

void TestKoiloScriptLexer::TestDefaultConstructor() {
    KoiloScriptLexer lexer("");
    auto tokens = lexer.Tokenize();
    TEST_ASSERT_FALSE(lexer.HasError());
    TEST_ASSERT_TRUE(tokens.size() >= 1);
    TEST_ASSERT_TRUE(tokens.back().type == TokenType::END_OF_FILE);
}

void TestKoiloScriptLexer::TestParameterizedConstructor() {
    KoiloScriptLexer lexer("42");
    auto tokens = lexer.Tokenize();
    TEST_ASSERT_FALSE(lexer.HasError());
    TEST_ASSERT_TRUE(tokens.size() >= 2);
    TEST_ASSERT_TRUE(tokens[0].type == TokenType::NUMBER);
    TEST_ASSERT_EQUAL_STRING("42", tokens[0].value.c_str());
}

// ========== Method Tests ==========

void TestKoiloScriptLexer::TestTokenize() {
    KoiloScriptLexer lexer("var x = 42;");
    auto tokens = lexer.Tokenize();
    TEST_ASSERT_FALSE(lexer.HasError());
    TEST_ASSERT_TRUE(tokens.size() >= 5);
    TEST_ASSERT_TRUE(tokens[0].type == TokenType::VAR);
    TEST_ASSERT_TRUE(tokens[1].type == TokenType::IDENTIFIER);
    TEST_ASSERT_EQUAL_STRING("x", tokens[1].value.c_str());
    TEST_ASSERT_TRUE(tokens[2].type == TokenType::EQUALS);
    TEST_ASSERT_TRUE(tokens[3].type == TokenType::NUMBER);
    TEST_ASSERT_EQUAL_STRING("42", tokens[3].value.c_str());
    TEST_ASSERT_TRUE(tokens[4].type == TokenType::SEMICOLON);
}

void TestKoiloScriptLexer::TestHasError() {
    KoiloScriptLexer lexer("var x = 42;");
    lexer.Tokenize();
    TEST_ASSERT_FALSE(lexer.HasError());
}

void TestKoiloScriptLexer::TestGetError() {
    KoiloScriptLexer lexer("var x = 42;");
    lexer.Tokenize();
    TEST_ASSERT_FALSE(lexer.HasError());
    TEST_ASSERT_NOT_NULL(lexer.GetError());
}

// ========== Edge Cases ==========

void TestKoiloScriptLexer::TestEdgeCases() {
    // Empty input
    KoiloScriptLexer empty("");
    auto t1 = empty.Tokenize();
    TEST_ASSERT_FALSE(empty.HasError());

    // String token
    KoiloScriptLexer strLexer("\"hello\"");
    auto t2 = strLexer.Tokenize();
    TEST_ASSERT_FALSE(strLexer.HasError());
    TEST_ASSERT_TRUE(t2[0].type == TokenType::STRING);
    TEST_ASSERT_EQUAL_STRING("hello", t2[0].value.c_str());

    // Keywords
    KoiloScriptLexer kwLexer("if while var function");
    auto t3 = kwLexer.Tokenize();
    TEST_ASSERT_FALSE(kwLexer.HasError());
    TEST_ASSERT_TRUE(t3[0].type == TokenType::IF);
    TEST_ASSERT_TRUE(t3[1].type == TokenType::WHILE);
    TEST_ASSERT_TRUE(t3[2].type == TokenType::VAR);
    TEST_ASSERT_TRUE(t3[3].type == TokenType::FUNCTION);

    // Operators
    KoiloScriptLexer opLexer("+ - * / < > ==");
    auto t4 = opLexer.Tokenize();
    TEST_ASSERT_FALSE(opLexer.HasError());
    TEST_ASSERT_TRUE(t4[0].type == TokenType::PLUS);
    TEST_ASSERT_TRUE(t4[1].type == TokenType::MINUS);
    TEST_ASSERT_TRUE(t4[2].type == TokenType::MULTIPLY);
    TEST_ASSERT_TRUE(t4[3].type == TokenType::DIVIDE);
    TEST_ASSERT_TRUE(t4[4].type == TokenType::LESS);
    TEST_ASSERT_TRUE(t4[5].type == TokenType::GREATER);
    TEST_ASSERT_TRUE(t4[6].type == TokenType::EQUAL_EQUAL);

    // Identifier
    KoiloScriptLexer idLexer("myVar");
    auto t5 = idLexer.Tokenize();
    TEST_ASSERT_FALSE(idLexer.HasError());
    TEST_ASSERT_TRUE(t5[0].type == TokenType::IDENTIFIER);
    TEST_ASSERT_EQUAL_STRING("myVar", t5[0].value.c_str());
}

// ========== Test Runner ==========

void TestKoiloScriptLexer::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestTokenize);
    RUN_TEST(TestHasError);
    RUN_TEST(TestGetError);
    RUN_TEST(TestEdgeCases);
}
