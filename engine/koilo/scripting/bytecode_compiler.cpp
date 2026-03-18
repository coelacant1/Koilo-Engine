// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file bytecode_compiler.cpp
 * @brief Compiles KoiloScript AST into bytecode chunks.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#include <koilo/scripting/bytecode_compiler.hpp>
#include <koilo/scripting/script_class.hpp>
#include <koilo/scripting/reflection_bridge.hpp>
#include <algorithm>
#include <cstdlib>

namespace koilo {
namespace scripting {

// ============================================================================
// Builtin function table
// ============================================================================

static const char* BUILTINS[] = {
    "sin", "cos", "tan", "abs", "sqrt",        // 0-4
    "min", "max", "lerp", "clamp", "print",     // 5-9
    "length", "floor", "ceil", "round",          // 10-13
    "contains", "type", "str", "num", "random",  // 14-18
    "map", "degrees", "radians",                 // 19-21
    "connect", "disconnect", "connect_once",     // 22-24
    "profiler_fps", "profiler_time", "profiler_enable", // 25-27
    "has_module", "list_modules",                        // 28-29
    "start_coroutine", "stop_coroutine",                 // 30-31
    "stop_all_coroutines",                                // 32
    "debug",                                              // 33
    "format",                                             // 34
};
static constexpr int BUILTIN_COUNT = sizeof(BUILTINS) / sizeof(BUILTINS[0]);

bool BytecodeCompiler::IsBuiltin(const std::string& name) const {
    for (int i = 0; i < BUILTIN_COUNT; ++i) {
        if (name == BUILTINS[i]) return true;
    }
    return false;
}

uint16_t BytecodeCompiler::GetBuiltinId(const std::string& name) const {
    for (int i = 0; i < BUILTIN_COUNT; ++i) {
        if (name == BUILTINS[i]) return static_cast<uint16_t>(i);
    }
    return 0xFFFF;
}

// ============================================================================
// Error handling
// ============================================================================

void BytecodeCompiler::SetError(const std::string& msg) {
    if (!hasError_) {
        hasError_ = true;
        error_ = msg;
    }
}

// ============================================================================
// Scope management
// ============================================================================

void BytecodeCompiler::BeginScope() {
    scopes_.push_back({current_->localCount});
}

void BytecodeCompiler::EndScope() {
    if (scopes_.empty()) return;
    int startSlot = scopes_.back().startSlot;
    // Remove locals declared in this scope
    while (current_->localCount > startSlot) {
        current_->localNames.pop_back();
        current_->localCount--;
    }
    scopes_.pop_back();
}

bool BytecodeCompiler::IsLocal(const std::string& name) const {
    return current_->ResolveLocal(name) >= 0;
}

bool BytecodeCompiler::IsMemberPath(const std::string& target) const {
    return target.find('.') != std::string::npos;
}

// ============================================================================
// Top-level compilation
// ============================================================================

CompiledScript BytecodeCompiler::Compile(const scripting::ScriptAST& ast) {
    CompiledScript result;
    script_ = &result;
    hasError_ = false;
    error_.clear();

    // Compile all function declarations first (so they're available)
    for (auto& fn : ast.functions) {
        CompileFunction(fn.get());
        if (hasError_) return result;
    }

    // Also scan initStatements for function declarations
    for (auto& stmt : ast.initStatements) {
        if (stmt->type == scripting::StatementNode::Type::FUNCTION_DECL) {
            auto* fn = static_cast<scripting::FunctionDeclNode*>(stmt.get());
            CompileFunction(fn);
            if (hasError_) return result;
        }
    }

    // Compile main chunk (init statements, excluding function decls already compiled)
    result.mainChunk.name = "<main>";
    result.mainChunk.atoms = &result.atoms;
    current_ = &result.mainChunk;

    for (auto& stmt : ast.initStatements) {
        if (stmt->type == scripting::StatementNode::Type::FUNCTION_DECL) continue;
        CompileStatement(stmt.get());
        if (hasError_) return result;
    }

    current_->Emit(OpCode::RETURN_NONE);
    current_ = nullptr;
    script_ = nullptr;
    return result;
}

// ============================================================================
// Function compilation
// ============================================================================

void BytecodeCompiler::CompileFunction(const scripting::FunctionDeclNode* func) {
    BytecodeChunk chunk;
    chunk.name = func->name;
    chunk.arity = static_cast<int>(func->params.size());
    chunk.atoms = &script_->atoms;

    BytecodeChunk* outer = current_;
    current_ = &chunk;

    // Declare parameters as locals
    for (auto& param : func->params) {
        current_->DeclareLocal(param);
    }

    BeginScope();

    for (auto& stmt : func->body) {
        CompileStatement(stmt.get());
        if (hasError_) {
            current_ = outer;
            return;
        }
    }

    // Implicit return none
    current_->Emit(OpCode::RETURN_NONE);

    EndScope();

    script_->functions[func->name] = std::move(chunk);
    current_ = outer;
}

// ============================================================================
// Statement compilation
// ============================================================================

void BytecodeCompiler::CompileStatement(const scripting::StatementNode* stmt) {
    if (hasError_) return;

    if (stmt->line > 0) {
        current_->EmitLine(stmt->line);
    }

    switch (stmt->type) {
        case scripting::StatementNode::Type::ASSIGNMENT:
            CompileAssignment(static_cast<const scripting::AssignmentNode*>(stmt));
            break;
        case scripting::StatementNode::Type::IF_STATEMENT:
            CompileIfStatement(static_cast<const scripting::IfStatementNode*>(stmt));
            break;
        case scripting::StatementNode::Type::FUNCTION_CALL:
            CompileFunctionCallStmt(static_cast<const scripting::FunctionCallNode*>(stmt));
            break;
        case scripting::StatementNode::Type::VAR_DECL:
            CompileVarDecl(static_cast<const scripting::VarDeclNode*>(stmt));
            break;
        case scripting::StatementNode::Type::WHILE_LOOP:
            CompileWhileLoop(static_cast<const scripting::WhileNode*>(stmt));
            break;
        case scripting::StatementNode::Type::FOR_EACH:
            CompileForEach(static_cast<const scripting::ForEachNode*>(stmt));
            break;
        case scripting::StatementNode::Type::FUNCTION_DECL:
            // Already compiled in first pass
            break;
        case scripting::StatementNode::Type::RETURN:
            CompileReturn(static_cast<const scripting::ReturnNode*>(stmt));
            break;
        case scripting::StatementNode::Type::CLASS_DECL:
            CompileClassDecl(static_cast<const scripting::ClassDeclNode*>(stmt));
            break;
        case scripting::StatementNode::Type::IMPORT:
            break; // Handled at AST level before compilation
        case scripting::StatementNode::Type::SIGNAL_DECL: {
            auto* sig = static_cast<const scripting::SignalDeclNode*>(stmt);
            uint16_t nameIdx = current_->AddStringConstant(sig->name);
            current_->Emit(OpCode::DECLARE_SIGNAL, nameIdx);
            break;
        }
        case scripting::StatementNode::Type::EMIT: {
            auto* emitNode = static_cast<const scripting::EmitNode*>(stmt);
            for (auto& arg : emitNode->arguments) {
                CompileExpression(arg.get());
                if (hasError_) return;
            }
            uint16_t nameIdx = current_->AddStringConstant(emitNode->signalName);
            current_->Emit(OpCode::EMIT_SIGNAL, nameIdx,
                          static_cast<uint8_t>(emitNode->arguments.size()));
            break;
        }

        case scripting::StatementNode::Type::YIELD_STMT: {
            current_->Emit(OpCode::YIELD);
            break;
        }

        case scripting::StatementNode::Type::BREAK_STMT: {
            if (loopStack_.empty()) {
                std::string msg = "'break' used outside of loop";
                if (stmt->line > 0) msg += " (line " + std::to_string(stmt->line) + ")";
                SetError(msg);
            } else {
                loopStack_.back().breakJumps.push_back(current_->Emit(OpCode::JUMP, 0));
            }
            break;
        }

        case scripting::StatementNode::Type::CONTINUE_STMT: {
            if (loopStack_.empty()) {
                std::string msg = "'continue' used outside of loop";
                if (stmt->line > 0) msg += " (line " + std::to_string(stmt->line) + ")";
                SetError(msg);
            } else {
                size_t backOffset = current_->code.size() - loopStack_.back().continueTarget + 3;
                current_->Emit(OpCode::JUMP_BACK, static_cast<uint16_t>(backOffset));
            }
            break;
        }

        default: {
            std::string msg = "Unsupported statement type: " + std::to_string(static_cast<int>(stmt->type));
            if (stmt->line > 0) msg += " (line " + std::to_string(stmt->line) + ")";
            SetError(msg);
            break;
        }
    }
}

void BytecodeCompiler::CompileBlock(const std::vector<std::unique_ptr<scripting::StatementNode>>& stmts) {
    BeginScope();
    for (auto& stmt : stmts) {
        CompileStatement(stmt.get());
        if (hasError_) break;
    }
    EndScope();
}

void BytecodeCompiler::CompileAssignment(const scripting::AssignmentNode* node) {
    const std::string& target = node->target;

    // Index assignment: target[indexExpr] = value  OR  target[indexExpr].member = value
    if (node->indexExpr) {
        // Check if target contains a member path (e.g. "stars.pos" from stars[i].pos = val)
        size_t dotPos = target.find('.');
        if (dotPos != std::string::npos) {
            // Chained index + member: arr[i].field = value
            // SET_MEMBER expects stack: [value, object] (bottom to top)
            std::string rootName = target.substr(0, dotPos);
            std::string memberPath = target.substr(dotPos + 1);

            // Compile value first (goes to bottom of stack)
            CompileExpression(node->value.get());
            if (hasError_) return;

            // Load the root collection and INDEX_GET the element
            int slot = current_->ResolveLocal(rootName);
            if (slot >= 0) {
                current_->Emit(OpCode::LOAD_LOCAL, static_cast<uint16_t>(slot));
            } else {
                uint16_t name = current_->AddStringConstant(rootName);
                current_->Emit(OpCode::LOAD_GLOBAL, name);
            }
            CompileExpression(node->indexExpr.get());
            if (hasError_) return;
            current_->Emit(OpCode::INDEX_GET);

            // Navigate intermediate members if path has multiple dots (a[i].b.c = val)
            size_t lastDot = memberPath.rfind('.');
            if (lastDot != std::string::npos) {
                std::string mid = memberPath.substr(0, lastDot);
                size_t pos = 0;
                while (pos < mid.size()) {
                    size_t d = mid.find('.', pos);
                    std::string part = (d != std::string::npos) ? mid.substr(pos, d - pos) : mid.substr(pos);
                    uint16_t pname = current_->AddStringConstant(part);
                    current_->Emit(OpCode::GET_MEMBER, pname);
                    pos = (d != std::string::npos) ? d + 1 : mid.size();
                }
                memberPath = memberPath.substr(lastDot + 1);
            }

            // SET_MEMBER: stack has [value, object] -> set member
            uint16_t mname = current_->AddStringConstant(memberPath);
            current_->Emit(OpCode::SET_MEMBER, mname);
            return;
        }

        // Simple index assignment: target[indexExpr] = value
        // Stack order for INDEX_SET: collection, index, value (bottom to top)
        // INDEX_SET pushes modified collection back for writeback
        int slot = current_->ResolveLocal(target);
        if (slot >= 0) {
            current_->Emit(OpCode::LOAD_LOCAL, static_cast<uint16_t>(slot));
            CompileExpression(node->indexExpr.get());
            if (hasError_) return;
            CompileExpression(node->value.get());
            if (hasError_) return;
            current_->Emit(OpCode::INDEX_SET);
            current_->Emit(OpCode::STORE_LOCAL, static_cast<uint16_t>(slot));
        } else {
            uint16_t name = current_->AddStringConstant(target);
            current_->Emit(OpCode::LOAD_GLOBAL, name);
            CompileExpression(node->indexExpr.get());
            if (hasError_) return;
            CompileExpression(node->value.get());
            if (hasError_) return;
            current_->Emit(OpCode::INDEX_SET);
            current_->Emit(OpCode::STORE_GLOBAL, name);
        }
        return;
    }

    // Compile the value expression
    CompileExpression(node->value.get());
    if (hasError_) return;

    // Member path assignment: obj.field = value
    if (IsMemberPath(target)) {
        // Split into object path and final member
        size_t lastDot = target.rfind('.');
        std::string objPath = target.substr(0, lastDot);
        std::string member = target.substr(lastDot + 1);

        // Load the object
        size_t firstDot = objPath.find('.');
        std::string rootName = (firstDot != std::string::npos) ? objPath.substr(0, firstDot) : objPath;

        int slot = current_->ResolveLocal(rootName);
        if (slot >= 0) {
            current_->Emit(OpCode::LOAD_LOCAL, static_cast<uint16_t>(slot));
        } else {
            uint16_t name = current_->AddStringConstant(rootName);
            current_->Emit(OpCode::LOAD_GLOBAL, name);
        }

        // Navigate intermediate members
        if (firstDot != std::string::npos && firstDot < lastDot) {
            std::string mid = objPath.substr(firstDot + 1);
            // Split mid by dots and emit GET_MEMBER for each
            size_t pos = 0;
            while (pos < mid.size()) {
                size_t dot = mid.find('.', pos);
                std::string part = (dot != std::string::npos) ? mid.substr(pos, dot - pos) : mid.substr(pos);
                uint16_t pname = current_->AddStringConstant(part);
                current_->Emit(OpCode::GET_MEMBER, pname);
                pos = (dot != std::string::npos) ? dot + 1 : mid.size();
            }
        }

        // SET_MEMBER: stack has [value, object] -> set member
        uint16_t mname = current_->AddStringConstant(member);
        current_->Emit(OpCode::SET_MEMBER, mname);
        return;
    }

    // Simple variable assignment
    int slot = current_->ResolveLocal(target);
    if (slot >= 0) {
        current_->Emit(OpCode::STORE_LOCAL, static_cast<uint16_t>(slot));
    } else {
        uint16_t name = current_->AddStringConstant(target);
        current_->Emit(OpCode::STORE_GLOBAL, name);
    }
}

void BytecodeCompiler::CompileIfStatement(const scripting::IfStatementNode* node) {
    CompileExpression(node->condition.get());
    if (hasError_) return;

    size_t jumpToElse = current_->Emit(OpCode::JUMP_IF_FALSE, 0);
    current_->Emit(OpCode::POP);  // pop truthy condition

    CompileBlock(node->thenBlock);

    if (!node->elseBlock.empty()) {
        size_t jumpOverElse = current_->Emit(OpCode::JUMP, 0);
        current_->PatchJump(jumpToElse);
        current_->Emit(OpCode::POP);  // pop falsy condition
        CompileBlock(node->elseBlock);
        current_->PatchJump(jumpOverElse);
    } else {
        size_t jumpOverPop = current_->Emit(OpCode::JUMP, 0);
        current_->PatchJump(jumpToElse);
        current_->Emit(OpCode::POP);  // pop falsy condition
        current_->PatchJump(jumpOverPop);
    }
}

void BytecodeCompiler::CompileFunctionCallStmt(const scripting::FunctionCallNode* node) {
    // If there's a full expression tree, compile that
    if (node->expression) {
        CompileExpression(node->expression.get());
        current_->Emit(OpCode::POP); // discard return value
        return;
    }

    // Simple function call: funcName(args...)
    const std::string& name = node->functionName;

    // Check for method call (obj.method)
    size_t dot = name.find('.');
    if (dot != std::string::npos) {
        // Method call on object
        std::string objPath = name.substr(0, dot);
        std::string methodName = name.substr(dot + 1);

        // Load object
        int slot = current_->ResolveLocal(objPath);
        if (slot >= 0) {
            current_->Emit(OpCode::LOAD_LOCAL, static_cast<uint16_t>(slot));
        } else {
            uint16_t oname = current_->AddStringConstant(objPath);
            current_->Emit(OpCode::LOAD_GLOBAL, oname);
        }

        // Navigate nested path
        size_t nextDot = methodName.find('.');
        while (nextDot != std::string::npos) {
            std::string part = methodName.substr(0, nextDot);
            uint16_t pname = current_->AddStringConstant(part);
            current_->Emit(OpCode::GET_MEMBER, pname);
            methodName = methodName.substr(nextDot + 1);
            nextDot = methodName.find('.');
        }

        // Compile arguments
        for (auto& arg : node->arguments) {
            CompileExpression(arg.get());
            if (hasError_) return;
        }

        uint16_t mname = current_->AddStringConstant(methodName);
        current_->Emit(OpCode::CALL_METHOD, mname, static_cast<uint8_t>(node->arguments.size()));
        current_->Emit(OpCode::POP); // discard return value from statement
        return;
    }

    // Builtin function
    if (IsBuiltin(name)) {
        for (auto& arg : node->arguments) {
            CompileExpression(arg.get());
            if (hasError_) return;
        }
        uint16_t bid = GetBuiltinId(name);
        current_->Emit(OpCode::CALL_BUILTIN, bid, static_cast<uint8_t>(node->arguments.size()));
        current_->Emit(OpCode::POP);
        return;
    }

    // User function call
    for (auto& arg : node->arguments) {
        CompileExpression(arg.get());
        if (hasError_) return;
    }

    // Push function reference, then CALL
    uint16_t fname = current_->AddStringConstant(name);
    current_->Emit(OpCode::LOAD_GLOBAL, fname);
    current_->EmitByte(OpCode::CALL, static_cast<uint8_t>(node->arguments.size()));
    current_->Emit(OpCode::POP);
}

void BytecodeCompiler::CompileVarDecl(const scripting::VarDeclNode* node) {
    if (node->initializer) {
        CompileExpression(node->initializer.get());
    } else {
        current_->Emit(OpCode::PUSH_NONE);
    }

    if (hasError_) return;

    // In function scope: declare as local
    if (!scopes_.empty()) {
        int slot = current_->DeclareLocal(node->name);
        current_->Emit(OpCode::DECLARE_LOCAL, static_cast<uint16_t>(slot));
    } else {
        // Top-level: store as global
        uint16_t name = current_->AddStringConstant(node->name);
        current_->Emit(OpCode::STORE_GLOBAL, name);
    }
}

void BytecodeCompiler::CompileWhileLoop(const scripting::WhileNode* node) {
    size_t loopStart = current_->code.size();

    loopStack_.push_back({loopStart, {}});

    CompileExpression(node->condition.get());
    if (hasError_) { loopStack_.pop_back(); return; }

    size_t exitJump = current_->Emit(OpCode::JUMP_IF_FALSE, 0);
    current_->Emit(OpCode::POP);  // Pop truthy condition before body

    CompileBlock(node->body);

    // Jump back to condition
    size_t backOffset = current_->code.size() - loopStart + 3;
    current_->Emit(OpCode::JUMP_BACK, static_cast<uint16_t>(backOffset));

    current_->PatchJump(exitJump);
    current_->Emit(OpCode::POP);  // Pop falsy condition on exit

    // Patch all break jumps to after the loop
    for (size_t bj : loopStack_.back().breakJumps) {
        current_->PatchJump(bj);
    }
    loopStack_.pop_back();
}

void BytecodeCompiler::CompileForEach(const scripting::ForEachNode* node) {
    // Compile collection expression
    CompileExpression(node->collection.get());
    if (hasError_) return;

    // ITER_INIT: convert to iterator
    current_->Emit(OpCode::ITER_INIT);

    size_t loopStart = current_->code.size();

    loopStack_.push_back({loopStart, {}});

    // ITER_NEXT: advance or jump to end
    size_t exitJump = current_->Emit(OpCode::ITER_NEXT, 0);

    // ITER_VALUE: push current value
    current_->Emit(OpCode::ITER_VALUE);

    BeginScope();
    int slot = current_->DeclareLocal(node->varName);
    current_->Emit(OpCode::DECLARE_LOCAL, static_cast<uint16_t>(slot));

    for (auto& stmt : node->body) {
        CompileStatement(stmt.get());
        if (hasError_) break;
    }
    EndScope();

    // Jump back to ITER_NEXT
    size_t backOffset = current_->code.size() - loopStart + 3;
    current_->Emit(OpCode::JUMP_BACK, static_cast<uint16_t>(backOffset));

    current_->PatchJump(exitJump);

    // Patch all break jumps to after the loop
    for (size_t bj : loopStack_.back().breakJumps) {
        current_->PatchJump(bj);
    }
    loopStack_.pop_back();

    // Pop iterator
    current_->Emit(OpCode::POP);
}

void BytecodeCompiler::CompileReturn(const scripting::ReturnNode* node) {
    if (node->value) {
        CompileExpression(node->value.get());
        current_->Emit(OpCode::RETURN);
    } else {
        current_->Emit(OpCode::RETURN_NONE);
    }
}

// ============================================================================
// Expression compilation - each pushes exactly one value onto the stack
// ============================================================================

void BytecodeCompiler::CompileExpression(const scripting::ExpressionNode* expr) {
    if (hasError_ || !expr) return;

    switch (expr->type) {
        case scripting::ExpressionNode::Type::NUMBER:
            CompileNumber(expr);
            break;
        case scripting::ExpressionNode::Type::STRING_LITERAL:
            CompileString(expr);
            break;
        case scripting::ExpressionNode::Type::IDENTIFIER:
            CompileIdentifier(expr);
            break;
        case scripting::ExpressionNode::Type::BINARY_OP:
            CompileBinaryOp(expr);
            break;
        case scripting::ExpressionNode::Type::UNARY_OP:
            CompileUnaryOp(expr);
            break;
        case scripting::ExpressionNode::Type::FUNCTION_CALL:
            CompileFunctionCall(expr);
            break;
        case scripting::ExpressionNode::Type::MEMBER_ACCESS:
            CompileMemberAccess(expr);
            break;
        case scripting::ExpressionNode::Type::ARRAY_LITERAL:
            CompileArrayLiteral(expr);
            break;
        case scripting::ExpressionNode::Type::TABLE_LITERAL:
            CompileTableLiteral(expr);
            break;
        case scripting::ExpressionNode::Type::INDEX_ACCESS:
            CompileIndexAccess(expr);
            break;
        case scripting::ExpressionNode::Type::TERNARY:
            CompileTernary(expr);
            break;
        case scripting::ExpressionNode::Type::NULL_LITERAL:
            current_->Emit(OpCode::PUSH_NONE);
            break;
        case scripting::ExpressionNode::Type::BOOL_LITERAL:
            current_->Emit(expr->value == "true" ? OpCode::PUSH_TRUE : OpCode::PUSH_FALSE);
            break;
        case scripting::ExpressionNode::Type::NEW_EXPR:
            CompileNewExpr(expr);
            break;
        case scripting::ExpressionNode::Type::SELF:
            current_->Emit(OpCode::LOAD_SELF);
            break;
    }
}

void BytecodeCompiler::CompileNumber(const scripting::ExpressionNode* expr) {
    double val = std::strtod(expr->value.c_str(), nullptr);
    uint16_t idx = current_->AddNumberConstant(val);
    current_->Emit(OpCode::PUSH_CONST, idx);
}

void BytecodeCompiler::CompileString(const scripting::ExpressionNode* expr) {
    uint16_t idx = current_->AddStringConstant(expr->value);
    current_->Emit(OpCode::PUSH_CONST, idx);
}

void BytecodeCompiler::CompileIdentifier(const scripting::ExpressionNode* expr) {
    const std::string& name = expr->value;

    int slot = current_->ResolveLocal(name);
    if (slot >= 0) {
        current_->Emit(OpCode::LOAD_LOCAL, static_cast<uint16_t>(slot));
    } else if (scripting::ReflectionBridge::FindClass(name)) {
        uint16_t idx = current_->AddStringConstant(name);
        current_->Emit(OpCode::LOAD_CLASS, idx);
    } else {
        uint16_t idx = current_->AddStringConstant(name);
        current_->Emit(OpCode::LOAD_GLOBAL, idx);
    }
}

void BytecodeCompiler::CompileBinaryOp(const scripting::ExpressionNode* expr) {
    // Short-circuit for AND/OR
    if (expr->op == "&&" || expr->op == "and") {
        CompileExpression(expr->left.get());
        size_t jump = current_->Emit(OpCode::JUMP_IF_FALSE, 0);
        current_->Emit(OpCode::POP);
        CompileExpression(expr->right.get());
        current_->PatchJump(jump);
        return;
    }
    if (expr->op == "||" || expr->op == "or") {
        CompileExpression(expr->left.get());
        size_t jump = current_->Emit(OpCode::JUMP_IF_TRUE, 0);
        current_->Emit(OpCode::POP);
        CompileExpression(expr->right.get());
        current_->PatchJump(jump);
        return;
    }

    CompileExpression(expr->left.get());
    if (hasError_) return;
    CompileExpression(expr->right.get());
    if (hasError_) return;

    if (expr->op == "+") current_->Emit(OpCode::ADD);
    else if (expr->op == "-") current_->Emit(OpCode::SUB);
    else if (expr->op == "*") current_->Emit(OpCode::MUL);
    else if (expr->op == "/") current_->Emit(OpCode::DIV);
    else if (expr->op == "%") current_->Emit(OpCode::MOD);
    else if (expr->op == "==") current_->Emit(OpCode::EQ);
    else if (expr->op == "!=") current_->Emit(OpCode::NEQ);
    else if (expr->op == "<") current_->Emit(OpCode::LT);
    else if (expr->op == ">") current_->Emit(OpCode::GT);
    else if (expr->op == "<=") current_->Emit(OpCode::LTE);
    else if (expr->op == ">=") current_->Emit(OpCode::GTE);
    else {
        std::string msg = "Unknown binary operator: " + expr->op;
        if (expr->line > 0) msg += " (line " + std::to_string(expr->line) + ")";
        SetError(msg);
    }
}

void BytecodeCompiler::CompileUnaryOp(const scripting::ExpressionNode* expr) {
    // Parser stores unary operand in 'right' (left is unused for unary ops)
    auto* operand = expr->right ? expr->right.get() : expr->left.get();
    CompileExpression(operand);
    if (hasError_) return;

    if (expr->op == "-") current_->Emit(OpCode::NEG);
    else if (expr->op == "!" || expr->op == "not") current_->Emit(OpCode::NOT);
    else {
        std::string msg = "Unknown unary operator: " + expr->op;
        if (expr->line > 0) msg += " (line " + std::to_string(expr->line) + ")";
        SetError(msg);
    }
}

void BytecodeCompiler::CompileFunctionCall(const scripting::ExpressionNode* expr) {
    // If there's a left expression, determine if it's an object method call
    // or a simple function call (e.g., add(3, 4) where left is IDENTIFIER "add")
    if (expr->left) {
        // Simple function call: left is just an identifier (function name)
        if (expr->left->type == scripting::ExpressionNode::Type::IDENTIFIER && expr->value.empty()) {
            const std::string& funcName = expr->left->value;

            // Check builtin first
            if (IsBuiltin(funcName)) {
                for (auto& arg : expr->args) {
                    CompileExpression(arg.get());
                    if (hasError_) return;
                }
                uint16_t bid = GetBuiltinId(funcName);
                current_->Emit(OpCode::CALL_BUILTIN, bid, static_cast<uint8_t>(expr->args.size()));
                return;
            }

            // User function or constructor: push args, load func, call
            for (auto& arg : expr->args) {
                CompileExpression(arg.get());
                if (hasError_) return;
            }

            // If the name matches a reflected class, emit CONSTRUCT directly
            if (scripting::ReflectionBridge::FindClass(funcName)) {
                uint16_t fname = current_->AddStringConstant(funcName);
                current_->Emit(OpCode::CONSTRUCT, fname, static_cast<uint8_t>(expr->args.size()));
                return;
            }

            uint16_t fname = current_->AddStringConstant(funcName);
            current_->Emit(OpCode::LOAD_GLOBAL, fname);
            current_->EmitByte(OpCode::CALL, static_cast<uint8_t>(expr->args.size()));
            return;
        }

        // Method call: left is receiver expression (object.method or chained)
        // Flatten to extract the method name from the call node's value field,
        // or from a member access chain
        std::string methodName = expr->value;
        if (methodName.empty() && expr->left->type == scripting::ExpressionNode::Type::MEMBER_ACCESS) {
            // The method name is the member access's value; compile the object part
            methodName = expr->left->value;
            CompileExpression(expr->left->left.get());
        } else {
            CompileExpression(expr->left.get());
        }
        if (hasError_) return;

        for (auto& arg : expr->args) {
            CompileExpression(arg.get());
            if (hasError_) return;
        }

        uint16_t mname = current_->AddStringConstant(methodName);
        current_->Emit(OpCode::CALL_METHOD, mname, static_cast<uint8_t>(expr->args.size()));
        return;
    }

    const std::string& name = expr->value;

    // Check for method call via dotted name
    size_t dot = name.find('.');
    if (dot != std::string::npos) {
        std::string objName = name.substr(0, dot);
        std::string rest = name.substr(dot + 1);

        // Load object
        int slot = current_->ResolveLocal(objName);
        if (slot >= 0) {
            current_->Emit(OpCode::LOAD_LOCAL, static_cast<uint16_t>(slot));
        } else if (scripting::ReflectionBridge::FindClass(objName)) {
            uint16_t oname = current_->AddStringConstant(objName);
            current_->Emit(OpCode::LOAD_CLASS, oname);
        } else {
            uint16_t oname = current_->AddStringConstant(objName);
            current_->Emit(OpCode::LOAD_GLOBAL, oname);
        }

        // Navigate intermediate members
        size_t nextDot = rest.find('.');
        while (nextDot != std::string::npos) {
            std::string part = rest.substr(0, nextDot);
            uint16_t pname = current_->AddStringConstant(part);
            current_->Emit(OpCode::GET_MEMBER, pname);
            rest = rest.substr(nextDot + 1);
            nextDot = rest.find('.');
        }

        // Compile arguments
        for (auto& arg : expr->args) {
            CompileExpression(arg.get());
            if (hasError_) return;
        }

        uint16_t mname = current_->AddStringConstant(rest);
        current_->Emit(OpCode::CALL_METHOD, mname, static_cast<uint8_t>(expr->args.size()));
        return;
    }

    // Constructor call: if name matches a registered class
    // We can't know at compile time if it's a class, so emit CONSTRUCT
    // and let the VM fall back to function call if it's not a class
    // Actually, classes start with uppercase by convention - but let's
    // handle this at VM level. Emit as function call; VM will check.

    // Builtin function
    if (IsBuiltin(name)) {
        for (auto& arg : expr->args) {
            CompileExpression(arg.get());
            if (hasError_) return;
        }
        uint16_t bid = GetBuiltinId(name);
        current_->Emit(OpCode::CALL_BUILTIN, bid, static_cast<uint8_t>(expr->args.size()));
        return;
    }

    // User function or constructor
    for (auto& arg : expr->args) {
        CompileExpression(arg.get());
        if (hasError_) return;
    }

    // Emit CONSTRUCT if it's a reflected class
    if (scripting::ReflectionBridge::FindClass(name)) {
        uint16_t fname = current_->AddStringConstant(name);
        current_->Emit(OpCode::CONSTRUCT, fname, static_cast<uint8_t>(expr->args.size()));
        return;
    }

    // Otherwise treat as function call
    uint16_t fname = current_->AddStringConstant(name);
    current_->Emit(OpCode::LOAD_GLOBAL, fname);
    current_->EmitByte(OpCode::CALL, static_cast<uint8_t>(expr->args.size()));
}

void BytecodeCompiler::CompileMemberAccess(const scripting::ExpressionNode* expr) {
    // Member access: left.value
    if (expr->left) {
        CompileExpression(expr->left.get());
    } else {
        // Flat path in value field: "obj.field.sub"
        const std::string& path = expr->value;
        size_t firstDot = path.find('.');
        if (firstDot == std::string::npos) {
            // Just an identifier
            CompileIdentifier(expr);
            return;
        }

        std::string rootName = path.substr(0, firstDot);
        int slot = current_->ResolveLocal(rootName);
        if (slot >= 0) {
            current_->Emit(OpCode::LOAD_LOCAL, static_cast<uint16_t>(slot));
        } else if (scripting::ReflectionBridge::FindClass(rootName)) {
            uint16_t idx = current_->AddStringConstant(rootName);
            current_->Emit(OpCode::LOAD_CLASS, idx);
        } else {
            uint16_t idx = current_->AddStringConstant(rootName);
            current_->Emit(OpCode::LOAD_GLOBAL, idx);
        }

        // Emit GET_MEMBER for each part after the first
        std::string rest = path.substr(firstDot + 1);
        size_t pos = 0;
        while (pos < rest.size()) {
            size_t dot = rest.find('.', pos);
            std::string part = (dot != std::string::npos) ? rest.substr(pos, dot - pos) : rest.substr(pos);
            uint16_t pname = current_->AddStringConstant(part);
            current_->Emit(OpCode::GET_MEMBER, pname);
            pos = (dot != std::string::npos) ? dot + 1 : rest.size();
        }
        return;
    }

    if (hasError_) return;

    uint16_t mname = current_->AddStringConstant(expr->value);
    current_->Emit(OpCode::GET_MEMBER, mname);
}

void BytecodeCompiler::CompileArrayLiteral(const scripting::ExpressionNode* expr) {
    for (auto& elem : expr->args) {
        CompileExpression(elem.get());
        if (hasError_) return;
    }
    current_->Emit(OpCode::MAKE_ARRAY, static_cast<uint16_t>(expr->args.size()));
}

void BytecodeCompiler::CompileTableLiteral(const scripting::ExpressionNode* expr) {
    for (auto& [key, val] : expr->tableEntries) {
        uint16_t kidx = current_->AddStringConstant(key);
        current_->Emit(OpCode::PUSH_CONST, kidx);
        CompileExpression(val.get());
        if (hasError_) return;
    }
    current_->Emit(OpCode::MAKE_TABLE, static_cast<uint16_t>(expr->tableEntries.size()));
}

void BytecodeCompiler::CompileIndexAccess(const scripting::ExpressionNode* expr) {
    CompileExpression(expr->left.get());
    if (hasError_) return;
    CompileExpression(expr->right.get());
    if (hasError_) return;
    current_->Emit(OpCode::INDEX_GET);
}

void BytecodeCompiler::CompileTernary(const scripting::ExpressionNode* expr) {
    CompileExpression(expr->condition.get());
    if (hasError_) return;

    size_t jumpToFalse = current_->Emit(OpCode::JUMP_IF_FALSE, 0);
    CompileExpression(expr->left.get());
    size_t jumpOverElse = current_->Emit(OpCode::JUMP, 0);
    current_->PatchJump(jumpToFalse);
    CompileExpression(expr->right.get());
    current_->PatchJump(jumpOverElse);
}

// ============================================================================
// Script class compilation
// ============================================================================

void BytecodeCompiler::CompileClassDecl(const scripting::ClassDeclNode* node) {
    // Create ScriptClass and register it in the compiled script
    auto klass = std::make_unique<ScriptClass>();
    klass->name = node->name;

    // Register fields with defaults
    for (auto& field : node->fields) {
        ScriptClass::FieldDef fd;
        fd.name = field.name;
        klass->fieldDefs.push_back(fd);
        klass->fieldDefaults.push_back(Value()); // Placeholder; actual init in __init chunk
    }

    // Compile field initializer: ClassName.__init(self) sets all field defaults
    {
        std::string initName = node->name + ".__init";
        BytecodeChunk initChunk;
        initChunk.name = initName;
        initChunk.arity = 1;
        initChunk.atoms = &script_->atoms;

        BytecodeChunk* outer = current_;
        current_ = &initChunk;
        current_->DeclareLocal("self");

        for (auto& field : node->fields) {
            if (field.defaultValue) {
                // SET_MEMBER pops obj, then pops val
                // Stack needs: val (bottom), obj=self (top)
                CompileExpression(field.defaultValue.get()); // push value
                if (hasError_) { current_ = outer; return; }
                current_->Emit(OpCode::LOAD_LOCAL, static_cast<uint16_t>(0)); // push self
                uint16_t fieldIdx = current_->AddStringConstant(field.name);
                current_->Emit(OpCode::SET_MEMBER, fieldIdx);
            }
        }
        current_->Emit(OpCode::RETURN_NONE);
        script_->functions[initName] = std::move(initChunk);
        current_ = outer;
    }

    // Compile methods as named functions in the script
    for (auto& method : node->methods) {
        std::string qualifiedName = node->name + "." + method->name;
        BytecodeChunk chunk;
        chunk.name = qualifiedName;
        chunk.arity = static_cast<int>(method->params.size()) + 1; // +1 for implicit 'self'
        chunk.atoms = &script_->atoms;

        BytecodeChunk* outer = current_;
        current_ = &chunk;

        // First local is always 'self'
        current_->DeclareLocal("self");
        for (auto& param : method->params) {
            current_->DeclareLocal(param);
        }

        BeginScope();
        for (auto& stmt : method->body) {
            CompileStatement(stmt.get());
            if (hasError_) { current_ = outer; return; }
        }
        EndScope();
        current_->Emit(OpCode::RETURN_NONE);

        klass->compiledMethods[method->name] = nullptr; // Pointer set after move
        klass->methods[method->name] = method.get();
        script_->functions[qualifiedName] = std::move(chunk);
        current_ = outer;
    }

    // Store class in compiled script
    script_->classes[node->name] = std::move(klass);
}

void BytecodeCompiler::CompileNewExpr(const scripting::ExpressionNode* expr) {
    // Compile arguments
    for (auto& arg : expr->args) {
        CompileExpression(arg.get());
        if (hasError_) return;
    }
    uint16_t nameIdx = current_->AddStringConstant(expr->value);
    current_->Emit(OpCode::NEW_INSTANCE, nameIdx, static_cast<uint8_t>(expr->args.size()));
}

} // namespace scripting
} // namespace koilo
