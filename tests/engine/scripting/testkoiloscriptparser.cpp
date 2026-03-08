// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testkoiloscriptparser.cpp
 * @brief Implementation of KoiloScriptParser unit tests.
 */

#include "testkoiloscriptparser.hpp"

using namespace koilo;
using namespace koilo::scripting;

// ========== Constructor Tests ==========

void TestKoiloScriptParser::TestDefaultConstructor() {
    std::vector<Token> tokens;
    tokens.push_back(Token(TokenType::END_OF_FILE, "", 1, 1));
    KoiloScriptParser parser(tokens);
    TEST_ASSERT_FALSE(parser.HasError());
}

void TestKoiloScriptParser::TestParameterizedConstructor() {
    KoiloScriptLexer lexer("var x = 42;");
    auto tokens = lexer.Tokenize();
    KoiloScriptParser parser(tokens);
    ScriptAST ast;
    bool ok = parser.Parse(ast);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FALSE(parser.HasError());
    TEST_ASSERT_TRUE(ast.initStatements.size() > 0);
}

// ========== Method Tests ==========

void TestKoiloScriptParser::TestParse() {
    KoiloScriptLexer lexer("fn foo() { }");
    auto tokens = lexer.Tokenize();
    KoiloScriptParser parser(tokens);
    ScriptAST ast;
    bool ok = parser.Parse(ast);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FALSE(parser.HasError());
    TEST_ASSERT_TRUE(ast.functions.size() > 0);
}

void TestKoiloScriptParser::TestHasError() {
    KoiloScriptLexer lexer("var x = 42;");
    auto tokens = lexer.Tokenize();
    KoiloScriptParser parser(tokens);
    ScriptAST ast;
    parser.Parse(ast);
    TEST_ASSERT_FALSE(parser.HasError());
}

void TestKoiloScriptParser::TestGetError() {
    KoiloScriptLexer lexer("var = ;");
    auto tokens = lexer.Tokenize();
    KoiloScriptParser parser(tokens);
    ScriptAST ast;
    bool ok = parser.Parse(ast);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(parser.HasError());
    TEST_ASSERT_NOT_NULL(parser.GetError());
    TEST_ASSERT_TRUE(strlen(parser.GetError()) > 0);
}

void TestKoiloScriptParser::TestGetErrorLine() {
    KoiloScriptLexer lexer("var = ;");
    auto tokens = lexer.Tokenize();
    KoiloScriptParser parser(tokens);
    ScriptAST ast;
    parser.Parse(ast);
    TEST_ASSERT_TRUE(parser.HasError());
    TEST_ASSERT_TRUE(parser.GetErrorLine() >= 1);
}

void TestKoiloScriptParser::TestGetErrorColumn() {
    KoiloScriptLexer lexer("var = ;");
    auto tokens = lexer.Tokenize();
    KoiloScriptParser parser(tokens);
    ScriptAST ast;
    parser.Parse(ast);
    TEST_ASSERT_TRUE(parser.HasError());
    TEST_ASSERT_TRUE(parser.GetErrorColumn() >= 1);
}

// ========== Edge Cases ==========

void TestKoiloScriptParser::TestEdgeCases() {
    // Empty token stream (just EOF)
    std::vector<Token> emptyTokens;
    emptyTokens.push_back(Token(TokenType::END_OF_FILE, "", 1, 1));
    KoiloScriptParser emptyParser(emptyTokens);
    ScriptAST emptyAst;
    bool ok = emptyParser.Parse(emptyAst);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FALSE(emptyParser.HasError());
}

// ========== Test Runner ==========

void TestKoiloScriptParser::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestParse);
    RUN_TEST(TestHasError);
    RUN_TEST(TestGetError);
    RUN_TEST(TestGetErrorLine);
    RUN_TEST(TestGetErrorColumn);
    RUN_TEST(TestEdgeCases);
}
