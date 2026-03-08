// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/scripting/koiloscript_lexer.hpp>
#include <sstream>

namespace koilo {
namespace scripting {

KoiloScriptLexer::KoiloScriptLexer(const std::string& source)
    : source(source), pos(0), line(1), column(1), hasError(false), errorMessage("") {}

std::vector<Token> KoiloScriptLexer::Tokenize() {
    std::vector<Token> tokens;
    while (!IsAtEnd()) {
        Token token = NextToken();
        if (token.type != TokenType::COMMENT && token.type != TokenType::NEWLINE) {
            tokens.push_back(token);
        }
        if (HasError()) break;
    }
    tokens.push_back(Token(TokenType::END_OF_FILE, "", line, column));
    return tokens;
}

Token KoiloScriptLexer::NextToken() {
    SkipWhitespace();
    if (IsAtEnd()) return Token(TokenType::END_OF_FILE, "", line, column);
    
    int tokenLine = line, tokenColumn = column;
    char c = Peek();
    
    if (c == '#') { SkipComment(); return Token(TokenType::COMMENT, "", tokenLine, tokenColumn); }
    if (c == '/' && PeekNext() == '/') { SkipComment(); return Token(TokenType::COMMENT, "", tokenLine, tokenColumn); }
    if (c == '\n') { Advance(); return Token(TokenType::NEWLINE, "\\n", tokenLine, tokenColumn); }
    if (IsDigit(c) || (c == '-' && IsDigit(PeekNext()))) return ReadNumber();
    if (c == '"') return ReadString();
    if (IsAlpha(c) || c == '_') return ReadIdentifier();
    return ReadOperator();
}

Token KoiloScriptLexer::ReadNumber() {
    int tokenLine = line, tokenColumn = column;
    std::string value;
    if (Peek() == '-') value += Advance();
    while (IsDigit(Peek())) value += Advance();
    if (Peek() == '.' && IsDigit(PeekNext())) {
        value += Advance();
        while (IsDigit(Peek())) value += Advance();
    }
    return Token(TokenType::NUMBER, value, tokenLine, tokenColumn);
}

Token KoiloScriptLexer::ReadString() {
    int tokenLine = line, tokenColumn = column;
    std::string value;
    Advance(); // consume "
    while (!IsAtEnd() && Peek() != '"') {
        if (Peek() == '\n') {
            SetError("Unterminated string literal");
            return Token(TokenType::END_OF_FILE, "", tokenLine, tokenColumn);
        }
        if (Peek() == '\\') {
            Advance();
            if (!IsAtEnd()) {
                char escaped = Advance();
                switch (escaped) {
                    case 'n': value += '\n'; break;
                    case 't': value += '\t'; break;
                    case 'r': value += '\r'; break;
                    case '\\': value += '\\'; break;
                    case '"': value += '"'; break;
                    default: value += escaped; break;
                }
            }
        } else {
            value += Advance();
        }
    }
    if (IsAtEnd()) {
        SetError("Unterminated string literal");
        return Token(TokenType::END_OF_FILE, "", tokenLine, tokenColumn);
    }
    Advance(); // consume closing "
    return Token(TokenType::STRING, value, tokenLine, tokenColumn);
}

Token KoiloScriptLexer::ReadIdentifier() {
    int tokenLine = line, tokenColumn = column;
    std::string value;
    while (IsAlphaNumeric(Peek()) || Peek() == '_') value += Advance();
    TokenType type = CheckKeyword(value);
    return Token(type, value, tokenLine, tokenColumn);
}

Token KoiloScriptLexer::ReadOperator() {
    int tokenLine = line, tokenColumn = column;
    char c = Advance();
    switch (c) {
        case '=': return (Peek() == '=') ? (Advance(), Token(TokenType::EQUAL_EQUAL, "==", tokenLine, tokenColumn)) : Token(TokenType::EQUALS, "=", tokenLine, tokenColumn);
        case '!': if (Peek() == '=') { Advance(); return Token(TokenType::NOT_EQUAL, "!=", tokenLine, tokenColumn); } break;
        case '<': return (Peek() == '=') ? (Advance(), Token(TokenType::LESS_EQUAL, "<=", tokenLine, tokenColumn)) : Token(TokenType::LESS, "<", tokenLine, tokenColumn);
        case '>': return (Peek() == '=') ? (Advance(), Token(TokenType::GREATER_EQUAL, ">=", tokenLine, tokenColumn)) : Token(TokenType::GREATER, ">", tokenLine, tokenColumn);
        case '+': return Token(TokenType::PLUS, "+", tokenLine, tokenColumn);
        case '-': return (Peek() == '>') ? (Advance(), Token(TokenType::ARROW, "->", tokenLine, tokenColumn)) : Token(TokenType::MINUS, "-", tokenLine, tokenColumn);
        case '*': return Token(TokenType::MULTIPLY, "*", tokenLine, tokenColumn);
        case '/': return Token(TokenType::DIVIDE, "/", tokenLine, tokenColumn);
        case '%': return Token(TokenType::MODULO, "%", tokenLine, tokenColumn);
        case '.': return Token(TokenType::DOT, ".", tokenLine, tokenColumn);
        case ',': return Token(TokenType::COMMA, ",", tokenLine, tokenColumn);
        case ':': return Token(TokenType::COLON, ":", tokenLine, tokenColumn);
        case '?': return Token(TokenType::QUESTION, "?", tokenLine, tokenColumn);
        case ';': return Token(TokenType::SEMICOLON, ";", tokenLine, tokenColumn);
        case '{': return Token(TokenType::LBRACE, "{", tokenLine, tokenColumn);
        case '}': return Token(TokenType::RBRACE, "}", tokenLine, tokenColumn);
        case '[': return Token(TokenType::LBRACKET, "[", tokenLine, tokenColumn);
        case ']': return Token(TokenType::RBRACKET, "]", tokenLine, tokenColumn);
        case '(': return Token(TokenType::LPAREN, "(", tokenLine, tokenColumn);
        case ')': return Token(TokenType::RPAREN, ")", tokenLine, tokenColumn);
    }
    std::stringstream ss; ss << "Unexpected character: '" << c << "'";
    SetError(ss.str());
    return Token(TokenType::END_OF_FILE, "", tokenLine, tokenColumn);
}

TokenType KoiloScriptLexer::CheckKeyword(const std::string& id) {
    if (id == "and") return TokenType::AND;
    if (id == "ASSETS") return TokenType::ASSETS;
    if (id == "AUDIO") return TokenType::AUDIO;
    if (id == "ANIMATIONS") return TokenType::ANIMATIONS;
    if (id == "AUTO") return TokenType::AUTO;
    if (id == "auto") return TokenType::AUTO;
    if (id == "break") return TokenType::BREAK;
    if (id == "CAMERA") return TokenType::CAMERA;
    if (id == "camera") return TokenType::CAMERA;
    if (id == "class") return TokenType::CLASS;
    if (id == "continue") return TokenType::CONTINUE;
    if (id == "CONTROLS") return TokenType::CONTROLS;
    if (id == "DISPLAY") return TokenType::DISPLAY;
    if (id == "false") return TokenType::FALSE_LITERAL;
    if (id == "fn") return TokenType::FUNCTION;
    if (id == "for") return TokenType::FOR;
    if (id == "FUNCTION") return TokenType::FUNCTION;
    if (id == "function") return TokenType::FUNCTION;
    if (id == "FUNCTIONS") return TokenType::FUNCTIONS;
    if (id == "in") return TokenType::IN;
    if (id == "KEY") return TokenType::KEY;
    if (id == "MATERIAL") return TokenType::MATERIAL;
    if (id == "material") return TokenType::MATERIAL;
    if (id == "MODEL") return TokenType::MODEL;
    if (id == "not") return TokenType::NOT;
    if (id == "new") return TokenType::NEW;
    if (id == "null") return TokenType::NULL_LITERAL;
    if (id == "OBJECT") return TokenType::OBJECT;
    if (id == "object") return TokenType::OBJECT;
    if (id == "or") return TokenType::OR;
    if (id == "SCENE") return TokenType::SCENE;
    if (id == "self") return TokenType::SELF;
    if (id == "STATE") return TokenType::STATE;
    if (id == "state") return TokenType::STATE;
    if (id == "STATES") return TokenType::STATES;
    if (id == "TEXTURE") return TokenType::TEXTURE;
    if (id == "TRANSITION") return TokenType::TRANSITION;
    if (id == "transition") return TokenType::TRANSITION;
    if (id == "true") return TokenType::TRUE_LITERAL;
    if (id == "UPDATE") return TokenType::UPDATE;
    if (id == "var") return TokenType::VAR;
    if (id == "while") return TokenType::WHILE;
    if (id == "if") return TokenType::IF;
    if (id == "import") return TokenType::IMPORT;
    if (id == "signal") return TokenType::SIGNAL;
    if (id == "emit") return TokenType::EMIT;
    if (id == "yield") return TokenType::YIELD;
    if (id == "else") return TokenType::ELSE;
    if (id == "elseif") return TokenType::ELSEIF;
    if (id == "return") return TokenType::RETURN;
    if (id == "set_state") return TokenType::SET_STATE;
    if (id == "call") return TokenType::CALL;
    return TokenType::IDENTIFIER;
}

char KoiloScriptLexer::Peek() const { return IsAtEnd() ? '\0' : source[pos]; }
char KoiloScriptLexer::PeekNext() const { return (pos + 1 >= source.length()) ? '\0' : source[pos + 1]; }
char KoiloScriptLexer::Advance() {
    if (IsAtEnd()) return '\0';
    char c = source[pos++];
    if (c == '\n') { line++; column = 1; } else { column++; }
    return c;
}
bool KoiloScriptLexer::IsAtEnd() const { return pos >= source.length(); }
bool KoiloScriptLexer::IsDigit(char c) const { return c >= '0' && c <= '9'; }
bool KoiloScriptLexer::IsAlpha(char c) const { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
bool KoiloScriptLexer::IsAlphaNumeric(char c) const { return IsAlpha(c) || IsDigit(c); }
void KoiloScriptLexer::SkipWhitespace() {
    while (!IsAtEnd() && (Peek() == ' ' || Peek() == '\t' || Peek() == '\r' || Peek() == '\n')) Advance();
}
void KoiloScriptLexer::SkipComment() { while (!IsAtEnd() && Peek() != '\n') Advance(); }
void KoiloScriptLexer::SetError(const std::string& message) {
    hasError = true;
    std::stringstream ss;
    ss << "Lexer error at line " << line << ", column " << column << ": " << message;
    errorMessage = ss.str();
}

const char* KoiloScriptLexer::GetTokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::STRING: return "STRING";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::DISPLAY: return "DISPLAY";
        case TokenType::ASSETS: return "ASSETS";
        case TokenType::SCENE: return "SCENE";
        case TokenType::STATES: return "STATES";
        case TokenType::UPDATE: return "UPDATE";
        case TokenType::FUNCTION: return "FUNCTION";
        case TokenType::FUNCTIONS: return "FUNCTIONS";
        case TokenType::MODEL: return "MODEL";
        case TokenType::TEXTURE: return "TEXTURE";
        case TokenType::AUDIO: return "AUDIO";
        case TokenType::OBJECT: return "OBJECT";
        case TokenType::CAMERA: return "CAMERA";
        case TokenType::STATE: return "STATE";
        case TokenType::MATERIAL: return "MATERIAL";
        case TokenType::CONTROLS: return "CONTROLS";
        case TokenType::KEY: return "KEY";
        case TokenType::ANIMATIONS: return "ANIMATIONS";
        case TokenType::TRANSITION: return "TRANSITION";
        case TokenType::AUTO: return "AUTO";
        case TokenType::ARROW: return "ARROW";
        case TokenType::IF: return "IF";
        case TokenType::ELSE: return "ELSE";
        case TokenType::ELSEIF: return "ELSEIF";
        case TokenType::RETURN: return "RETURN";
        case TokenType::SET_STATE: return "SET_STATE";
        case TokenType::CALL: return "CALL";
        case TokenType::VAR: return "VAR";
        case TokenType::WHILE: return "WHILE";
        case TokenType::FOR: return "FOR";
        case TokenType::IN: return "IN";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";
        case TokenType::TRUE_LITERAL: return "TRUE";
        case TokenType::FALSE_LITERAL: return "FALSE";
        case TokenType::NULL_LITERAL: return "NULL";
        case TokenType::QUESTION: return "QUESTION";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::EQUALS: return "EQUALS";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MULTIPLY: return "MULTIPLY";
        case TokenType::DIVIDE: return "DIVIDE";
        case TokenType::MODULO: return "MODULO";
        case TokenType::DOT: return "DOT";
        case TokenType::COMMA: return "COMMA";
        case TokenType::COLON: return "COLON";
        case TokenType::EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TokenType::NOT_EQUAL: return "NOT_EQUAL";
        case TokenType::LESS: return "LESS";
        case TokenType::GREATER: return "GREATER";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::COMMENT: return "COMMENT";
        case TokenType::END_OF_FILE: return "END_OF_FILE";
        default: return "UNKNOWN";
    }
}

} // namespace scripting
} // namespace koilo
