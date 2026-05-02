// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/scripting/value_marshaller.hpp>
#include <koilo/scripting/string_atom.hpp>
#include <koilo/registry/type_registry.hpp>
#include <cstring>
#include <cstdio>

namespace koilo {
namespace scripting {

// Method dispatch cache: avoids repeated FindMethodTyped lookups for the same
// (ClassDesc*, methodName, argc) combination.
namespace {
struct MethodCacheKey {
    const ClassDesc* classDesc;
    std::string methodName;
    int argc;
    bool operator==(const MethodCacheKey& o) const {
        return classDesc == o.classDesc && argc == o.argc && methodName == o.methodName;
    }
};
struct MethodCacheHash {
    size_t operator()(const MethodCacheKey& k) const {
        size_t h = std::hash<const void*>()(k.classDesc);
        h ^= std::hash<std::string>()(k.methodName) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>()(k.argc) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        return h;
    }
};
static std::unordered_map<MethodCacheKey, const MethodDesc*, MethodCacheHash> s_methodCache;

// Fast atom-keyed cache: per-call lookup is a single 64-bit integer hash
// over POD components, avoiding the repeated _Hash_bytes over the method
// name string that dominated reflection dispatch in profiling.
// Keyed on AtomTable* + atom id so entries from different scripts/atom
// tables don't collide.
struct MethodAtomKey {
    const ClassDesc* classDesc;
    const AtomTable* atomTable;
    uint32_t atom;
    int argc;
    bool operator==(const MethodAtomKey& o) const {
        return classDesc == o.classDesc && atomTable == o.atomTable
            && atom == o.atom && argc == o.argc;
    }
};
struct MethodAtomKeyHash {
    size_t operator()(const MethodAtomKey& k) const {
        // Mix four POD inputs with splitmix64-style avalanche.  The ClassDesc
        // pointer dominates entropy; atom + argc disambiguate overloads.
        uint64_t h = reinterpret_cast<uint64_t>(k.classDesc) * 0x9E3779B97F4A7C15ULL;
        h ^= reinterpret_cast<uint64_t>(k.atomTable) + 0x94D049BB133111EBULL + (h << 13) + (h >> 17);
        h ^= (uint64_t)k.atom + 0xBF58476D1CE4E5B9ULL + (h << 7);
        h ^= (uint64_t)k.argc * 0x94D049BB133111EBULL;
        h ^= h >> 31;
        return (size_t)h;
    }
};
static std::unordered_map<MethodAtomKey, const MethodDesc*, MethodAtomKeyHash> s_methodAtomCache;
} // anon

Value KoiloScriptEngine::CallReflectedMethod(const std::string& path, const std::vector<Value>& args) {
    // Parse "objectName.MethodName" or "objectName.field.subfield.MethodName"
    size_t lastDot = path.rfind('.');
    if (lastDot == std::string::npos) {
        SetError("Invalid method call path: " + path);
        return Value();
    }
    
    std::string objectPath = path.substr(0, lastDot);
    std::string methodName = path.substr(lastDot + 1);
    
    // Check if the base is a script array or table variable (not a C++ reflected object)
    Value baseVal = GetVariable(objectPath);
    if (baseVal.type == Value::Type::NONE && activeCtx_->variables.count(objectPath)) {
        baseVal = activeCtx_->variables[objectPath];
    }
    
    // Array methods: arr.push(val), arr.pop(), arr.remove(idx), arr.contains(val), arr.length
    if (baseVal.type == Value::Type::ARRAY) {
        // Find the mutable Value* for the array in scope or globals
        Value* arrPtr = nullptr;
        for (int i = (int)activeCtx_->scopeStack.size() - 1; i >= 0; i--) {
            auto it = activeCtx_->scopeStack[i].find(objectPath);
            if (it != activeCtx_->scopeStack[i].end() && it->second.type == Value::Type::ARRAY) {
                arrPtr = &it->second;
                break;
            }
        }
        if (!arrPtr && activeCtx_->variables.count(objectPath) && activeCtx_->variables[objectPath].type == Value::Type::ARRAY) {
            arrPtr = &activeCtx_->variables[objectPath];
        }
        
        if (arrPtr) return DispatchArrayMethod(arrPtr, methodName, args);
        return Value();
    }
    
    // Table methods: tbl.keys(), tbl.has(key)
    if (baseVal.type == Value::Type::TABLE && baseVal.tableValue) {
        return DispatchTableMethod(baseVal.tableValue, methodName, args);
    }
    
    // Navigate to the target object (handles nested paths like "obj.field.subfield")
    void* objectInstance = nullptr;
    const ClassDesc* objectClass = nullptr;
    
    // Check if it's a simple object reference (no dots in objectPath)
    if (objectPath.find('.') == std::string::npos) {
        // Simple: "objectName"
        // DEBUG: Print what we're looking for
        
        
        
        if (activeCtx_->variables.count(objectPath)) {
            
            
        }
        
        // First check for 'self' keyword
        if (objectPath == "self") {
            ScriptContext& actx = ActiveCtx();
            if (actx.selfObject && actx.selfDesc) {
                objectInstance = actx.selfObject;
                objectClass = actx.selfDesc;
            } else {
                SetError("'self' used outside of object context");
                return Value();
            }
        }
        // Check if it's a direct reflected object
        else if (activeCtx_->reflectedObjects.count(objectPath)) {
            
            auto& refObj = activeCtx_->reflectedObjects[objectPath];
            objectInstance = refObj.instance; // may be null for class-level static calls
            objectClass = refObj.classDesc;
        }
        // Check if it's a variable holding an object reference
        else if (activeCtx_->variables.count(objectPath) && activeCtx_->variables[objectPath].type == Value::Type::OBJECT) {
            
            std::string actualObjectName = activeCtx_->variables[objectPath].objectName;
            
            if (!activeCtx_->reflectedObjects.count(actualObjectName)) {
                SetError("Referenced object not found: " + actualObjectName);
                return Value();
            }
            auto& refObj = activeCtx_->reflectedObjects[actualObjectName];
            if (!refObj.instance) {
                SetError("Object no longer valid: " + actualObjectName);
                return Value();
            }
            objectInstance = refObj.instance;
            objectClass = refObj.classDesc;
        }
        else {
            // Check if this is a known module global that isn't loaded
            static const char* MODULE_GLOBALS[] = {"physics", "audio", "ai", "particles", "effects", "ui"};
            for (auto* name : MODULE_GLOBALS) {
                if (objectPath == name && !moduleLoader_.HasModule(name)) {
                    SetError("Module '" + objectPath + "' not loaded. Register it with GetModuleLoader().Register() or add the module .so/.bin");
                    return Value();
                }
            }
            
            SetError("Object not found: " + objectPath);
            return Value();
        }
    } else {
        // Nested: "objectName.field.subfield"
        // Parse: objectName is before first dot, rest is the field path
        size_t firstDot = objectPath.find('.');
        std::string baseObjectName = objectPath.substr(0, firstDot);
        std::string fieldPath = objectPath.substr(firstDot + 1);
        
        // Get base object (check reflected objects and activeCtx_->variables)
        if (activeCtx_->reflectedObjects.count(baseObjectName)) {
            auto& refObj = activeCtx_->reflectedObjects[baseObjectName];
            if (!refObj.instance) {
                SetError("Object no longer valid: " + baseObjectName);
                return Value();
            }
            objectInstance = refObj.instance;
            objectClass = refObj.classDesc;
        }
        else if (activeCtx_->variables.count(baseObjectName) && activeCtx_->variables[baseObjectName].type == Value::Type::OBJECT) {
            std::string actualObjectName = activeCtx_->variables[baseObjectName].objectName;
            if (!activeCtx_->reflectedObjects.count(actualObjectName)) {
                SetError("Referenced object not found: " + actualObjectName);
                return Value();
            }
            auto& refObj = activeCtx_->reflectedObjects[actualObjectName];
            if (!refObj.instance) {
                SetError("Object no longer valid: " + actualObjectName);
                return Value();
            }
            objectInstance = refObj.instance;
            objectClass = refObj.classDesc;
        }
        else {
            SetError("Object not found: " + baseObjectName);
            return Value();
        }
        
        // Navigate through nested fields
        std::string remainingPath = fieldPath;
        while (!remainingPath.empty()) {
            size_t dot = remainingPath.find('.');
            std::string fieldName = (dot == std::string::npos) ? remainingPath : remainingPath.substr(0, dot);
            
            const FieldDecl* field = ReflectionBridge::FindField(objectClass, fieldName);
            if (!field) {
                SetError("Field not found: " + fieldName + " in class " + std::string(objectClass->name));
                return Value();
            }
            
            // Get pointer to this field
            objectInstance = ReflectionBridge::GetFieldPointer(objectInstance, field);
            if (!objectInstance) {
                SetError("Failed to access field: " + fieldName);
                return Value();
            }
            
            // Update class descriptor via TypeRegistry (portable, no name mangling)
            objectClass = ClassForType(*field->type);
            if (!objectClass) {
                SetError("Cannot find class for field type: " + std::string(field->type->name()));
                return Value();
            }
            
            // Move to next part of path
            if (dot == std::string::npos) {
                break;
            }
            remainingPath = remainingPath.substr(dot + 1);
        }
    }
    
    if (!objectClass) {
        SetError("Invalid object for method call: " + objectPath);
        return Value();
    }
    
    // Find the method (with type-aware overload resolution)
    const ClassDesc* argDescsBuf[8] = {};
    const ClassDesc** argDescsPtr = argDescsBuf;
    std::vector<const ClassDesc*> argDescsHeap;
    if (args.size() > 8) {
        argDescsHeap.resize(args.size(), nullptr);
        argDescsPtr = argDescsHeap.data();
    }
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i].type == Value::Type::OBJECT) {
            auto it = activeCtx_->reflectedObjects.find(args[i].objectName);
            if (it != activeCtx_->reflectedObjects.end())
                argDescsPtr[i] = it->second.classDesc;
        }
    }
    const MethodDesc* method = ReflectionBridge::FindMethodTyped(
        objectClass, methodName, (int)args.size(), argDescsPtr);
    if (!method) {
        SetError("Method not found: " + methodName + " in class " + std::string(objectClass->name));
        return Value();
    }
    
    void* invokeInstance = (method->is_static) ? nullptr : objectInstance;
    
    if (args.size() < (size_t)method->argc) {
        SetError("Method " + methodName + " expects " + std::to_string(method->argc) +
                 " arg(s) but got " + std::to_string(args.size()));
        return Value();
    }
    
    // Marshal arguments using stack buffer when possible
    static ArgMarshaller marshaller;
    marshaller.Clear();
    void* argPtrsBuf2[8] = {};
    void** argPtrsPtr = argPtrsBuf2;
    std::vector<void*> argPtrsHeap;
    size_t argCount = (size_t)method->argc;
    if (argCount > 0) {
        if (argCount > 8) {
            argPtrsHeap.resize(argCount);
            argPtrsPtr = argPtrsHeap.data();
        }
        for (size_t i = 0; i < argCount; ++i) {
            argPtrsPtr[i] = marshaller.Marshal(args[i], method->arg_types.data[i],
                                                activeCtx_->reflectedObjects);
            if (!argPtrsPtr[i]) {
                SetError("Failed to marshal args for method: " + methodName);
                return Value();
            }
        }
    }
    
    void* result = ReflectionBridge::InvokeMethod(invokeInstance, method, argCount > 0 ? argPtrsPtr : nullptr);
    
    // Handle return value
    if (ReflectionBridge::IsVoidMethod(method)) {
        return Value();  // Void method
    }
    
    if (!method->ret_type) {
        return Value();  // No return type info
    }
    
    // Handle primitive return types (and std::string)
    Value retVal = MarshalReturnValue(result, method->ret_type, method->ret_owns);
    if (retVal.type != Value::Type::NONE) {
        return retVal;
    }
    
    // Handle complex types (classes) - use compile-time ownership annotations
    bool isPointerReturn = method->ret_is_pointer;
    const ClassDesc* retClass = ClassForType(*method->ret_type);
    
    if (retClass && result) {
        char nameBuf[64];
        std::snprintf(nameBuf, sizeof(nameBuf), "_ret_%u", activeCtx_->tempCounter++);
        std::string tempName(nameBuf);
        
        // Destroy old owned instance before overwriting (prevents leak when
        // persistent-promoted temps are reused via tempCounter recycling)
        auto existIt = activeCtx_->reflectedObjects.find(tempName);
        if (existIt != activeCtx_->reflectedObjects.end() &&
            existIt->second.ownsInstance && existIt->second.instance) {
            if (existIt->second.classDesc && existIt->second.classDesc->destroy) {
                existIt->second.classDesc->destroy(existIt->second.instance);
            } else {
                ::operator delete(existIt->second.instance);
            }
        }
        
        if (isPointerReturn) {
            activeCtx_->reflectedObjects[tempName] = ReflectedObject(result, retClass, tempName, false);
        } else {
            void* newInstance = ::operator new(method->ret_size);
            if (retClass->copy_construct) {
                retClass->copy_construct(newInstance, result);
            } else {
                std::memcpy(newInstance, result, method->ret_size);
            }
            activeCtx_->reflectedObjects[tempName] = ReflectedObject(newInstance, retClass, tempName, true);
            // Free the BoxReturn allocation now that we've copied it
            if (method->ret_owns) {
                if (retClass->destroy) {
                    retClass->destroy(result);
                } else {
                    ::operator delete(result);
                }
            }
        }
        activeCtx_->frameTempObjects.push_back(std::move(tempName));
        
        return Value::Object(nameBuf);
    }
    
    return Value();
}

// --- Built-in type method dispatch ---

Value KoiloScriptEngine::DispatchArrayMethod(Value* arrPtr, const std::string& methodName, const std::vector<Value>& args) {
    if (methodName == "push" && !args.empty()) {
        arrPtr->arrayValue.push_back(args[0]);
        return Value((double)arrPtr->arrayValue.size());
    }
    if (methodName == "pop" && !arrPtr->arrayValue.empty()) {
        Value last = arrPtr->arrayValue.back();
        arrPtr->arrayValue.pop_back();
        return last;
    }
    if (methodName == "remove" && !args.empty() && args[0].type == Value::Type::NUMBER) {
        int idx = (int)args[0].numberValue;
        if (idx >= 0 && idx < (int)arrPtr->arrayValue.size()) {
            Value removed = arrPtr->arrayValue[idx];
            arrPtr->arrayValue.erase(arrPtr->arrayValue.begin() + idx);
            return removed;
        }
        return Value();
    }
    if (methodName == "contains" && !args.empty()) {
        for (const Value& elem : arrPtr->arrayValue) {
            if (elem.type == args[0].type) {
                if (elem.type == Value::Type::NUMBER && elem.numberValue == args[0].numberValue) return Value(true);
                if (elem.type == Value::Type::STRING && elem.stringValue == args[0].stringValue) return Value(true);
                if (elem.type == Value::Type::BOOL && elem.boolValue == args[0].boolValue) return Value(true);
            }
        }
        return Value(false);
    }
    if (methodName == "length") {
        return Value((double)arrPtr->arrayValue.size());
    }
    return Value();
}

Value KoiloScriptEngine::DispatchTableMethod(std::unordered_map<std::string, Value>* table, const std::string& methodName, const std::vector<Value>& args) {
    if (methodName == "keys") {
        std::vector<Value> keys;
        keys.reserve(table->size());
        for (auto& kv : *table) {
            keys.push_back(Value(kv.first));
        }
        return Value(keys);
    }
    if (methodName == "has" && !args.empty() && args[0].type == Value::Type::STRING) {
        return Value(table->count(args[0].stringValue) > 0);
    }
    if (methodName == "size") {
        return Value((double)table->size());
    }
    return Value();
}

Value KoiloScriptEngine::CallReflectedMethodDirect(
    void* instance, const ClassDesc* classDesc,
    const std::string& methodName, int argc,
    const std::vector<Value>& args)
{
    if (!classDesc) {
        SetError("CallReflectedMethodDirect: null class");
        return Value();
    }

    // Check method dispatch cache first
    MethodCacheKey cacheKey{classDesc, methodName, argc};
    const MethodDesc* method = nullptr;
    auto cacheIt = s_methodCache.find(cacheKey);
    if (cacheIt != s_methodCache.end()) {
        method = cacheIt->second;
    } else {
        // Build per-arg ClassDesc array for overload resolution
        const ClassDesc* argDescsBuf[8] = {};
        const ClassDesc** argDescs = argDescsBuf;
        std::vector<const ClassDesc*> argDescsHeap;
        if ((size_t)argc > 8) {
            argDescsHeap.resize(argc, nullptr);
            argDescs = argDescsHeap.data();
        }
        for (int i = 0; i < argc; ++i) {
            if (args[i].type == Value::Type::OBJECT) {
                auto it = activeCtx_->reflectedObjects.find(args[i].objectName);
                if (it != activeCtx_->reflectedObjects.end())
                    argDescs[i] = it->second.classDesc;
            }
        }

        method = ReflectionBridge::FindMethodTyped(
            classDesc, methodName, argc, argDescs);
        if (method) {
            s_methodCache[cacheKey] = method;
        }
    }
    if (!method) {
        SetError("Method not found: " + methodName + " in class " + std::string(classDesc->name));
        return Value();
    }

    return InvokeResolvedReflectedMethod(instance, classDesc, method, methodName, argc, args);
}

Value KoiloScriptEngine::CallReflectedMethodDirectAtom(
    void* instance, const ClassDesc* classDesc,
    const AtomTable* atomTable, uint32_t methodAtom,
    const std::string& methodName, int argc,
    const std::vector<Value>& args)
{
    if (!classDesc) {
        SetError("CallReflectedMethodDirectAtom: null class");
        return Value();
    }

    const MethodDesc* method = nullptr;
    // Atom-keyed inline cache: pure-POD key, no string hashing on hot path.
    // Falls back to the legacy string-keyed cache and FindMethodTyped on miss.
    if (atomTable && methodAtom != INVALID_ATOM) {
        MethodAtomKey akey{classDesc, atomTable, methodAtom, argc};
        auto ait = s_methodAtomCache.find(akey);
        if (ait != s_methodAtomCache.end()) {
            method = ait->second;
        } else {
            // Cold path: resolve using the canonical (string-keyed) cache so
            // both caches converge on the same MethodDesc, then memoize the
            // atom-keyed entry for future hot calls.
            MethodCacheKey skey{classDesc, methodName, argc};
            auto sit = s_methodCache.find(skey);
            if (sit != s_methodCache.end()) {
                method = sit->second;
            } else {
                const ClassDesc* argDescsBuf[8] = {};
                const ClassDesc** argDescs = argDescsBuf;
                std::vector<const ClassDesc*> argDescsHeap;
                if ((size_t)argc > 8) {
                    argDescsHeap.resize(argc, nullptr);
                    argDescs = argDescsHeap.data();
                }
                for (int i = 0; i < argc; ++i) {
                    if (args[i].type == Value::Type::OBJECT) {
                        auto it = activeCtx_->reflectedObjects.find(args[i].objectName);
                        if (it != activeCtx_->reflectedObjects.end())
                            argDescs[i] = it->second.classDesc;
                    }
                }
                method = ReflectionBridge::FindMethodTyped(classDesc, methodName, argc, argDescs);
                if (method) s_methodCache[skey] = method;
            }
            if (method) s_methodAtomCache[akey] = method;
        }
    } else {
        // No atom available -> fall back to the slow string path entirely.
        return CallReflectedMethodDirect(instance, classDesc, methodName, argc, args);
    }

    if (!method) {
        SetError("Method not found: " + methodName + " in class " + std::string(classDesc->name));
        return Value();
    }

    return InvokeResolvedReflectedMethod(instance, classDesc, method, methodName, argc, args);
}

Value KoiloScriptEngine::InvokeResolvedReflectedMethod(
    void* instance, const ClassDesc* classDesc,
    const MethodDesc* method,
    const std::string& methodName, int argc,
    const std::vector<Value>& args)
{
    (void)classDesc;
    void* invokeInstance = method->is_static ? nullptr : instance;
    if (!invokeInstance && !method->is_static) {
        SetError("Cannot call instance method " + methodName + " on class without instance");
        return Value();
    }

    if ((size_t)argc < (size_t)method->argc) {
        SetError("Method " + methodName + " expects " + std::to_string(method->argc) +
                 " arg(s) but got " + std::to_string(argc));
        return Value();
    }

    // Marshal arguments using pre-allocated buffers
    static ArgMarshaller marshaller;
    marshaller.Clear();
    void* argPtrsBuf[8] = {};
    void** argPtrsPtr = argPtrsBuf;
    std::vector<void*> argPtrsHeap;
    size_t argCount = (size_t)method->argc;

    if (argCount > 0) {
        if (argCount > 8) {
            argPtrsHeap.resize(argCount);
            argPtrsPtr = argPtrsHeap.data();
        }
        for (size_t i = 0; i < argCount; ++i) {
            argPtrsPtr[i] = marshaller.Marshal(args[i], method->arg_types.data[i],
                                                activeCtx_->reflectedObjects);
            if (!argPtrsPtr[i]) {
                SetError("Failed to marshal arg " + std::to_string(i) + " for method: " + methodName);
                return Value();
            }
        }
    }

    void* result = ReflectionBridge::InvokeMethod(invokeInstance, method, argCount > 0 ? argPtrsPtr : nullptr);

    if (ReflectionBridge::IsVoidMethod(method)) return Value();
    if (!method->ret_type) return Value();

    Value retVal = MarshalReturnValue(result, method->ret_type, method->ret_owns);
    if (retVal.type != Value::Type::NONE) return retVal;

    bool isPointerReturn = method->ret_is_pointer;
    const ClassDesc* retClass = ClassForType(*method->ret_type);

    if (retClass && result) {
        char nameBuf[64];
        std::snprintf(nameBuf, sizeof(nameBuf), "_ret_%u", activeCtx_->tempCounter++);
        std::string tempName(nameBuf);

        // Destroy old owned instance before overwriting (prevents leak when
        // persistent-promoted temps are reused via tempCounter recycling)
        auto existIt = activeCtx_->reflectedObjects.find(tempName);
        if (existIt != activeCtx_->reflectedObjects.end() &&
            existIt->second.ownsInstance && existIt->second.instance) {
            if (existIt->second.classDesc && existIt->second.classDesc->destroy) {
                existIt->second.classDesc->destroy(existIt->second.instance);
            } else {
                ::operator delete(existIt->second.instance);
            }
        }

        if (isPointerReturn) {
            activeCtx_->reflectedObjects[tempName] = ReflectedObject(result, retClass, tempName, false);
        } else {
            void* newInstance = ::operator new(method->ret_size);
            if (retClass->copy_construct) {
                retClass->copy_construct(newInstance, result);
            } else {
                std::memcpy(newInstance, result, method->ret_size);
            }
            activeCtx_->reflectedObjects[tempName] = ReflectedObject(newInstance, retClass, tempName, true);
            // Free the BoxReturn allocation now that we've copied it
            if (method->ret_owns) {
                if (retClass->destroy) {
                    retClass->destroy(result);
                } else {
                    ::operator delete(result);
                }
            }
        }
        activeCtx_->frameTempObjects.push_back(std::move(tempName));
        return Value::Object(nameBuf);
    }

    return Value();
}

} // namespace scripting
} // namespace koilo
