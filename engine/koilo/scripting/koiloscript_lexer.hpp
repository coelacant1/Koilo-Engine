// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <vector>
#include "../registry/reflect_macros.hpp"

namespace koilo {
namespace scripting {

enum class TokenType {
    NUMBER, STRING, IDENTIFIER,
    DISPLAY, ASSETS, SCENE, STATES, UPDATE, FUNCTION, FUNCTIONS,
    MODEL, TEXTURE, AUDIO, OBJECT, CAMERA, STATE, MATERIAL,
    CONTROLS, KEY, ANIMATIONS, TRANSITION, AUTO, ARROW,
    IF, ELSE, ELSEIF, RETURN, SET_STATE, CALL,
    VAR, WHILE, FOR, IN, AND, OR, NOT,
    TRUE_LITERAL, FALSE_LITERAL, NULL_LITERAL,
    CLASS, NEW, SELF, IMPORT,
    SIGNAL, EMIT, YIELD, BREAK, CONTINUE,
    QUESTION, SEMICOLON,
    EQUALS, PLUS, MINUS, MULTIPLY, DIVIDE, MODULO, DOT, COMMA, COLON,
    EQUAL_EQUAL, NOT_EQUAL, LESS, GREATER, LESS_EQUAL, GREATER_EQUAL,
    LBRACE, RBRACE, LBRACKET, RBRACKET, LPAREN, RPAREN,
    NEWLINE, COMMENT, END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token(TokenType t, const std::string& v, int l, int c)
        : type(t), value(v), line(l), column(c) {}
    Token() : type(TokenType::END_OF_FILE), value(""), line(0), column(0) {}

    KL_BEGIN_FIELDS(Token)
        KL_FIELD(Token, type, "Type", 0, 0),
        KL_FIELD(Token, value, "Value", 0, 0),
        KL_FIELD(Token, line, "Line", -2147483648, 2147483647),
        KL_FIELD(Token, column, "Column", -2147483648, 2147483647)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Token)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Token)
        KL_CTOR(Token, TokenType, const std::string &, int, int),
        KL_CTOR0(Token)
    KL_END_DESCRIBE(Token)

};

class KoiloScriptLexer {
public:
    explicit KoiloScriptLexer(const std::string& source);
    std::vector<Token> Tokenize();
    bool HasError() const { return hasError; }
    const char* GetError() const { return errorMessage.c_str(); }
    static const char* GetTokenTypeName(TokenType type);
    
private:
    const std::string source;
    size_t pos;
    int line, column;
    bool hasError;
    std::string errorMessage;
    
    Token NextToken();
    Token ReadNumber();
    Token ReadString();
    Token ReadIdentifier();
    Token ReadOperator();
    char Peek() const;
    char PeekNext() const;
    char Advance();
    bool IsAtEnd() const;
    bool IsDigit(char c) const;
    bool IsAlpha(char c) const;
    bool IsAlphaNumeric(char c) const;
    void SkipWhitespace();
    void SkipComment();
    void SetError(const std::string& message);
    TokenType CheckKeyword(const std::string& identifier);

    KL_BEGIN_FIELDS(KoiloScriptLexer)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(KoiloScriptLexer)
        KL_METHOD_AUTO(KoiloScriptLexer, Tokenize, "Tokenize"),
        KL_METHOD_AUTO(KoiloScriptLexer, HasError, "Has error"),
        KL_METHOD_AUTO(KoiloScriptLexer, GetError, "Get error"),
        KL_SMETHOD_AUTO(KoiloScriptLexer::GetTokenTypeName, "Get token type name")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KoiloScriptLexer)
        KL_CTOR(KoiloScriptLexer, const std::string &)
    KL_END_DESCRIBE(KoiloScriptLexer)

};

} // namespace scripting
} // namespace koilo
