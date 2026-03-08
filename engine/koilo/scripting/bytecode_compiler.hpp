// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file bytecode_compiler.hpp
 * @brief Compiles KoiloScript AST into bytecode chunks.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <koilo/scripting/bytecode.hpp>
#include <koilo/scripting/koiloscript_ast.hpp>
#include <string>
#include <vector>
#include "../registry/reflect_macros.hpp"

namespace koilo {
namespace scripting {

class BytecodeCompiler {
public:
    /**
     * @brief Compile a full script AST into a CompiledScript.
     * @param ast The parsed script AST.
     * @return CompiledScript with mainChunk (init/Setup) and function chunks.
     */
    CompiledScript Compile(const scripting::ScriptAST& ast);

    bool HasError() const { return hasError_; }
    const std::string& GetError() const { return error_; }

private:
    // Current chunk being compiled
    BytecodeChunk* current_ = nullptr;
    CompiledScript* script_ = nullptr;
    bool hasError_ = false;
    std::string error_;

    // Scope tracking for locals
    struct Scope {
        int startSlot = 0;

        KL_BEGIN_FIELDS(Scope)
            /* No reflected fields. */
        KL_END_FIELDS

        KL_BEGIN_METHODS(Scope)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(Scope)
            /* No reflected ctors. */
        KL_END_DESCRIBE(Scope)

    };
    std::vector<Scope> scopes_;

    void SetError(const std::string& msg);

    // Loop context for break/continue
    struct LoopContext {
        size_t continueTarget;             // Code offset to jump to on continue
        std::vector<size_t> breakJumps;    // JUMP offsets to patch on loop exit

        KL_BEGIN_FIELDS(LoopContext)
            KL_FIELD(LoopContext, continueTarget, "Continue target", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(LoopContext)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(LoopContext)
            /* No reflected ctors. */
        KL_END_DESCRIBE(LoopContext)

    };
    std::vector<LoopContext> loopStack_;

    // Statement compilation
    void CompileStatement(const scripting::StatementNode* stmt);
    void CompileAssignment(const scripting::AssignmentNode* node);
    void CompileIfStatement(const scripting::IfStatementNode* node);
    void CompileFunctionCallStmt(const scripting::FunctionCallNode* node);
    void CompileVarDecl(const scripting::VarDeclNode* node);
    void CompileWhileLoop(const scripting::WhileNode* node);
    void CompileForEach(const scripting::ForEachNode* node);
    void CompileReturn(const scripting::ReturnNode* node);
    void CompileBlock(const std::vector<std::unique_ptr<scripting::StatementNode>>& stmts);

    // Expression compilation (pushes result onto stack)
    void CompileExpression(const scripting::ExpressionNode* expr);
    void CompileNumber(const scripting::ExpressionNode* expr);
    void CompileString(const scripting::ExpressionNode* expr);
    void CompileIdentifier(const scripting::ExpressionNode* expr);
    void CompileBinaryOp(const scripting::ExpressionNode* expr);
    void CompileUnaryOp(const scripting::ExpressionNode* expr);
    void CompileFunctionCall(const scripting::ExpressionNode* expr);
    void CompileMemberAccess(const scripting::ExpressionNode* expr);
    void CompileArrayLiteral(const scripting::ExpressionNode* expr);
    void CompileTableLiteral(const scripting::ExpressionNode* expr);
    void CompileIndexAccess(const scripting::ExpressionNode* expr);
    void CompileTernary(const scripting::ExpressionNode* expr);

    // Class compilation
    void CompileClassDecl(const scripting::ClassDeclNode* node);
    void CompileNewExpr(const scripting::ExpressionNode* expr);

    // Function compilation
    void CompileFunction(const scripting::FunctionDeclNode* func);

    // Helpers
    void BeginScope();
    void EndScope();
    bool IsLocal(const std::string& name) const;
    bool IsBuiltin(const std::string& name) const;
    uint16_t GetBuiltinId(const std::string& name) const;

    // Path helpers for member access / assignment targets
    bool IsMemberPath(const std::string& target) const;

    KL_BEGIN_FIELDS(BytecodeCompiler)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(BytecodeCompiler)
        KL_METHOD_AUTO(BytecodeCompiler, HasError, "Has error"),
        KL_METHOD_AUTO(BytecodeCompiler, GetError, "Get error")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(BytecodeCompiler)
        /* No reflected ctors. */
    KL_END_DESCRIBE(BytecodeCompiler)

};

} // namespace scripting
} // namespace koilo
