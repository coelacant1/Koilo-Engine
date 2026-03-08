// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file bytecode.hpp
 * @brief Bytecode instruction set and chunk format for the KoiloScript VM.
 *
 * The bytecode is a flat uint8_t[] instruction stream produced by compiling
 * an AST. Each instruction is a 1-byte opcode followed by zero or more
 * operand bytes. Constants (numbers, strings) are stored in a separate
 * constant pool and referenced by 16-bit index.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <koilo/scripting/string_atom.hpp>
#include <koilo/scripting/script_class.hpp>
#include "../registry/reflect_macros.hpp"

namespace koilo {
namespace scripting {

// ============================================================================
// Opcodes
// ============================================================================

enum class OpCode : uint8_t {
    // Stack manipulation
    PUSH_CONST,         // [16-bit index] Push constant from pool
    PUSH_TRUE,          // Push boolean true
    PUSH_FALSE,         // Push boolean false
    PUSH_NONE,          // Push none value
    POP,                // Discard top of stack

    // Variables
    LOAD_LOCAL,         // [16-bit slot] Load local variable
    STORE_LOCAL,        // [16-bit slot] Store into local variable
    LOAD_GLOBAL,        // [16-bit name] Load global variable by name constant
    STORE_GLOBAL,       // [16-bit name] Store into global variable by name constant
    DECLARE_LOCAL,      // [16-bit slot] Declare local (pop value, store in slot)
    LOAD_CLASS,         // [16-bit name] Load reflected class by name (static access)

    // Arithmetic
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    NEG,                // Unary negate

    // Comparison
    EQ,
    NEQ,
    LT,
    GT,
    LTE,
    GTE,

    // Logic
    AND,
    OR,
    NOT,

    // String
    CONCAT,             // String concatenation (+ on strings)

    // Control flow
    JUMP,               // [16-bit offset] Unconditional forward jump
    JUMP_BACK,          // [16-bit offset] Unconditional backward jump (loops)
    JUMP_IF_FALSE,      // [16-bit offset] Conditional forward jump
    JUMP_IF_TRUE,       // [16-bit offset] Conditional forward jump

    // Functions
    CALL,               // [8-bit argc] Call function on stack (func below args)
    RETURN,             // Return top of stack (or none)
    RETURN_NONE,        // Return without value

    // Builtins
    CALL_BUILTIN,       // [16-bit builtin_id][8-bit argc] Call builtin function

    // Object/reflection
    GET_MEMBER,         // [16-bit name] Get member of object on stack
    SET_MEMBER,         // [16-bit name] Set member (value on top, object below)
    CALL_METHOD,        // [16-bit name][8-bit argc] Call method (object below args)

    // Constructors
    CONSTRUCT,          // [16-bit class_name][8-bit argc] Construct reflected object

    // Collections
    MAKE_ARRAY,         // [16-bit count] Pop count values, make array
    MAKE_TABLE,         // [16-bit count] Pop count key-value pairs, make table
    INDEX_GET,          // Pop index, pop collection, push element
    INDEX_SET,          // Pop value, pop index, pop collection, set element

    // Iteration
    ITER_INIT,          // Initialize iterator from array/table on stack
    ITER_NEXT,          // [16-bit jump_offset] Advance iterator or jump if done
    ITER_VALUE,         // Push current iterator value

    // Script classes
    NEW_INSTANCE,       // [16-bit class_name][8-bit argc] Create script class instance
    LOAD_SELF,          // Push current 'self' onto stack
    GET_FIELD,          // [16-bit name] Get field from script instance on stack
    SET_FIELD,          // [16-bit name] Set field (value on top, instance below)
    CALL_SCRIPT_METHOD, // [16-bit name][8-bit argc] Call method on script instance

    // Signals
    DECLARE_SIGNAL,     // [16-bit name] Register signal in SignalRegistry
    EMIT_SIGNAL,        // [16-bit name][8-bit argc] Emit signal with args on stack

    // Coroutines
    YIELD,              // Suspend coroutine execution, resume next frame

    // Debug / internal
    LINE,               // [16-bit line] Set current source line for error reporting
};

// ============================================================================
// Constants
// ============================================================================

struct Constant {
    enum class Type : uint8_t {
        NUMBER,
        STRING
    };

    Type type;
    double numberValue = 0.0;
    StringAtom stringAtom = INVALID_ATOM;  // Index into AtomTable

    static Constant Number(double v) {
        Constant c;
        c.type = Type::NUMBER;
        c.numberValue = v;
        return c;
    }

    static Constant String(StringAtom atom) {
        Constant c;
        c.type = Type::STRING;
        c.stringAtom = atom;
        return c;
    }

    KL_BEGIN_FIELDS(Constant)
        KL_FIELD(Constant, type, "Type", 0, 0),
        KL_FIELD(Constant, numberValue, "Number value", 0, 0),
        KL_FIELD(Constant, stringAtom, "String atom", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Constant)
        KL_SMETHOD_AUTO(Constant::Number, "Number"),
        KL_SMETHOD_AUTO(Constant::String, "String")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Constant)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Constant)

};

// ============================================================================
// BytecodeChunk - compiled function or top-level code
// ============================================================================

struct BytecodeChunk {
    std::string name;                        // Function name or "<main>"
    std::vector<uint8_t> code;               // Instruction bytes
    std::vector<Constant> constants;         // Constant pool
    std::vector<StringAtom> localNames;      // Local variable names (by slot index, interned)
    int arity = 0;                           // Number of parameters (0 for top-level)
    int localCount = 0;                      // Total locals (params + declared vars)
    AtomTable* atoms = nullptr;              // Shared atom table (owned by CompiledScript)

    // Source mapping: instruction offset -> source line number
    std::vector<std::pair<size_t, int>> sourceMap;

    // Emit helpers
    size_t Emit(OpCode op) {
        size_t offset = code.size();
        code.push_back(static_cast<uint8_t>(op));
        return offset;
    }

    size_t Emit(OpCode op, uint16_t operand) {
        size_t offset = code.size();
        code.push_back(static_cast<uint8_t>(op));
        code.push_back(static_cast<uint8_t>(operand & 0xFF));
        code.push_back(static_cast<uint8_t>((operand >> 8) & 0xFF));
        return offset;
    }

    size_t Emit(OpCode op, uint16_t operand1, uint8_t operand2) {
        size_t offset = code.size();
        code.push_back(static_cast<uint8_t>(op));
        code.push_back(static_cast<uint8_t>(operand1 & 0xFF));
        code.push_back(static_cast<uint8_t>((operand1 >> 8) & 0xFF));
        code.push_back(static_cast<uint8_t>(operand2));
        return offset;
    }

    size_t EmitByte(OpCode op, uint8_t operand) {
        size_t offset = code.size();
        code.push_back(static_cast<uint8_t>(op));
        code.push_back(operand);
        return offset;
    }

    void PatchJump(size_t offset) {
        // Patch 16-bit jump target at offset+1 to jump to current position
        size_t jump = code.size() - offset - 3; // subtract opcode + 2 operand bytes
        if (jump > 0xFFFF) return; // overflow guard
        code[offset + 1] = static_cast<uint8_t>(jump & 0xFF);
        code[offset + 2] = static_cast<uint8_t>((jump >> 8) & 0xFF);
    }

    uint16_t AddConstant(const Constant& c) {
        constants.push_back(c);
        return static_cast<uint16_t>(constants.size() - 1);
    }

    uint16_t AddStringConstant(const std::string& s) {
        if (!atoms) return AddConstant(Constant::String(INVALID_ATOM));
        StringAtom atom = atoms->Intern(s);
        // Dedup: O(1) atom comparison instead of O(n) string comparison
        for (uint16_t i = 0; i < constants.size(); ++i) {
            if (constants[i].type == Constant::Type::STRING && constants[i].stringAtom == atom)
                return i;
        }
        return AddConstant(Constant::String(atom));
    }

    uint16_t AddNumberConstant(double v) {
        // Dedup number constants
        for (uint16_t i = 0; i < constants.size(); ++i) {
            if (constants[i].type == Constant::Type::NUMBER && constants[i].numberValue == v)
                return i;
        }
        return AddConstant(Constant::Number(v));
    }

    int ResolveLocal(const std::string& name) const {
        if (!atoms) return -1;
        StringAtom atom = atoms->Find(name);
        if (atom == INVALID_ATOM) return -1;
        for (int i = static_cast<int>(localNames.size()) - 1; i >= 0; --i) {
            if (localNames[i] == atom) return i;
        }
        return -1;
    }

    int DeclareLocal(const std::string& name) {
        StringAtom atom = atoms ? atoms->Intern(name) : INVALID_ATOM;
        localNames.push_back(atom);
        localCount = static_cast<int>(localNames.size());
        return localCount - 1;
    }

    void EmitLine(int line) {
        sourceMap.push_back({code.size(), line});
        Emit(OpCode::LINE, static_cast<uint16_t>(line));
    }

    int GetLine(size_t offset) const {
        int line = 0;
        for (auto& [off, ln] : sourceMap) {
            if (off > offset) break;
            line = ln;
        }
        return line;
    }

    inline uint16_t ReadU16(size_t offset) const {
        return static_cast<uint16_t>(code[offset]) |
               (static_cast<uint16_t>(code[offset + 1]) << 8);
    }

};

// ============================================================================
// CompiledScript - all chunks for a single .ks file
// ============================================================================

struct CompiledScript {
    AtomTable atoms;                                             // Shared string intern table
    BytecodeChunk mainChunk;                                     // Top-level init code
    std::unordered_map<std::string, BytecodeChunk> functions;    // Named function chunks
    std::unordered_map<std::string, std::unique_ptr<ScriptClass>> classes; // Script-defined classes

    // After move/copy, atom pointers in chunks are stale. Call this to fix them.
    void FixupAtomPointers() {
        mainChunk.atoms = &atoms;
        for (auto& [name, chunk] : functions) {
            chunk.atoms = &atoms;
        }
    }

    KL_BEGIN_FIELDS(CompiledScript)
        KL_FIELD(CompiledScript, atoms, "Atoms", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(CompiledScript)
        KL_METHOD_AUTO(CompiledScript, FixupAtomPointers, "Fixup atom pointers")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(CompiledScript)
        /* No reflected ctors. */
    KL_END_DESCRIBE(CompiledScript)

};

} // namespace scripting
} // namespace koilo
