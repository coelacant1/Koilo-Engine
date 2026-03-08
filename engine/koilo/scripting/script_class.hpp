// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file script_class.hpp
 * @brief Script-defined class and instance types for KoiloScript.
 *
 * Supports user-defined classes with fields (with defaults) and methods.
 * Instances are prototype-based: fields are per-instance, methods delegate
 * to the class definition.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <koilo/scripting/script_context.hpp>
#include "../registry/reflect_macros.hpp"

namespace koilo {
namespace scripting {

// Forward declarations
struct FunctionDeclNode;
struct BytecodeChunk;

/**
 * @brief Definition of a script-defined class.
 * Stores field defaults and method references (AST or bytecode).
 */
struct ScriptClass {
    std::string name;

    // Field names with default values (ordered for deterministic init)
    struct FieldDef {
        std::string name;
        // Default value stored as index into class's default array

        KL_BEGIN_FIELDS(FieldDef)
            KL_FIELD(FieldDef, name, "Name", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(FieldDef)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(FieldDef)
            /* No reflected ctors. */
        KL_END_DESCRIBE(FieldDef)

    };
    std::vector<FieldDef> fieldDefs;
    std::vector<Value> fieldDefaults;  // Parallel to fieldDefs

    // Method name -> FunctionDeclNode* (AST) for tree-walker
    std::unordered_map<std::string, FunctionDeclNode*> methods;

    // Method name -> BytecodeChunk* for VM execution
    std::unordered_map<std::string, BytecodeChunk*> compiledMethods;

    // Fast field index lookup
    int FindField(const std::string& name) const {
        for (int i = 0; i < static_cast<int>(fieldDefs.size()); ++i) {
            if (fieldDefs[i].name == name) return i;
        }
        return -1;
    }

    bool HasMethod(const std::string& name) const {
        return methods.count(name) > 0 || compiledMethods.count(name) > 0;
    }

    KL_BEGIN_FIELDS(ScriptClass)
        KL_FIELD(ScriptClass, name, "Name", 0, 0),
        KL_FIELD(ScriptClass, fieldDefs, "Field defs", 0, 0),
        KL_FIELD(ScriptClass, fieldDefaults, "Field defaults", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ScriptClass)
        KL_METHOD_AUTO(ScriptClass, FindField, "Find field"),
        KL_METHOD_AUTO(ScriptClass, HasMethod, "Has method")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ScriptClass)
        /* No reflected ctors. */
    KL_END_DESCRIBE(ScriptClass)

};

/**
 * @brief Instance of a script-defined class.
 * Has its own field storage; methods delegate to ScriptClass.
 */
struct ScriptInstance {
    ScriptClass* klass = nullptr;  // Class this instance belongs to
    std::vector<Value> fields;     // Per-instance field values (parallel to class fieldDefs)
    std::unordered_map<std::string, Value> extraFields;  // Dynamic fields set at runtime

    Value GetField(const std::string& name) const;
    void SetField(const std::string& name, const Value& val);
    bool HasField(const std::string& name) const;
    bool HasMethod(const std::string& name) const;

    KL_BEGIN_FIELDS(ScriptInstance)
        KL_FIELD(ScriptInstance, klass, "Klass", 0, 0),
        KL_FIELD(ScriptInstance, fields, "Fields", 0, 0),
        KL_FIELD(ScriptInstance, extraFields, "Extra fields", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ScriptInstance)
        KL_METHOD_AUTO(ScriptInstance, SetField, "Set field"),
        KL_METHOD_AUTO(ScriptInstance, HasField, "Has field"),
        KL_METHOD_AUTO(ScriptInstance, HasMethod, "Has method")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ScriptInstance)
        /* No reflected ctors. */
    KL_END_DESCRIBE(ScriptInstance)

};

} // namespace scripting
} // namespace koilo
