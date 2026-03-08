// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>

namespace koilo {
namespace scripting {

// Forward declarations
class ASTNode;
class ExpressionNode;

/**
 * @brief Base class for all AST nodes
 */
class ASTNode {
public:
    virtual ~ASTNode() = default;
    int line = 0;
    int column = 0;
};

/**
 * @brief Expression node (for calculations, values, etc.)
 */
class ExpressionNode : public ASTNode {
public:
    enum class Type {
        NUMBER,
        STRING_LITERAL, // String value (not identifier)
        IDENTIFIER,
        BINARY_OP,      // e.g., a + b, x * y
        UNARY_OP,       // e.g., -x, not x
        FUNCTION_CALL,  // e.g., sin(time)
        MEMBER_ACCESS,  // e.g., face.position.y
        ARRAY_LITERAL,  // e.g., [0, 1, 2]
        TABLE_LITERAL,  // e.g., { speed: 5.0, name: "player" }
        INDEX_ACCESS,   // e.g., items[0]
        TERNARY,        // e.g., cond ? a : b
        NULL_LITERAL,   // null
        BOOL_LITERAL,   // true, false
        NEW_EXPR,       // new ClassName(args)
        SELF            // self reference in class methods
    };
    
    Type type;
    std::string value;
    std::string op;  // For operators
    std::unique_ptr<ExpressionNode> left;
    std::unique_ptr<ExpressionNode> right;
    std::unique_ptr<ExpressionNode> condition; // For ternary
    std::vector<std::unique_ptr<ExpressionNode>> args;  // For function calls or array literals
    std::vector<std::pair<std::string, std::unique_ptr<ExpressionNode>>> tableEntries; // For table literals
};

/**
 * @brief DISPLAY block configuration
 */
struct DisplayNode : public ASTNode {
    std::map<std::string, std::string> properties;
    // Common properties: type, width, height, brightness
};

/**
 * @brief ASSETS block - asset declarations
 */
struct AssetNode : public ASTNode {
    enum class AssetType {
        MODEL,
        TEXTURE,
        AUDIO
    };
    
    AssetType type;
    std::string name;      // Asset identifier
    std::string filepath;  // Path to asset file
};

/**
 * @brief SCENE object declaration
 */
struct ObjectNode : public ASTNode {
    std::string id;
    std::string className;  // C++ class to instantiate (e.g., "Light", "Mesh")
    std::string modelRef;  // Reference to asset (optional, for backward compat)
    std::map<std::string, std::unique_ptr<ExpressionNode>> properties;
    // Properties: position, rotation, scale, or any reflected fields
};

/**
 * @brief Material declaration in ASSETS block
 */
struct MaterialNode : public ASTNode {
    std::string name;       // Material identifier (e.g., "faceMat")
    std::string className;  // C++ class name (e.g., "UniformColorMaterial")
    std::map<std::string, std::unique_ptr<ExpressionNode>> properties;
};

/**
 * @brief CAMERA declaration
 */
struct CameraNode : public ASTNode {
    std::string id;
    std::map<std::string, std::unique_ptr<ExpressionNode>> properties;
    // Properties: position, target, fov
};

/**
 * @brief STATE definition
 */
struct StateNode : public ASTNode {
    std::string name;
    std::map<std::string, std::unique_ptr<ExpressionNode>> properties;
    std::vector<std::unique_ptr<class StatementNode>> statements;
};

/**
 * @brief Base class for statements in UPDATE/STATE blocks
 */
class StatementNode : public ASTNode {
public:
    enum class Type {
        ASSIGNMENT,      // face.position = value
        IF_STATEMENT,    // if (condition) { ... }
        FUNCTION_CALL,   // set_state(name)
        RETURN,          // return value
        VAR_DECL,        // var x = 5
        WHILE_LOOP,      // while condition { ... }
        FOR_EACH,        // for item in collection { ... }
        FUNCTION_DECL,   // function name(args) { ... }
        CLASS_DECL,      // class Name { ... }
        IMPORT,          // import "path.ks";
        SIGNAL_DECL,     // signal name(params);
        EMIT,            // emit name(args);
        YIELD_STMT,      // yield;
        BREAK_STMT,      // break;
        CONTINUE_STMT    // continue;
    };
    
    Type type;
};

/**
 * @brief Assignment statement (face.position.y = 5.0)
 */
struct AssignmentNode : public StatementNode {
    std::string target;  // e.g., "face.position.y"
    std::unique_ptr<ExpressionNode> value;
    std::unique_ptr<ExpressionNode> indexExpr;  // e.g., _targets[name] -> indexExpr is "name"
    
    AssignmentNode() { type = Type::ASSIGNMENT; }
};

/**
 * @brief If statement
 */
struct IfStatementNode : public StatementNode {
    std::unique_ptr<ExpressionNode> condition;
    std::vector<std::unique_ptr<StatementNode>> thenBlock;
    std::vector<std::unique_ptr<StatementNode>> elseBlock;
    
    IfStatementNode() { type = Type::IF_STATEMENT; }
};

/**
 * @brief Function call statement
 */
struct FunctionCallNode : public StatementNode {
    std::string functionName;
    std::vector<std::unique_ptr<ExpressionNode>> arguments;
    std::unique_ptr<ExpressionNode> expression; // Full expression tree for chained calls
    
    FunctionCallNode() { type = Type::FUNCTION_CALL; }
};

/**
 * @brief Variable declaration (var x = 5)
 */
struct VarDeclNode : public StatementNode {
    std::string name;
    std::unique_ptr<ExpressionNode> initializer; // may be null
    
    VarDeclNode() { type = Type::VAR_DECL; }
};

/**
 * @brief While loop (while condition { ... })
 */
struct WhileNode : public StatementNode {
    std::unique_ptr<ExpressionNode> condition;
    std::vector<std::unique_ptr<StatementNode>> body;
    
    WhileNode() { type = Type::WHILE_LOOP; }
};

/**
 * @brief For-each loop (for item in collection { ... })
 */
struct ForEachNode : public StatementNode {
    std::string varName;
    std::unique_ptr<ExpressionNode> collection;
    std::vector<std::unique_ptr<StatementNode>> body;
    
    ForEachNode() { type = Type::FOR_EACH; }
};

/**
 * @brief Function declaration (function name(args) { ... })
 */
struct FunctionDeclNode : public StatementNode {
    std::string name;
    std::vector<std::string> params;
    std::vector<std::unique_ptr<StatementNode>> body;
    
    FunctionDeclNode() { type = Type::FUNCTION_DECL; }
};

/**
 * @brief Return statement (return value)
 */
struct ReturnNode : public StatementNode {
    std::unique_ptr<ExpressionNode> value; // may be null for bare return
    
    ReturnNode() { type = Type::RETURN; }
};

/**
 * @brief Class declaration (class Name { var field = default; fn method() { ... } })
 */
struct ClassFieldDecl {
    std::string name;
    std::unique_ptr<ExpressionNode> defaultValue; // may be null
};

struct ClassDeclNode : public StatementNode {
    std::string name;
    std::vector<ClassFieldDecl> fields;
    std::vector<std::unique_ptr<FunctionDeclNode>> methods;
    
    ClassDeclNode() { type = Type::CLASS_DECL; }
};

struct ImportNode : public StatementNode {
    std::string path; // e.g., "utils/math_helpers.ks"
    
    ImportNode() { type = Type::IMPORT; }
};

struct SignalDeclNode : public StatementNode {
    std::string name;
    std::vector<std::string> params; // documentation only
    
    SignalDeclNode() { type = Type::SIGNAL_DECL; }
};

struct EmitNode : public StatementNode {
    std::string signalName;
    std::vector<std::unique_ptr<ExpressionNode>> arguments;
    
    EmitNode() { type = Type::EMIT; }
};

struct YieldNode : public StatementNode {
    YieldNode() { type = Type::YIELD_STMT; }
};

struct BreakNode : public StatementNode {
    BreakNode() { type = Type::BREAK_STMT; }
};

struct ContinueNode : public StatementNode {
    ContinueNode() { type = Type::CONTINUE_STMT; }
};

/**
 * @brief Control mapping (key/pin -> action)
 */
struct ControlNode : public ASTNode {
    std::string inputType;  // "key" or "pin"
    std::string inputId;    // Key name/number (e.g., "1", "b", "q")
    std::string action;     // Action function name (e.g., "set_state", "blink", "quit")
    std::vector<std::string> actionArgs;  // Action arguments (e.g., ["default"])
};

/**
 * @brief Transition animation config
 */
struct TransitionNode : public ASTNode {
    std::string target;  // "morph", "color", etc.
    std::map<std::string, std::unique_ptr<ExpressionNode>> properties;
};

/**
 * @brief Auto animation config (e.g., auto-blink)
 */
struct AutoAnimNode : public ASTNode {
    std::string name;
    std::map<std::string, std::unique_ptr<ExpressionNode>> properties;
};

/**
 * @brief UPDATE block
 */
struct UpdateNode : public ASTNode {
    std::vector<std::unique_ptr<StatementNode>> statements;
};

/**
 * @brief Complete KoiloScript AST
 */
struct ScriptAST {
    std::unique_ptr<DisplayNode> display;
    std::vector<std::unique_ptr<AssetNode>> assets;
    std::vector<std::unique_ptr<MaterialNode>> materials;
    std::vector<std::unique_ptr<ObjectNode>> objects;
    std::vector<std::unique_ptr<CameraNode>> cameras;
    std::vector<std::unique_ptr<StateNode>> states;
    std::vector<std::unique_ptr<ControlNode>> controls;
    std::vector<std::unique_ptr<TransitionNode>> transitions;
    std::vector<std::unique_ptr<AutoAnimNode>> autoAnims;
    std::vector<std::unique_ptr<FunctionDeclNode>> functions;
    std::unique_ptr<UpdateNode> update;

    // Code-first: top-level statements executed during init (var decls, method calls, etc.)
    std::vector<std::unique_ptr<StatementNode>> initStatements;

    // Import declarations (processed before compilation)
    std::vector<std::string> imports;

    // Signal declarations
    std::vector<std::string> signals;
};

} // namespace scripting
} // namespace koilo
