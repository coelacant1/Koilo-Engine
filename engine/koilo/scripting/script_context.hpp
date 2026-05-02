// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <koilo/scripting/koiloscript_ast.hpp>
#include <koilo/scripting/reflection_bridge.hpp>
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <memory>
#include "../registry/reflect_macros.hpp"

namespace koilo {
namespace scripting {

// Forward declaration for script-side class instances
struct ScriptClass;
struct ScriptInstance;
class AtomTable;

/**
 * @brief Runtime value type for KoiloScript execution
 */
struct Value {
    enum class Type {
        NUMBER,
        STRING,
        ARRAY,
        OBJECT,           // Reference to a reflected C++ object
        BOOL,
        TABLE,
        FUNCTION,         // Reference to a user-defined function
        SCRIPT_INSTANCE,  // Reference to a script-defined class instance
        NONE              // null
    };
    
    Type type = Type::NONE;
    double numberValue = 0.0;
    std::string stringValue;
    std::vector<Value> arrayValue;  // Mixed-type array
    std::string objectName;  // Name of reflected object (for OBJECT type)
    bool boolValue = false;
    std::unordered_map<std::string, Value>* tableValue = nullptr; // Owned externally
    FunctionDeclNode* functionRef = nullptr;  // For FUNCTION type
    ScriptInstance* instanceRef = nullptr;    // For SCRIPT_INSTANCE type
    
    Value() : type(Type::NONE) {}
    Value(double n) : type(Type::NUMBER), numberValue(n) {}
    Value(const std::string& s) : type(Type::STRING), stringValue(s) {}
    Value(const std::vector<Value>& arr) : type(Type::ARRAY), arrayValue(arr) {}
    Value(bool b) : type(Type::BOOL), boolValue(b) {}
    
    // Convenience: construct array from vector<double>
    static Value FromDoubles(const std::vector<double>& nums) {
        Value v;
        v.type = Type::ARRAY;
        v.arrayValue.reserve(nums.size());
        for (double n : nums) v.arrayValue.push_back(Value(n));
        return v;
    }
    
    // Convenience: get double from array element
    double ArrayNum(size_t i) const {
        if (i < arrayValue.size() && arrayValue[i].type == Type::NUMBER)
            return arrayValue[i].numberValue;
        return 0.0;
    }
    
    // Create object reference value
    static Value Object(const std::string& name) {
        Value v;
        v.type = Type::OBJECT;
        v.objectName = name;
        return v;
    }
    
    // Create script instance reference
    static Value Instance(ScriptInstance* inst) {
        Value v;
        v.type = Type::SCRIPT_INSTANCE;
        v.instanceRef = inst;
        return v;
    }
    
    // Truthiness check
    bool IsTruthy() const {
        switch (type) {
            case Type::NUMBER: return numberValue != 0.0;
            case Type::BOOL: return boolValue;
            case Type::STRING: return !stringValue.empty();
            case Type::ARRAY: return !arrayValue.empty();
            case Type::NONE: return false;
            default: return true;
        }
    }

    KL_BEGIN_FIELDS(Value)
        KL_FIELD(Value, numberValue, "Number value", 0, 0),
        KL_FIELD(Value, stringValue, "String value", 0, 0),
        KL_FIELD(Value, arrayValue, "Array value", 0, 0),
        KL_FIELD(Value, objectName, "Object name", 0, 0),
        KL_FIELD(Value, boolValue, "Bool value", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Value)
        KL_METHOD_AUTO(Value, ArrayNum, "Array num"),
        KL_METHOD_AUTO(Value, IsTruthy, "Is truthy")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Value)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Value)

};

/**
 * @brief Isolated execution context for a single script.
 *
 * Each ScriptContext holds its own variables, scope stack, reflected objects,
 * states, controls, and temp-object lifecycle. KoiloScriptEngine owns one or
 * more of these and switches the "active" context during execution.
 *
 * In the single-context model the engine creates one implicit primary context.
 * In multi-context mode each scene object with a SetScript() call gets its
 * own context.
 */
struct ScriptContext {
    // Pre-reserve buckets so steady-state inserts of recycled `_ret_N` /
    // `_ctor_*` temp names don't trigger rehashes (which would invalidate
    // cached ReflectedObject* pointers used by the receiver inline cache).
    ScriptContext() {
        reflectedObjects.reserve(256);
        variables.reserve(128);
    }

    // --- identity ---
    std::string objectId;   ///< Logical name ("face", "tail", or "" for primary)

    // --- parsed representation ---
    ScriptAST ast;

    // --- runtime variable storage ---
    std::unordered_map<std::string, Value> variables;
    std::vector<std::unordered_map<std::string, Value>> scopeStack;

    /// Bumped on events that may invalidate `Value*` pointers cached from
    /// `variables` lookups (erase, clear).  Combined with a `bucket_count()`
    /// snapshot in the cache validator this catches rehashes too.
    uint64_t variablesGen = 0;

    // -- LOAD_GLOBAL atom cache --------------------------------
    // Direct-mapped cache from StringAtom (per-CompiledScript AtomTable id)
    // to a stable pointer into `variables`. The bytecode VM hits this on
    // every LOAD_GLOBAL/STORE_GLOBAL to avoid re-hashing the variable name.
    //
    // Validation invariants:
    //   - atomTable identity guards against the cache being reused after
    //     the active CompiledScript switches (atom IDs are only meaningful
    //     within their owning AtomTable).
    //   - bucketCount snapshot detects rehashes of `variables`, which would
    //     invalidate any iterator/pointer the slot is holding.
    // `variables` is never erased from inside the script engine, so the
    // bucket_count() check is sufficient to detect the only invalidation
    // event we care about (insert-induced rehash).
    struct GlobalCacheSlot {
        Value* ptr = nullptr;
        std::size_t bucketCount = 0;
        const AtomTable* atomTable = nullptr;
    };
    std::vector<GlobalCacheSlot> globalCache;

    // -- Built-in time/dt/fps slot cache -----------------------
    // ExecuteUpdate writes "time", "deltaTime", "fps" every frame. These
    // are rvalue-string operator[] hits - each does a fresh hash + lookup.
    // Cache stable Value* pointers, validated by bucket_count() so rehashes
    // (script inserts more globals) safely invalidate.
    Value* builtinTimeSlot = nullptr;
    Value* builtinDeltaTimeSlot = nullptr;
    Value* builtinFpsSlot = nullptr;
    std::size_t builtinSlotsBucketCount = 0;

    // --- reflected C++ objects visible to this context ---
    std::unordered_map<std::string, ReflectedObject> reflectedObjects;

    /// Bumped on every event that may invalidate `ReflectedObject*` pointers
    /// returned from `reflectedObjects` lookups (erase, clear, rehash).
    /// Lookup-side caches (e.g. the BytecodeVM receiver inline cache) compare
    /// this against a snapshot to validate cached pointers without re-hashing
    /// the string key.  Steady-state hot loops insert into existing nodes
    /// (temp names recycle via `tempCounter`), so this counter does not bump
    /// per frame.
    uint64_t reflectedObjectsGen = 0;

    // --- state machine ---
    std::map<std::string, StateNode*> states;
    std::string currentState;

    // --- input bindings ---
    std::map<std::string, const ControlNode*> controlMap;

    // --- object lifecycle ---
    std::vector<std::string> frameTempObjects;
    std::set<std::string> persistentObjects;
    std::vector<std::unique_ptr<std::unordered_map<std::string, Value>>> ownedTables;
    int tempCounter = 0;  ///< Context-scoped counter for temp object names

    // --- call-stack bookkeeping ---
    int callDepth = 0;
    bool returnFlag = false;
    Value returnValue;

    // --- self binding (multi-context: bound C++ instance) ---
    void* selfObject = nullptr;
    const ClassDesc* selfDesc = nullptr;

    // --- frame time ---
    float currentTime = 0.0f;

    // --- cached function pointers (avoid per-frame lookup) ---
    FunctionDeclNode* cachedUpdateFunc = nullptr;
    bool updateFuncCached = false;

    KL_BEGIN_FIELDS(ScriptContext)
        KL_FIELD(ScriptContext, scopeStack, "Scope stack", 0, 0),
        KL_FIELD(ScriptContext, currentState, "Current state", 0, 0),
        KL_FIELD(ScriptContext, persistentObjects, "Persistent objects", 0, 0),
        KL_FIELD(ScriptContext, ownedTables, "Owned tables", 0, 0),
        KL_FIELD(ScriptContext, returnValue, "Return value", 0, 0),
        KL_FIELD(ScriptContext, currentTime, "Current time", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ScriptContext)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ScriptContext)
        /* No reflected ctors. */
    KL_END_DESCRIBE(ScriptContext)

};

} // namespace scripting
} // namespace koilo
