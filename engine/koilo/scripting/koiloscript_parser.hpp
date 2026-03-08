// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <koilo/scripting/koiloscript_lexer.hpp>
#include <koilo/scripting/koiloscript_ast.hpp>
#include <string>
#include <memory>
#include "../registry/reflect_macros.hpp"

namespace koilo {
namespace scripting {

/**
 * @brief Parser for KoiloScript language
 * 
 * Implements a recursive descent parser that builds an AST from tokens.
 * Parses all KoiloScript blocks: DISPLAY, ASSETS, SCENE, STATES, UPDATE.
 * 
 * Usage:
 *   KoiloScriptParser parser(tokens);
 *   ScriptAST ast;
 *   if (parser.Parse(ast)) {
 *       // Use AST
 *   } else {
 *       printf("Parse error: %s\n", parser.GetError());
 *   }
 */
class KoiloScriptParser {
public:
    /**
     * @brief Construct parser with tokens
     * @param tokens Token stream from lexer
     */
    explicit KoiloScriptParser(const std::vector<Token>& tokens);
    
    /**
     * @brief Parse tokens into AST
     * @param outAST Output AST structure
     * @return true if parsing succeeded, false otherwise
     */
    bool Parse(ScriptAST& outAST);
    
    /**
     * @brief Check if parser encountered an error
     */
    bool HasError() const { return hasError; }
    
    /**
     * @brief Get error message
     */
    const char* GetError() const { return errorMessage.c_str(); }
    
    /**
     * @brief Get error line number
     */
    int GetErrorLine() const { return errorLine; }
    
    /**
     * @brief Get error column number
     */
    int GetErrorColumn() const { return errorColumn; }
    
private:
    const std::vector<Token>& tokens;
    size_t current;
    bool hasError;
    std::string errorMessage;
    int errorLine;
    int errorColumn;
    
    // Token navigation
    const Token& Peek() const;
    const Token& Previous() const;
    const Token& Advance();
    bool IsAtEnd() const;
    bool Check(TokenType type) const;
    bool Match(TokenType type);
    bool Match(const std::vector<TokenType>& types);
    Token Consume(TokenType type, const std::string& errorMsg);
    
    // Error handling
    void SetError(const std::string& message);
    void SetError(const std::string& message, const Token& token);
    
    // Stamp AST node with current source location
    template<typename T>
    T& Stamp(T& node) { 
        const auto& t = Peek();
        node.line = t.line; 
        node.column = t.column; 
        return node; 
    }
    template<typename T>
    std::unique_ptr<T>& Stamp(std::unique_ptr<T>& node) { 
        const auto& t = Peek();
        node->line = t.line; 
        node->column = t.column; 
        return node; 
    }
    
    // Top-level parsing
    std::unique_ptr<DisplayNode> ParseDisplay();
    void ParseAssets(ScriptAST& outAST);
    void ParseScene(ScriptAST& outAST);
    std::vector<std::unique_ptr<StateNode>> ParseStates();
    void ParseControls(ScriptAST& outAST);
    void ParseAnimations(ScriptAST& outAST);
    void ParseFunctions(ScriptAST& outAST);
    std::unique_ptr<UpdateNode> ParseUpdate();
    
    // Block parsing helpers
    std::unique_ptr<ObjectNode> ParseObject();
    std::unique_ptr<CameraNode> ParseCamera();
    std::unique_ptr<MaterialNode> ParseMaterial();
    std::unique_ptr<StateNode> ParseState();
    
    // Statement parsing
    std::unique_ptr<StatementNode> ParseStatement();
    std::unique_ptr<AssignmentNode> ParseAssignment();
    std::unique_ptr<IfStatementNode> ParseIfStatement();
    std::unique_ptr<FunctionCallNode> ParseFunctionCall();
    std::unique_ptr<FunctionCallNode> ParseMethodCallStatement();
    std::unique_ptr<VarDeclNode> ParseVarDecl();
    std::unique_ptr<WhileNode> ParseWhileLoop();
    std::unique_ptr<ForEachNode> ParseForEach();
    std::unique_ptr<FunctionDeclNode> ParseFunctionDecl();
    std::unique_ptr<ReturnNode> ParseReturn();
    std::unique_ptr<ClassDeclNode> ParseClassDecl();
    std::unique_ptr<EmitNode> ParseEmitStatement();
    
    // Expression parsing
    std::unique_ptr<ExpressionNode> ParseExpression();
    std::unique_ptr<ExpressionNode> ParseTernary();
    std::unique_ptr<ExpressionNode> ParseLogicalOr();
    std::unique_ptr<ExpressionNode> ParseLogicalAnd();
    std::unique_ptr<ExpressionNode> ParseComparison();
    std::unique_ptr<ExpressionNode> ParseTerm();
    std::unique_ptr<ExpressionNode> ParseFactor();
    std::unique_ptr<ExpressionNode> ParseUnary();
    std::unique_ptr<ExpressionNode> ParsePostfix();
    std::unique_ptr<ExpressionNode> ParsePrimary();
    std::unique_ptr<ExpressionNode> ParseArrayLiteral();
    std::unique_ptr<ExpressionNode> ParseTableLiteral();
    
    // Helper: flatten an expression tree to a dot-separated path (for backward compat)
    std::string FlattenExpressionToPath(const ExpressionNode* expr);
    
    // Property parsing
    std::map<std::string, std::string> ParseProperties();
    std::map<std::string, std::unique_ptr<ExpressionNode>> ParseExpressionProperties();

    // Note: KoiloScriptParser is not meant to be instantiated from scripts
    // Reflection disabled

    // KoiloScriptParser is not meant to be instantiated from scripts

    // Note: KoiloScriptParser not meant to be instantiated from scripts
    // Constructor reflection disabled

    KL_BEGIN_FIELDS(KoiloScriptParser)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(KoiloScriptParser)
        KL_METHOD_AUTO(KoiloScriptParser, Parse, "Parse"),
        KL_METHOD_AUTO(KoiloScriptParser, HasError, "Has error"),
        KL_METHOD_AUTO(KoiloScriptParser, GetError, "Get error"),
        KL_METHOD_AUTO(KoiloScriptParser, GetErrorLine, "Get error line"),
        KL_METHOD_AUTO(KoiloScriptParser, GetErrorColumn, "Get error column")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KoiloScriptParser)
        /* No constructors exposed to scripts */
    KL_END_DESCRIBE(KoiloScriptParser)

};

} // namespace scripting
} // namespace koilo
