// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/scripting/koiloscript_parser.hpp>
#include <sstream>

namespace koilo {
namespace scripting {

KoiloScriptParser::KoiloScriptParser(const std::vector<Token>& tokens)
    : tokens(tokens), current(0), hasError(false), errorMessage(""), errorLine(0), errorColumn(0) {}

bool KoiloScriptParser::Parse(ScriptAST& outAST) {
    while (!IsAtEnd()) {
        const Token& token = Peek();
        
        // Top-level function declarations
        if (token.type == TokenType::FUNCTION) {
            outAST.functions.push_back(ParseFunctionDecl());
        }
        // Top-level class declarations
        else if (token.type == TokenType::CLASS) {
            outAST.initStatements.push_back(ParseClassDecl());
        }
        // Import declarations
        else if (token.type == TokenType::IMPORT) {
            Advance(); // consume 'import'
            if (Peek().type != TokenType::STRING) {
                SetError("Expected string path after 'import'", Peek());
                return false;
            }
            outAST.imports.push_back(Peek().value);
            Advance(); // consume path string
            if (Peek().type == TokenType::SEMICOLON) Advance(); // optional semicolon
        }
        // Signal declarations: signal name(params);
        else if (token.type == TokenType::SIGNAL) {
            Advance(); // consume 'signal'
            if (Peek().type != TokenType::IDENTIFIER) {
                SetError("Expected signal name after 'signal'", Peek());
                return false;
            }
            outAST.signals.push_back(Peek().value);
            auto node = std::make_unique<SignalDeclNode>();
            node->name = Peek().value;
            Advance(); // consume name
            // Optional parameter list (for documentation only)
            if (Peek().type == TokenType::LPAREN) {
                Advance(); // consume '('
                while (Peek().type != TokenType::RPAREN && !IsAtEnd()) {
                    if (Peek().type == TokenType::IDENTIFIER) {
                        node->params.push_back(Peek().value);
                        Advance();
                    }
                    if (Peek().type == TokenType::COMMA) Advance();
                }
                if (Peek().type == TokenType::RPAREN) Advance(); // consume ')'
            }
            if (Peek().type == TokenType::SEMICOLON) Advance();
            outAST.initStatements.push_back(std::move(node));
        }
        // Emit statements at top level
        else if (token.type == TokenType::EMIT) {
            outAST.initStatements.push_back(ParseEmitStatement());
        }
        // Top-level statements: var, assignments, method calls, control flow
        else if (token.type == TokenType::VAR ||
                 token.type == TokenType::IDENTIFIER ||
                 token.type == TokenType::IF ||
                 token.type == TokenType::WHILE ||
                 token.type == TokenType::FOR) {
            outAST.initStatements.push_back(ParseStatement());
        }
        else {
            SetError("Unexpected token at top level", token);
            return false;
        }
        
        if (HasError()) {
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// Token Navigation
// ============================================================================

const Token& KoiloScriptParser::Peek() const {
    return tokens[current];
}

const Token& KoiloScriptParser::Previous() const {
    return tokens[current - 1];
}

const Token& KoiloScriptParser::Advance() {
    if (!IsAtEnd()) current++;
    return Previous();
}

bool KoiloScriptParser::IsAtEnd() const {
    return Peek().type == TokenType::END_OF_FILE;
}

bool KoiloScriptParser::Check(TokenType type) const {
    if (IsAtEnd()) return false;
    return Peek().type == type;
}

bool KoiloScriptParser::Match(TokenType type) {
    if (Check(type)) {
        Advance();
        return true;
    }
    return false;
}

bool KoiloScriptParser::Match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (Check(type)) {
            Advance();
            return true;
        }
    }
    return false;
}

Token KoiloScriptParser::Consume(TokenType type, const std::string& errorMsg) {
    if (Check(type)) return Advance();
    SetError(errorMsg, Peek());
    return Peek();
}

void KoiloScriptParser::SetError(const std::string& message) {
    hasError = true;
    errorMessage = message;
    if (!IsAtEnd()) {
        errorLine = Peek().line;
        errorColumn = Peek().column;
    }
}

void KoiloScriptParser::SetError(const std::string& message, const Token& token) {
    hasError = true;
    std::stringstream ss;
    ss << "Parse error at line " << token.line << ", column " << token.column << ": " << message;
    errorMessage = ss.str();
    errorLine = token.line;
    errorColumn = token.column;
}

// ============================================================================
// Top-Level Block Parsing
// ============================================================================

std::unique_ptr<DisplayNode> KoiloScriptParser::ParseDisplay() {
    auto node = std::make_unique<DisplayNode>();
    
    Consume(TokenType::DISPLAY, "Expected DISPLAY keyword");
    Consume(TokenType::LBRACE, "Expected '{' after DISPLAY");
    
    node->properties = ParseProperties();
    
    Consume(TokenType::RBRACE, "Expected '}' after DISPLAY properties");
    
    return node;
}

void KoiloScriptParser::ParseAssets(ScriptAST& outAST) {
    Consume(TokenType::ASSETS, "Expected ASSETS keyword");
    Consume(TokenType::LBRACE, "Expected '{' after ASSETS");
    
    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        // Check for material declaration
        if (Check(TokenType::MATERIAL)) {
            outAST.materials.push_back(ParseMaterial());
            if (HasError()) break;
            continue;
        }
        
        auto asset = std::make_unique<AssetNode>();
        
        // Parse asset type (MODEL, TEXTURE, AUDIO keywords or lowercase)
        if (Check(TokenType::MODEL)) {
            Advance();
            asset->type = AssetNode::AssetType::MODEL;
        } else if (Check(TokenType::TEXTURE)) {
            Advance();
            asset->type = AssetNode::AssetType::TEXTURE;
        } else if (Check(TokenType::AUDIO)) {
            Advance();
            asset->type = AssetNode::AssetType::AUDIO;
        } else if (Check(TokenType::IDENTIFIER)) {
            const std::string& typeStr = Peek().value;
            if (typeStr == "model") {
                asset->type = AssetNode::AssetType::MODEL;
            } else if (typeStr == "texture") {
                asset->type = AssetNode::AssetType::TEXTURE;
            } else if (typeStr == "audio") {
                asset->type = AssetNode::AssetType::AUDIO;
            } else {
                SetError("Expected asset type (model, texture, audio, or material)", Peek());
                break;
            }
            Advance();
        } else {
            SetError("Expected asset type (model, texture, audio, or material)", Peek());
            break;
        }
        
        // Parse asset name
        Token name = Consume(TokenType::IDENTIFIER, "Expected asset name");
        asset->name = name.value;
        
        Consume(TokenType::EQUALS, "Expected '=' after asset name");
        
        // Parse asset filepath
        Token filepath = Consume(TokenType::STRING, "Expected asset filepath");
        asset->filepath = filepath.value;
        
        outAST.assets.push_back(std::move(asset));
        
        if (HasError()) break;
    }
    
    Consume(TokenType::RBRACE, "Expected '}' after ASSETS");
}

void KoiloScriptParser::ParseScene(ScriptAST& outAST) {
    Consume(TokenType::SCENE, "Expected SCENE keyword");
    Consume(TokenType::LBRACE, "Expected '{' after SCENE");
    
    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        if (Check(TokenType::OBJECT)) {
            outAST.objects.push_back(ParseObject());
        } else if (Check(TokenType::CAMERA)) {
            outAST.cameras.push_back(ParseCamera());
        } else if (!Check(TokenType::RBRACE)) {
            SetError("Expected 'object' or 'camera' in SCENE", Peek());
            break;
        }
        
        if (HasError()) break;
    }
    
    Consume(TokenType::RBRACE, "Expected '}' after SCENE");
}

std::unique_ptr<ObjectNode> KoiloScriptParser::ParseObject() {
    auto node = std::make_unique<ObjectNode>();
    
    Consume(TokenType::OBJECT, "Expected 'object' keyword");
    
    Token id = Consume(TokenType::IDENTIFIER, "Expected object ID");
    node->id = id.value;
    
    // Optional: ": ClassName" for reflection-based instantiation
    if (Match(TokenType::COLON)) {
        Token className = Consume(TokenType::IDENTIFIER, "Expected class name after ':'");
        node->className = className.value;
    }
    
    Consume(TokenType::LBRACE, "Expected '{' after object ID");
    
    node->properties = ParseExpressionProperties();
    
    Consume(TokenType::RBRACE, "Expected '}' after object properties");
    
    return node;
}

std::unique_ptr<CameraNode> KoiloScriptParser::ParseCamera() {
    auto node = std::make_unique<CameraNode>();
    
    Consume(TokenType::CAMERA, "Expected 'camera' keyword");
    
    Token id = Consume(TokenType::IDENTIFIER, "Expected camera ID");
    node->id = id.value;
    
    Consume(TokenType::LBRACE, "Expected '{' after camera ID");
    
    node->properties = ParseExpressionProperties();
    
    Consume(TokenType::RBRACE, "Expected '}' after camera properties");
    
    return node;
}

std::unique_ptr<MaterialNode> KoiloScriptParser::ParseMaterial() {
    auto node = std::make_unique<MaterialNode>();
    
    Consume(TokenType::MATERIAL, "Expected 'material' keyword");
    
    Token name = Consume(TokenType::IDENTIFIER, "Expected material name");
    node->name = name.value;
    
    Consume(TokenType::COLON, "Expected ':' after material name");
    
    Token className = Consume(TokenType::IDENTIFIER, "Expected class name after ':'");
    node->className = className.value;
    
    Consume(TokenType::LBRACE, "Expected '{' after material class name");
    
    node->properties = ParseExpressionProperties();
    
    Consume(TokenType::RBRACE, "Expected '}' after material properties");
    
    return node;
}

std::vector<std::unique_ptr<StateNode>> KoiloScriptParser::ParseStates() {
    std::vector<std::unique_ptr<StateNode>> states;
    
    Consume(TokenType::STATES, "Expected STATES keyword");
    Consume(TokenType::LBRACE, "Expected '{' after STATES");
    
    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        states.push_back(ParseState());
        if (HasError()) break;
    }
    
    Consume(TokenType::RBRACE, "Expected '}' after STATES");
    
    return states;
}

std::unique_ptr<StateNode> KoiloScriptParser::ParseState() {
    auto node = std::make_unique<StateNode>();
    
    Consume(TokenType::STATE, "Expected STATE keyword");
    
    Token name = Consume(TokenType::IDENTIFIER, "Expected state name");
    node->name = name.value;
    
    Consume(TokenType::LBRACE, "Expected '{' after state name");
    
    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        // Distinguish property (key: value) from statement (object.method())
        if (Check(TokenType::IDENTIFIER)) {
            // Look ahead: if next token after identifier is COLON, it's a property
            size_t savedPos = current;
            Advance(); // consume identifier
            bool isProperty = Check(TokenType::COLON);
            current = savedPos; // restore
            
            if (isProperty) {
                Token key = Consume(TokenType::IDENTIFIER, "Expected property name");
                Consume(TokenType::COLON, "Expected ':' after property name");
                node->properties[key.value] = ParseExpression();
                if (HasError()) break;
                continue;
            }
        }
        
        node->statements.push_back(ParseStatement());
        if (HasError()) break;
    }
    
    Consume(TokenType::RBRACE, "Expected '}' after state body");
    
    return node;
}

void KoiloScriptParser::ParseControls(ScriptAST& outAST) {
    Consume(TokenType::CONTROLS, "Expected CONTROLS keyword");
    Consume(TokenType::LBRACE, "Expected '{' after CONTROLS");
    
    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        auto ctrl = std::make_unique<ControlNode>();
        
        // Parse input type (key, pin)
        if (Check(TokenType::KEY) || (Check(TokenType::IDENTIFIER) && Peek().value == "key")) {
            ctrl->inputType = "key";
            Advance();
        } else if (Check(TokenType::IDENTIFIER) && Peek().value == "pin") {
            ctrl->inputType = "pin";
            Advance();
        } else {
            SetError("Expected 'key' or 'pin' in CONTROLS", Peek());
            break;
        }
        
        // Parse input identifier (number or letter)
        if (Check(TokenType::NUMBER) || Check(TokenType::IDENTIFIER)) {
            ctrl->inputId = Advance().value;
        } else {
            SetError("Expected key/pin identifier", Peek());
            break;
        }
        
        // Parse arrow ->
        Consume(TokenType::ARROW, "Expected '->' after key identifier");
        
        // Parse action: functionName(args) or just functionName()
        if (Check(TokenType::IDENTIFIER) || Check(TokenType::SET_STATE)) {
            ctrl->action = Advance().value;
        } else {
            SetError("Expected action name after '->'", Peek());
            break;
        }
        
        // Parse optional arguments
        if (Match(TokenType::LPAREN)) {
            while (!Check(TokenType::RPAREN) && !IsAtEnd()) {
                if (Check(TokenType::STRING)) {
                    ctrl->actionArgs.push_back(Advance().value);
                } else if (Check(TokenType::NUMBER)) {
                    ctrl->actionArgs.push_back(Advance().value);
                } else if (Check(TokenType::IDENTIFIER)) {
                    ctrl->actionArgs.push_back(Advance().value);
                } else {
                    break;
                }
                if (!Check(TokenType::RPAREN)) {
                    Consume(TokenType::COMMA, "Expected ',' between arguments");
                }
            }
            Consume(TokenType::RPAREN, "Expected ')' after arguments");
        }
        
        outAST.controls.push_back(std::move(ctrl));
        if (HasError()) break;
    }
    
    Consume(TokenType::RBRACE, "Expected '}' after CONTROLS");
}

void KoiloScriptParser::ParseAnimations(ScriptAST& outAST) {
    Consume(TokenType::ANIMATIONS, "Expected ANIMATIONS keyword");
    Consume(TokenType::LBRACE, "Expected '{' after ANIMATIONS");
    
    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        if (Check(TokenType::TRANSITION)) {
            auto node = std::make_unique<TransitionNode>();
            Advance(); // consume 'transition'
            
            Token target = Consume(TokenType::IDENTIFIER, "Expected transition target");
            node->target = target.value;
            
            Consume(TokenType::LBRACE, "Expected '{' after transition target");
            node->properties = ParseExpressionProperties();
            Consume(TokenType::RBRACE, "Expected '}' after transition properties");
            
            outAST.transitions.push_back(std::move(node));
        } else if (Check(TokenType::AUTO)) {
            auto node = std::make_unique<AutoAnimNode>();
            Advance(); // consume 'auto'
            
            Token name = Consume(TokenType::IDENTIFIER, "Expected auto-animation name");
            node->name = name.value;
            
            Consume(TokenType::LBRACE, "Expected '{' after auto-animation name");
            node->properties = ParseExpressionProperties();
            Consume(TokenType::RBRACE, "Expected '}' after auto-animation properties");
            
            outAST.autoAnims.push_back(std::move(node));
        } else {
            SetError("Expected 'transition' or 'auto' in ANIMATIONS", Peek());
            break;
        }
        
        if (HasError()) break;
    }
    
    Consume(TokenType::RBRACE, "Expected '}' after ANIMATIONS");
}

std::unique_ptr<UpdateNode> KoiloScriptParser::ParseUpdate() {
    auto node = std::make_unique<UpdateNode>();
    
    Consume(TokenType::UPDATE, "Expected UPDATE keyword");
    Consume(TokenType::LBRACE, "Expected '{' after UPDATE");
    
    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        node->statements.push_back(ParseStatement());
        if (HasError()) break;
    }
    
    Consume(TokenType::RBRACE, "Expected '}' after UPDATE statements");
    
    return node;
}

// ============================================================================
// Statement Parsing
// ============================================================================

std::unique_ptr<StatementNode> KoiloScriptParser::ParseStatement() {
    // Variable declaration (requires semicolon)
    if (Check(TokenType::VAR)) {
        auto node = ParseVarDecl();
        Consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
        return node;
    }
    
    // While loop (no semicolon - block statement)
    if (Check(TokenType::WHILE)) {
        return ParseWhileLoop();
    }
    
    // For-each loop (no semicolon - block statement)
    if (Check(TokenType::FOR)) {
        return ParseForEach();
    }
    
    // Function declaration (no semicolon - block statement)
    if (Check(TokenType::FUNCTION)) {
        return ParseFunctionDecl();
    }
    
    // Class declaration (no semicolon - block statement)
    if (Check(TokenType::CLASS)) {
        return ParseClassDecl();
    }
    
    // Return statement (requires semicolon)
    if (Check(TokenType::RETURN)) {
        auto node = ParseReturn();
        Consume(TokenType::SEMICOLON, "Expected ';' after return statement");
        return node;
    }
    
    // If statement (no semicolon - block statement)
    if (Check(TokenType::IF)) {
        return ParseIfStatement();
    }
    
    // Emit statement (requires semicolon)
    if (Check(TokenType::EMIT)) {
        auto node = ParseEmitStatement();
        return node;
    }
    
    // Yield statement
    if (Check(TokenType::YIELD)) {
        Advance(); // consume 'yield'
        Consume(TokenType::SEMICOLON, "Expected ';' after yield");
        return std::make_unique<YieldNode>();
    }
    
    // Break statement
    if (Check(TokenType::BREAK)) {
        auto node = std::make_unique<BreakNode>();
        node->line = Peek().line;
        Advance();
        Consume(TokenType::SEMICOLON, "Expected ';' after break");
        return node;
    }
    
    // Continue statement
    if (Check(TokenType::CONTINUE)) {
        auto node = std::make_unique<ContinueNode>();
        node->line = Peek().line;
        Advance();
        Consume(TokenType::SEMICOLON, "Expected ';' after continue");
        return node;
    }
    
    // Function call (set_state, call, etc.)
    if (Check(TokenType::SET_STATE) || Check(TokenType::CALL)) {
        auto node = ParseFunctionCall();
        Consume(TokenType::SEMICOLON, "Expected ';' after function call");
        return node;
    }
    
    // Check for method call or assignment (identifier or self)
    if (Check(TokenType::IDENTIFIER) || Check(TokenType::SELF)) {
        // Look ahead to determine type: assignment or expression statement
        size_t savedPos = current;
        
        // Skip through any postfix chain: ident(.ident | (args) | [expr])*
        Advance();  // initial identifier or self
        while (true) {
            if (Check(TokenType::DOT)) {
                Advance(); // '.'
                if (Check(TokenType::IDENTIFIER)) Advance(); // ident after dot
            } else if (Check(TokenType::LPAREN)) {
                // Skip balanced parens
                Advance(); // '('
                int depth = 1;
                while (depth > 0 && !IsAtEnd()) {
                    if (Check(TokenType::LPAREN)) depth++;
                    if (Check(TokenType::RPAREN)) depth--;
                    Advance();
                }
            } else if (Check(TokenType::LBRACKET)) {
                // Skip balanced brackets
                Advance(); // '['
                int depth = 1;
                while (depth > 0 && !IsAtEnd()) {
                    if (Check(TokenType::LBRACKET)) depth++;
                    if (Check(TokenType::RBRACKET)) depth--;
                    Advance();
                }
            } else {
                break;
            }
        }
        
        bool isAssignment = Check(TokenType::EQUALS);
        current = savedPos;
        
        if (isAssignment) {
            auto node = ParseAssignment();
            Consume(TokenType::SEMICOLON, "Expected ';' after assignment");
            return node;
        } else {
            // Expression statement (method call, chained call, etc.)
            auto expr = ParseExpression();
            auto node = std::make_unique<FunctionCallNode>();
            Stamp(node);
            node->functionName = FlattenExpressionToPath(expr.get());
            node->expression = std::move(expr);
            
            Consume(TokenType::SEMICOLON, "Expected ';' after expression statement");
            return node;
        }
    }
    
    SetError("Expected statement", Peek());
    return nullptr;
}

std::unique_ptr<AssignmentNode> KoiloScriptParser::ParseAssignment() {
    auto node = std::make_unique<AssignmentNode>();
    Stamp(node);
    std::string target;
    if (Check(TokenType::SELF)) {
        target = Advance().value;
    } else {
        target = Consume(TokenType::IDENTIFIER, "Expected identifier").value;
    }
    
    // Check for bracket-index access
    if (Match(TokenType::LBRACKET)) {
        node->indexExpr = ParseExpression();
        Consume(TokenType::RBRACKET, "Expected ']' after index");
    }
    
    while (Match(TokenType::DOT)) {
        target += ".";
        target += Consume(TokenType::IDENTIFIER, "Expected identifier after '.'").value;
    }
    
    node->target = target;
    
    Consume(TokenType::EQUALS, "Expected '=' in assignment");
    
    node->value = ParseExpression();
    
    return node;
}

std::unique_ptr<IfStatementNode> KoiloScriptParser::ParseIfStatement() {
    auto node = std::make_unique<IfStatementNode>();
    Stamp(node);
    
    Consume(TokenType::IF, "Expected 'if'");
    
    // Support both: if (condition) { and if condition {
    bool hasParens = Match(TokenType::LPAREN);
    
    node->condition = ParseExpression();
    
    if (hasParens) {
        Consume(TokenType::RPAREN, "Expected ')' after condition");
    }
    Consume(TokenType::LBRACE, "Expected '{' after if condition");
    
    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        node->thenBlock.push_back(ParseStatement());
        if (HasError()) break;
    }
    
    Consume(TokenType::RBRACE, "Expected '}' after if block");
    
    // Else / else if block
    if (Match(TokenType::ELSE)) {
        if (Check(TokenType::IF)) {
            // else if -> parse as nested if in the else block
            node->elseBlock.push_back(ParseIfStatement());
        } else {
            Consume(TokenType::LBRACE, "Expected '{' after 'else'");
            
            while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
                node->elseBlock.push_back(ParseStatement());
                if (HasError()) break;
            }
            
            Consume(TokenType::RBRACE, "Expected '}' after else block");
        }
    }
    
    return node;
}

std::unique_ptr<FunctionCallNode> KoiloScriptParser::ParseFunctionCall() {
    auto node = std::make_unique<FunctionCallNode>();
    Stamp(node);
    Token funcToken = Advance();
    node->functionName = funcToken.value;
    
    // Arguments
    if (Match(TokenType::LPAREN)) {
        if (!Check(TokenType::RPAREN)) {
            do {
                node->arguments.push_back(ParseExpression());
            } while (Match(TokenType::COMMA));
        }
        Consume(TokenType::RPAREN, "Expected ')' after arguments");
    } else {
        // Single argument without parentheses (e.g., set_state happy)
        node->arguments.push_back(ParseExpression());
    }
    
    return node;
}

std::unique_ptr<FunctionCallNode> KoiloScriptParser::ParseMethodCallStatement() {
    auto node = std::make_unique<FunctionCallNode>();
    Stamp(node);
    std::string path;
    path = Consume(TokenType::IDENTIFIER, "Expected identifier").value;
    
    while (Match(TokenType::DOT)) {
        path += ".";
        path += Consume(TokenType::IDENTIFIER, "Expected identifier after '.'").value;
    }
    
    node->functionName = path;
    
    // Arguments
    Consume(TokenType::LPAREN, "Expected '(' for method call");
    
    if (!Check(TokenType::RPAREN)) {
        do {
            node->arguments.push_back(ParseExpression());
        } while (Match(TokenType::COMMA));
    }
    
    Consume(TokenType::RPAREN, "Expected ')' after arguments");
    
    return node;
}

std::unique_ptr<VarDeclNode> KoiloScriptParser::ParseVarDecl() {
    auto node = std::make_unique<VarDeclNode>();
    Stamp(node);
    
    Consume(TokenType::VAR, "Expected 'var'");
    Token name = Consume(TokenType::IDENTIFIER, "Expected variable name");
    node->name = name.value;
    
    if (Match(TokenType::EQUALS)) {
        node->initializer = ParseExpression();
    }
    
    return node;
}

std::unique_ptr<WhileNode> KoiloScriptParser::ParseWhileLoop() {
    auto node = std::make_unique<WhileNode>();
    Stamp(node);
    
    Consume(TokenType::WHILE, "Expected 'while'");
    
    // Condition - parentheses optional
    bool hasParen = Match(TokenType::LPAREN);
    node->condition = ParseExpression();
    if (hasParen) {
        Consume(TokenType::RPAREN, "Expected ')' after while condition");
    }
    
    Consume(TokenType::LBRACE, "Expected '{' after while condition");
    
    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        node->body.push_back(ParseStatement());
        if (HasError()) break;
    }
    
    Consume(TokenType::RBRACE, "Expected '}' after while body");
    
    return node;
}

std::unique_ptr<ForEachNode> KoiloScriptParser::ParseForEach() {
    auto node = std::make_unique<ForEachNode>();
    Stamp(node);
    Advance(); // consume 'for' keyword
    Token varName = Consume(TokenType::IDENTIFIER, "Expected variable name");
    node->varName = varName.value;
    
    Consume(TokenType::IN, "Expected 'in' after for variable");
    
    node->collection = ParseExpression();
    
    Consume(TokenType::LBRACE, "Expected '{' after for-in expression");
    
    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        node->body.push_back(ParseStatement());
        if (HasError()) break;
    }
    
    Consume(TokenType::RBRACE, "Expected '}' after for body");
    
    return node;
}

std::unique_ptr<FunctionDeclNode> KoiloScriptParser::ParseFunctionDecl() {
    auto node = std::make_unique<FunctionDeclNode>();
    Stamp(node);
    
    Consume(TokenType::FUNCTION, "Expected 'function'");
    Token name = Consume(TokenType::IDENTIFIER, "Expected function name");
    node->name = name.value;
    
    Consume(TokenType::LPAREN, "Expected '(' after function name");
    
    if (!Check(TokenType::RPAREN)) {
        do {
            Token param = Consume(TokenType::IDENTIFIER, "Expected parameter name");
            node->params.push_back(param.value);
        } while (Match(TokenType::COMMA));
    }
    
    Consume(TokenType::RPAREN, "Expected ')' after parameters");
    Consume(TokenType::LBRACE, "Expected '{' after function declaration");
    
    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        node->body.push_back(ParseStatement());
        if (HasError()) break;
    }
    
    Consume(TokenType::RBRACE, "Expected '}' after function body");
    
    return node;
}

std::unique_ptr<ReturnNode> KoiloScriptParser::ParseReturn() {
    auto node = std::make_unique<ReturnNode>();
    Stamp(node);
    Advance(); // consume 'return' keyword
    
    // Return value is optional - check if next token starts an expression
    if (!Check(TokenType::RBRACE) && !Check(TokenType::END_OF_FILE) && !Check(TokenType::SEMICOLON)) {
        node->value = ParseExpression();
    }
    
    return node;
}

std::unique_ptr<ClassDeclNode> KoiloScriptParser::ParseClassDecl() {
    auto node = std::make_unique<ClassDeclNode>();
    Stamp(node);
    Advance(); // consume 'class'
    
    node->name = Consume(TokenType::IDENTIFIER, "Expected class name").value;
    Consume(TokenType::LBRACE, "Expected '{' after class name");
    
    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        if (HasError()) break;
        
        // Field declaration: var name = default;
        if (Check(TokenType::VAR)) {
            Advance(); // consume 'var'
            ClassFieldDecl field;
            field.name = Consume(TokenType::IDENTIFIER, "Expected field name").value;
            if (Match(TokenType::EQUALS)) {
                field.defaultValue = ParseExpression();
            }
            Consume(TokenType::SEMICOLON, "Expected ';' after field declaration");
            node->fields.push_back(std::move(field));
        }
        // Method declaration: fn name(params) { ... }
        else if (Check(TokenType::FUNCTION)) {
            node->methods.push_back(ParseFunctionDecl());
        }
        else {
            SetError("Expected 'var' or 'fn' inside class body", Peek());
            break;
        }
    }
    
    Consume(TokenType::RBRACE, "Expected '}' after class body");
    return node;
}

void KoiloScriptParser::ParseFunctions(ScriptAST& outAST) {
    Consume(TokenType::FUNCTIONS, "Expected FUNCTIONS keyword");
    Consume(TokenType::LBRACE, "Expected '{' after FUNCTIONS");
    
    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        outAST.functions.push_back(ParseFunctionDecl());
        if (HasError()) break;
    }
    
    Consume(TokenType::RBRACE, "Expected '}' after FUNCTIONS");
}

std::unique_ptr<ExpressionNode> KoiloScriptParser::ParseExpression() {
    return ParseTernary();
}

std::unique_ptr<ExpressionNode> KoiloScriptParser::ParseTernary() {
    auto expr = ParseLogicalOr();
    
    if (Match(TokenType::QUESTION)) {
        auto ternary = std::make_unique<ExpressionNode>();
        ternary->type = ExpressionNode::Type::TERNARY;
        ternary->condition = std::move(expr);
        ternary->left = ParseExpression();
        Consume(TokenType::COLON, "Expected ':' in ternary expression");
        ternary->right = ParseExpression();
        return ternary;
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> KoiloScriptParser::ParseLogicalOr() {
    auto expr = ParseLogicalAnd();
    
    while (Match(TokenType::OR)) {
        auto binary = std::make_unique<ExpressionNode>();
        binary->type = ExpressionNode::Type::BINARY_OP;
        binary->op = "or";
        binary->left = std::move(expr);
        binary->right = ParseLogicalAnd();
        expr = std::move(binary);
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> KoiloScriptParser::ParseLogicalAnd() {
    auto expr = ParseComparison();
    
    while (Match(TokenType::AND)) {
        auto binary = std::make_unique<ExpressionNode>();
        binary->type = ExpressionNode::Type::BINARY_OP;
        binary->op = "and";
        binary->left = std::move(expr);
        binary->right = ParseComparison();
        expr = std::move(binary);
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> KoiloScriptParser::ParseComparison() {
    auto expr = ParseTerm();
    
    while (Match({TokenType::EQUAL_EQUAL, TokenType::NOT_EQUAL, 
                  TokenType::LESS, TokenType::GREATER,
                  TokenType::LESS_EQUAL, TokenType::GREATER_EQUAL})) {
        Token op = Previous();
        auto right = ParseTerm();
        
        auto binary = std::make_unique<ExpressionNode>();
        binary->type = ExpressionNode::Type::BINARY_OP;
        binary->op = op.value;
        binary->left = std::move(expr);
        binary->right = std::move(right);
        expr = std::move(binary);
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> KoiloScriptParser::ParseTerm() {
    auto expr = ParseFactor();
    
    while (Match({TokenType::PLUS, TokenType::MINUS})) {
        Token op = Previous();
        auto right = ParseFactor();
        
        auto binary = std::make_unique<ExpressionNode>();
        binary->type = ExpressionNode::Type::BINARY_OP;
        binary->op = op.value;
        binary->left = std::move(expr);
        binary->right = std::move(right);
        expr = std::move(binary);
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> KoiloScriptParser::ParseFactor() {
    auto expr = ParseUnary();
    
    while (Match({TokenType::MULTIPLY, TokenType::DIVIDE, TokenType::MODULO})) {
        Token op = Previous();
        auto right = ParseUnary();
        
        auto binary = std::make_unique<ExpressionNode>();
        binary->type = ExpressionNode::Type::BINARY_OP;
        binary->op = op.value;
        binary->left = std::move(expr);
        binary->right = std::move(right);
        expr = std::move(binary);
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> KoiloScriptParser::ParseUnary() {
    if (Match({TokenType::MINUS})) {
        Token op = Previous();
        auto right = ParseUnary();
        
        auto unary = std::make_unique<ExpressionNode>();
        unary->type = ExpressionNode::Type::UNARY_OP;
        unary->op = op.value;
        unary->right = std::move(right);
        return unary;
    }
    
    if (Match(TokenType::NOT)) {
        auto right = ParseUnary();
        
        auto unary = std::make_unique<ExpressionNode>();
        unary->type = ExpressionNode::Type::UNARY_OP;
        unary->op = "not";
        unary->right = std::move(right);
        return unary;
    }
    
    return ParsePostfix();
}

std::unique_ptr<ExpressionNode> KoiloScriptParser::ParsePostfix() {
    auto expr = ParsePrimary();
    
    // Unified postfix loop: handle '.member', '(args)', '[index]'
    while (true) {
        if (Check(TokenType::DOT)) {
            Advance(); // consume '.'
            Token ident = Consume(TokenType::IDENTIFIER, "Expected identifier after '.'");
            
            auto access = std::make_unique<ExpressionNode>();
            access->type = ExpressionNode::Type::MEMBER_ACCESS;
            access->left = std::move(expr);
            access->value = ident.value;
            expr = std::move(access);
        }
        else if (Check(TokenType::LPAREN)) {
            Advance(); // consume '('
            
            auto call = std::make_unique<ExpressionNode>();
            call->type = ExpressionNode::Type::FUNCTION_CALL;
            call->left = std::move(expr); // receiver expression
            
            if (!Check(TokenType::RPAREN)) {
                do {
                    call->args.push_back(ParseExpression());
                } while (Match(TokenType::COMMA));
            }
            Consume(TokenType::RPAREN, "Expected ')' after arguments");
            expr = std::move(call);
        }
        else if (Check(TokenType::LBRACKET)) {
            Advance(); // consume '['
            auto index = ParseExpression();
            Consume(TokenType::RBRACKET, "Expected ']' after index");
            
            auto access = std::make_unique<ExpressionNode>();
            access->type = ExpressionNode::Type::INDEX_ACCESS;
            access->left = std::move(expr);
            access->right = std::move(index);
            expr = std::move(access);
        }
        else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> KoiloScriptParser::ParsePrimary() {
    // Number
    if (Check(TokenType::NUMBER)) {
        auto node = std::make_unique<ExpressionNode>();
        node->type = ExpressionNode::Type::NUMBER;
        node->value = Advance().value;
        return node;
    }
    
    // String literal
    if (Check(TokenType::STRING)) {
        auto node = std::make_unique<ExpressionNode>();
        node->type = ExpressionNode::Type::STRING_LITERAL;
        node->value = Advance().value;
        return node;
    }
    
    // Boolean literals
    if (Check(TokenType::TRUE_LITERAL)) {
        Advance();
        auto node = std::make_unique<ExpressionNode>();
        node->type = ExpressionNode::Type::BOOL_LITERAL;
        node->value = "true";
        return node;
    }
    if (Check(TokenType::FALSE_LITERAL)) {
        Advance();
        auto node = std::make_unique<ExpressionNode>();
        node->type = ExpressionNode::Type::BOOL_LITERAL;
        node->value = "false";
        return node;
    }
    
    // Null literal
    if (Check(TokenType::NULL_LITERAL)) {
        Advance();
        auto node = std::make_unique<ExpressionNode>();
        node->type = ExpressionNode::Type::NULL_LITERAL;
        return node;
    }
    
    // Array literal [x, y, z]
    if (Check(TokenType::LBRACKET)) {
        return ParseArrayLiteral();
    }
    
    // Table literal { key: value, ... }
    // Disambiguate: only treat { as table when not at statement level
    // Tables start with { and have key: value pairs inside
    if (Check(TokenType::LBRACE)) {
        // Look ahead: if RBRACE immediately or identifier followed by COLON, it's a table
        size_t savedPos = current;
        Advance(); // consume {
        bool isTable = Check(TokenType::RBRACE); // empty table
        if (!isTable && Check(TokenType::IDENTIFIER)) {
            Advance();
            isTable = Check(TokenType::COLON);
        }
        current = savedPos;
        if (isTable) {
            return ParseTableLiteral();
        }
    }
    
    // Parenthesized expression
    if (Match(TokenType::LPAREN)) {
        auto expr = ParseExpression();
        Consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    // Identifier - postfix handles '.', '()', '[]'
    if (Check(TokenType::IDENTIFIER)) {
        auto node = std::make_unique<ExpressionNode>();
        node->type = ExpressionNode::Type::IDENTIFIER;
        node->value = Advance().value;
        return node;
    }
    
    // new ClassName(args) - constructor for script classes
    if (Check(TokenType::NEW)) {
        Advance(); // consume 'new'
        auto node = std::make_unique<ExpressionNode>();
        node->type = ExpressionNode::Type::NEW_EXPR;
        node->value = Consume(TokenType::IDENTIFIER, "Expected class name after 'new'").value;
        Consume(TokenType::LPAREN, "Expected '(' after class name in new expression");
        while (!Check(TokenType::RPAREN) && !IsAtEnd()) {
            node->args.push_back(ParseExpression());
            if (!Check(TokenType::RPAREN)) {
                Consume(TokenType::COMMA, "Expected ',' between arguments");
            }
        }
        Consume(TokenType::RPAREN, "Expected ')' after new arguments");
        return node;
    }
    
    // self - reference to current script instance
    if (Check(TokenType::SELF)) {
        Advance();
        auto node = std::make_unique<ExpressionNode>();
        node->type = ExpressionNode::Type::SELF;
        node->value = "self";
        return node;
    }
    
    SetError("Expected expression", Peek());
    return nullptr;
}

// ParseMemberAccess removed - absorbed into ParsePostfix() unified postfix loop

std::string KoiloScriptParser::FlattenExpressionToPath(const ExpressionNode* expr) {
    if (!expr) return "";
    
    switch (expr->type) {
        case ExpressionNode::Type::IDENTIFIER:
            return expr->value;
        case ExpressionNode::Type::MEMBER_ACCESS:
            if (expr->left) {
                return FlattenExpressionToPath(expr->left.get()) + "." + expr->value;
            }
            return expr->value;
        case ExpressionNode::Type::FUNCTION_CALL:
            if (expr->left) {
                // Chained call - flatten the receiver
                return FlattenExpressionToPath(expr->left.get());
            }
            return expr->value;
        default:
            return "";
    }
}

std::unique_ptr<ExpressionNode> KoiloScriptParser::ParseArrayLiteral() {
    auto node = std::make_unique<ExpressionNode>();
    node->type = ExpressionNode::Type::ARRAY_LITERAL;
    
    Consume(TokenType::LBRACKET, "Expected '['");
    
    if (!Check(TokenType::RBRACKET)) {
        do {
            node->args.push_back(ParseExpression());
        } while (Match(TokenType::COMMA));
    }
    
    Consume(TokenType::RBRACKET, "Expected ']'");
    
    return node;
}

// ============================================================================
// Property Parsing Helpers
// ============================================================================

std::map<std::string, std::string> KoiloScriptParser::ParseProperties() {
    std::map<std::string, std::string> props;
    
    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        Token key = Consume(TokenType::IDENTIFIER, "Expected property name");
        Consume(TokenType::COLON, "Expected ':' after property name");
        
        std::string value;
        if (Check(TokenType::IDENTIFIER)) {
            value = Advance().value;
        } else if (Check(TokenType::NUMBER)) {
            value = Advance().value;
        } else if (Check(TokenType::STRING)) {
            value = Advance().value;
        } else {
            SetError("Expected property value", Peek());
            break;
        }
        
        props[key.value] = value;
    }
    
    return props;
}

std::map<std::string, std::unique_ptr<ExpressionNode>> KoiloScriptParser::ParseExpressionProperties() {
    std::map<std::string, std::unique_ptr<ExpressionNode>> props;
    
    while (!Check(TokenType::RBRACE) && !IsAtEnd()) {
        // Accept identifiers and keywords as property names
        Token key = Peek();
        if (key.type == TokenType::IDENTIFIER || key.type == TokenType::MATERIAL ||
            key.type == TokenType::MODEL || key.type == TokenType::TEXTURE ||
            key.type == TokenType::CAMERA || key.type == TokenType::OBJECT ||
            key.type == TokenType::STATE) {
            Advance();
        } else {
            SetError("Expected property name", key);
            break;
        }
        Consume(TokenType::COLON, "Expected ':' after property name");
        
        props[key.value] = ParseExpression();
        
        if (HasError()) break;
    }
    
    return props;
}

std::unique_ptr<ExpressionNode> KoiloScriptParser::ParseTableLiteral() {
    auto node = std::make_unique<ExpressionNode>();
    node->type = ExpressionNode::Type::TABLE_LITERAL;
    
    Consume(TokenType::LBRACE, "Expected '{'");
    
    if (!Check(TokenType::RBRACE)) {
        do {
            Token key = Consume(TokenType::IDENTIFIER, "Expected property name");
            Consume(TokenType::COLON, "Expected ':' after property name");
            auto value = ParseExpression();
            node->tableEntries.push_back({key.value, std::move(value)});
        } while (Match(TokenType::COMMA));
    }
    
    Consume(TokenType::RBRACE, "Expected '}'");
    
    return node;
}

std::unique_ptr<EmitNode> KoiloScriptParser::ParseEmitStatement() {
    Advance(); // consume 'emit'
    auto node = std::make_unique<EmitNode>();
    if (Peek().type != TokenType::IDENTIFIER) {
        SetError("Expected signal name after 'emit'", Peek());
        return node;
    }
    node->signalName = Peek().value;
    Advance(); // consume signal name
    
    // Parse arguments: emit signal_name(arg1, arg2, ...);
    if (Peek().type == TokenType::LPAREN) {
        Advance(); // consume '('
        while (Peek().type != TokenType::RPAREN && !IsAtEnd()) {
            node->arguments.push_back(ParseExpression());
            if (HasError()) return node;
            if (Peek().type == TokenType::COMMA) Advance();
        }
        if (Peek().type == TokenType::RPAREN) Advance(); // consume ')'
    }
    if (Peek().type == TokenType::SEMICOLON) Advance();
    return node;
}

} // namespace scripting
} // namespace koilo
