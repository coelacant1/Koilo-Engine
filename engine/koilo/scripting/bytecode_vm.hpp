// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file bytecode_vm.hpp
 * @brief Stack-based VM for executing KoiloScript bytecode.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <koilo/scripting/bytecode.hpp>
#include <koilo/scripting/script_context.hpp>
#include <koilo/scripting/script_class.hpp>
#include <koilo/scripting/nanboxed_value.hpp>
#include <koilo/kernel/memory/heap_pool.hpp>
#include <string>
#include <vector>

namespace koilo {
namespace scripting {

// Forward declarations
class ExpressionNode;
struct FunctionDeclNode;
class KoiloScriptEngine;
struct CoroutineState;

/**
 * @brief Stack-based VM that executes compiled KoiloScript bytecode.
 */
class BytecodeVM {
public:
    explicit BytecodeVM(KoiloScriptEngine* engine);

    /**
     * @brief Execute a compiled chunk (main or function).
     * @param chunk The bytecode chunk to execute.
     * @param args Arguments (for function calls).
     * @return The return value (or NONE).
     */
    Value Execute(const BytecodeChunk& chunk, const std::vector<Value>& args = {});

    /**
     * @brief Execute a named function from the compiled script.
     * @param script The compiled script containing the function.
     * @param name Function name.
     * @param args Arguments.
     * @return Return value.
     */
    Value CallFunction(const CompiledScript& script, const std::string& name,
                       const std::vector<Value>& args = {});

    bool HasError() const { return hasError_; }
    const std::string& GetError() const { return error_; }

    /** @brief Reclaim heap pools (strings, arrays, tables). Call between frames. */
    void ResetHeap() { stringPool_.Reset(); arrayPool_.Reset(); mapPool_.Reset(); }
    void ClearError() { hasError_ = false; error_.clear(); }
    size_t OwnedStringsCount() const { return stringPool_.Count(); }
    size_t OwnedInstancesCount() const { return ownedInstances_.size(); }

    /// @brief Peak string pool usage since construction.
    size_t StringPoolHighWater() const { return stringPool_.HighWaterMark(); }
    /// @brief Peak array pool usage since construction.
    size_t ArrayPoolHighWater() const { return arrayPool_.HighWaterMark(); }
    /// @brief Peak map pool usage since construction.
    size_t MapPoolHighWater() const { return mapPool_.HighWaterMark(); }

    void SetCompiledScript(const CompiledScript* script) { compiledScript_ = script; }

    // Coroutine support
    bool HasYielded() const { return yielded_; }
    void ClearYielded() { yielded_ = false; }
    void SetSuspendedState(CoroutineState* state) { suspendedState_ = state; }
    Value ResumeCoroutine(CoroutineState& state);

private:
    static constexpr int MAX_STACK = 256;
    static constexpr int MAX_FRAMES = 64;
    static constexpr int MAX_INSTRUCTIONS = 1000000; // per execution

    struct CallFrame {
        const BytecodeChunk* chunk = nullptr;
        size_t ip = 0;                     // Instruction pointer
        int stackBase = 0;                 // Base of this frame's stack
    };

    // State
    KoiloScriptEngine* engine_;
    const CompiledScript* compiledScript_ = nullptr;
    NanBoxedValue stack_[MAX_STACK];
    int stackTop_ = 0;
    CallFrame frames_[MAX_FRAMES];
    int frameCount_ = 0;
    bool hasError_ = false;
    std::string error_;
    int currentLine_ = 0;
    int executeDepth_ = 0; // tracks recursive Execute() calls

    // Coroutine state
    CoroutineState* suspendedState_ = nullptr;  // Set during coroutine execution
    bool yielded_ = false;

    // Iterator state for for-each loops
    struct Iterator {
        std::vector<NanBoxedValue> values;
        size_t index = 0;
    };
    std::vector<Iterator> iterators_;

    // Script instance ownership
    std::vector<std::unique_ptr<ScriptInstance>> ownedInstances_;

    // Reusable buffers to avoid per-call allocations
    std::vector<Value> methodArgs_;
    std::vector<Value> constructArgs_;

    // Heap pools for NaN-boxed composite values (pointer-stable, bounded growth)
    HeapPool<std::string> stringPool_{512};
    HeapPool<HeapArray> arrayPool_{128};
    HeapPool<std::unordered_map<std::string, Value>> mapPool_{64};

    // Core dispatch
    Value Run();

    // Stack operations
    void Push(NanBoxedValue v);
    NanBoxedValue Pop();
    NanBoxedValue& Peek(int distance = 0);

    // Heap allocation helpers (returns pointer-stable references)
    NanBoxedValue AllocString(const std::string& s);
    NanBoxedValue AllocObjectRef(const std::string& name);
    NanBoxedValue AllocFuncRef(const std::string& name);
    HeapArray& AllocArray();
    std::unordered_map<std::string, Value>& AllocMap(); // table storage

    // Value conversion (VM <-> engine boundary)
    Value ToValue(NanBoxedValue nb);
    NanBoxedValue FromValue(const Value& v);

    // Frame management
    CallFrame& CurrentFrame() { return frames_[frameCount_ - 1]; }
    uint8_t ReadByte();
    uint16_t ReadU16();
    OpCode ReadOpCode();

    // Error
    void RuntimeError(const std::string& msg);

    // VM operations
    void ExecuteAdd();
    void ExecuteConcat(NanBoxedValue a, NanBoxedValue b);
    Value CallBuiltin(uint16_t builtinId, int argc);
    Value CallReflectedMethod(const Value& object, const std::string& methodName, int argc);
    Value GetMember(const Value& object, const std::string& memberName);
    void SetMember(const Value& object, const std::string& memberName, const Value& value);
    Value ConstructObject(const std::string& className, int argc);

    // Operator overloading: dispatch __add/__sub/__mul/__div on reflected objects
    bool TryOperatorOverload(NanBoxedValue& a, NanBoxedValue& b, const char* opMethod, NanBoxedValue& out);
};

} // namespace scripting
} // namespace koilo
