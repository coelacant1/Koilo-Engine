// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testtoken.cpp
 * @brief Implementation of Token unit tests.
 */

#include "testtoken.hpp"

using namespace koilo;
using namespace koilo::scripting;

// ========== Constructor Tests ==========

void TestToken::TestDefaultConstructor() {
    Token obj;
    TEST_ASSERT_TRUE(obj.type == TokenType::END_OF_FILE);
    TEST_ASSERT_EQUAL_STRING("", obj.value.c_str());
    TEST_ASSERT_EQUAL(0, obj.line);
    TEST_ASSERT_EQUAL(0, obj.column);
}

void TestToken::TestParameterizedConstructor() {
    Token tok(TokenType::NUMBER, "42", 3, 7);
    TEST_ASSERT_TRUE(tok.type == TokenType::NUMBER);
    TEST_ASSERT_EQUAL_STRING("42", tok.value.c_str());
    TEST_ASSERT_EQUAL(3, tok.line);
    TEST_ASSERT_EQUAL(7, tok.column);
}

// ========== Edge Cases ==========

void TestToken::TestEdgeCases() {
    // String token
    Token strTok(TokenType::STRING, "hello world", 1, 1);
    TEST_ASSERT_TRUE(strTok.type == TokenType::STRING);
    TEST_ASSERT_EQUAL_STRING("hello world", strTok.value.c_str());

    // Identifier token
    Token idTok(TokenType::IDENTIFIER, "myVar", 10, 20);
    TEST_ASSERT_TRUE(idTok.type == TokenType::IDENTIFIER);
    TEST_ASSERT_EQUAL(10, idTok.line);
    TEST_ASSERT_EQUAL(20, idTok.column);

    // Empty value
    Token emptyTok(TokenType::SEMICOLON, "", 0, 0);
    TEST_ASSERT_EQUAL_STRING("", emptyTok.value.c_str());
}

// ========== Test Runner ==========

void TestToken::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
