// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file reflection_bridge.hpp
 * @brief Bridge between KoiloScript and the reflection system
 * 
 * Provides utilities to instantiate and manipulate C++ objects
 * via the reflection system from KoiloScript.
 * 
 * @date 15/02/2026
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <koilo/registry/global_registry.hpp>
#include <koilo/registry/registry.hpp>
#include <koilo/registry/type_registry.hpp>
#include <string>
#include <cstring>
#include "../registry/reflect_macros.hpp"

namespace koilo {
namespace scripting {

/**
 * @brief Wrapper for a reflected C++ object instance
 */
struct ReflectedObject {
    void* instance;                    ///< Raw pointer to C++ object
    const ClassDesc* classDesc;        ///< Reflection metadata
    std::string id;                    ///< Script identifier
    bool ownsInstance;                 ///< Whether we should delete the instance
    
    ReflectedObject()
        : instance(nullptr), classDesc(nullptr), id(""), ownsInstance(true) {}
    
    ReflectedObject(void* inst, const ClassDesc* desc, const std::string& scriptId, bool owns = true)
        : instance(inst), classDesc(desc), id(scriptId), ownsInstance(owns) {}
    
    ~ReflectedObject() {
        // Cleanup handled by CleanupFrameTemps() or engine destructor
    }

    KL_BEGIN_FIELDS(ReflectedObject)
        KL_FIELD(ReflectedObject, instance, "Instance", 0, 0),
        KL_FIELD(ReflectedObject, classDesc, "Class desc", 0, 0),
        KL_FIELD(ReflectedObject, id, "Id", 0, 0),
        KL_FIELD(ReflectedObject, ownsInstance, "Owns instance", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ReflectedObject)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ReflectedObject)
        KL_CTOR0(ReflectedObject),
        KL_CTOR(ReflectedObject, void *, const ClassDesc *, const std::string &, bool)
    KL_END_DESCRIBE(ReflectedObject)

};

/**
 * @brief Helper class for working with the reflection system
 */
class ReflectionBridge {
public:
    /**
     * @brief Find a class by name in the global registry
     * @param className Name of the class (e.g., "Light", "Vector3D")
     * @return Pointer to ClassDesc, or nullptr if not found
     */
    static const ClassDesc* FindClass(const std::string& className) {
        auto& map = ClassRegistryMap();
        auto it = map.find(className);
        return (it != map.end()) ? it->second : nullptr;
    }
    
    /**
     * @brief Create an instance of a class using its default constructor
     * @param classDesc Class descriptor from reflection
     * @return Pointer to newly created instance, or nullptr on failure
     */
    static void* CreateInstance(const ClassDesc* classDesc) {
        if (!classDesc) return nullptr;
        
        // Look for default constructor (no parameters)
        for (size_t i = 0; i < classDesc->ctor_count; ++i) {
            const ConstructorDesc& ctor = classDesc->ctors[i];
            if (ctor.arg_types.count == 0) {
                // Call constructor with no arguments
                void* args[1] = { nullptr };
                return ctor.invoker(args);
            }
        }
        
        return nullptr;
    }
    
    /**
     * @brief Find a field by name in a class
     * @param classDesc Class descriptor
     * @param fieldName Name of the field
     * @return Pointer to FieldDecl, or nullptr if not found
     */
    static const FieldDecl* FindField(const ClassDesc* classDesc, const std::string& fieldName) {
        if (!classDesc) return nullptr;
        
        auto& fc = FieldCache();
        auto cit = fc.find(classDesc);
        if (cit == fc.end()) {
            // Build cache for this class
            auto& m = fc[classDesc];
            for (size_t i = 0; i < classDesc->fields.count; ++i) {
                const FieldDecl& f = classDesc->fields.data[i];
                if (f.name) m[f.name] = &f;
                if (f.description) m[f.description] = &f;
            }
            cit = fc.find(classDesc);
        }
        auto fit = cit->second.find(fieldName);
        return (fit != cit->second.end()) ? fit->second : nullptr;
    }
    
    /**
     * @brief Find a method by name in a class, optionally filtering by arg count.
     *
     * When multiple overloads share the same arg count, this version picks the
     * first match.  Use the overload that accepts argValues + reflectedObjects
     * for type-based disambiguation.
     *
     * @param classDesc Class descriptor
     * @param methodName Name of the method
     * @param argCount Number of args provided (-1 = don't filter, pick first match)
     * @return Pointer to MethodDesc, or nullptr if not found
     */
    static const MethodDesc* FindMethod(const ClassDesc* classDesc, const std::string& methodName, int argCount = -1) {
        if (!classDesc) return nullptr;
        
        auto& mc = MethodCache();
        auto cit = mc.find(classDesc);
        if (cit == mc.end()) {
            BuildMethodCache(classDesc);
            cit = mc.find(classDesc);
        }
        auto mit = cit->second.find(methodName);
        if (mit == cit->second.end() || mit->second.empty()) return nullptr;
        
        const auto& overloads = mit->second;
        if (argCount < 0 || overloads.size() == 1) return overloads[0];
        
        // Exact arg count match (first match)
        for (const auto* md : overloads) {
            if ((int)md->argc == argCount) return md;
        }
        // Fallback: closest overload with fewer args (default param support)
        const MethodDesc* best = nullptr;
        for (const auto* md : overloads) {
            if ((int)md->argc <= argCount) {
                if (!best || md->argc > best->argc) best = md;
            }
        }
        return best ? best : overloads[0];
    }

    /**
     * @brief Find the best-matching overload using runtime argument type info.
     *
     * When multiple overloads share the same arg count, this inspects each
     * argument's reflected ClassDesc (for OBJECT values) to pick the overload
     * whose parameter types best match the actual arguments.
     *
     * @param classDesc      Class descriptor to search in
     * @param methodName     Name of the method
     * @param argCount       Number of arguments provided
     * @param argObjectDescs Per-argument ClassDesc* (nullptr for non-object args).
     *                       Array must have at least argCount entries.
     * @return Pointer to best-matching MethodDesc, or nullptr if not found
     */
    static const MethodDesc* FindMethodTyped(const ClassDesc* classDesc,
                                              const std::string& methodName,
                                              int argCount,
                                              const ClassDesc* const* argObjectDescs) {
        if (!classDesc) return nullptr;

        auto& mc = MethodCache();
        auto cit = mc.find(classDesc);
        if (cit == mc.end()) {
            BuildMethodCache(classDesc);
            cit = mc.find(classDesc);
        }
        auto mit = cit->second.find(methodName);
        if (mit == cit->second.end() || mit->second.empty()) return nullptr;

        const auto& overloads = mit->second;
        if (overloads.size() == 1) return overloads[0];

        // Collect candidates with matching arg count (stack buffer)
        const MethodDesc* candBuf[16];
        size_t candCount = 0;
        for (const auto* md : overloads) {
            if ((int)md->argc == argCount && candCount < 16) candBuf[candCount++] = md;
        }
        if (candCount == 0) {
            // Fallback: same as FindMethod
            const MethodDesc* best = nullptr;
            for (const auto* md : overloads) {
                if ((int)md->argc <= argCount) {
                    if (!best || md->argc > best->argc) best = md;
                }
            }
            return best ? best : overloads[0];
        }
        if (candCount == 1) return candBuf[0];

        // Score each candidate by type compatibility
        const MethodDesc* bestMatch = nullptr;
        int bestScore = -1;
        for (size_t ci = 0; ci < candCount; ++ci) {
            const MethodDesc* md = candBuf[ci];
            int score = 0;
            bool compatible = true;
            for (int i = 0; i < argCount && compatible; ++i) {
                if (!argObjectDescs[i]) continue;  // Non-object arg, skip
                const std::type_info* expectedType = md->arg_types.data[i];
                if (!expectedType) continue;
                auto expectedInfo = LookupType(*expectedType);
                if (expectedInfo && expectedInfo->classDesc) {
                    if (std::strcmp(argObjectDescs[i]->name, expectedInfo->classDesc->name) == 0) {
                        score += 2;  // Exact type match
                    } else {
                        compatible = false;  // Type mismatch
                    }
                }
            }
            if (compatible && score > bestScore) {
                bestScore = score;
                bestMatch = md;
            }
        }
        return bestMatch ? bestMatch : candBuf[0];
    }

private:
    static void BuildMethodCache(const ClassDesc* classDesc) {
        auto& mc = MethodCache();
        auto& m = mc[classDesc];
        for (size_t i = 0; i < classDesc->methods.count; ++i) {
            const MethodDesc& md = classDesc->methods.data[i];
            if (md.name) {
                m[md.name].push_back(&md);
                std::string fullName(md.name);
                auto pos = fullName.rfind("::");
                if (pos != std::string::npos) {
                    m[fullName.substr(pos + 2)].push_back(&md);
                }
            }
        }
    }

public:
    
    /**
     * @brief Get pointer to a field within an object
     * @param instance Pointer to object instance
     * @param field Field descriptor
     * @return Pointer to the field data
     */
    static void* GetFieldPointer(void* instance, const FieldDecl* field) {
        if (!instance || !field) return nullptr;
        return field->access.get_ptr(instance);
    }
    
    /**
     * @brief Get const pointer to a field within an object
     * @param instance Pointer to object instance
     * @param field Field descriptor
     * @return Const pointer to the field data
     */
    static const void* GetFieldPointer(const void* instance, const FieldDecl* field) {
        if (!instance || !field) return nullptr;
        return field->access.get_cptr(instance);
    }
    
    /**
     * @brief Set a float field value
     * @param instance Pointer to object instance
     * @param field Field descriptor
     * @param value Value to set
     * @return True if successful
     */
    static bool SetFloatField(void* instance, const FieldDecl* field, float value) {
        if (!instance || !field) return false;
        
        // Check type is float
        if (*field->type != typeid(float)) return false;
        
        float* fieldPtr = static_cast<float*>(field->access.get_ptr(instance));
        if (!fieldPtr) return false;
        
        *fieldPtr = value;
        return true;
    }
    
    /**
     * @brief Get a float field value
     * @param instance Pointer to object instance
     * @param field Field descriptor
     * @param outValue Output value
     * @return True if successful
     */
    static bool GetFloatField(const void* instance, const FieldDecl* field, float& outValue) {
        if (!instance || !field) return false;
        
        // Check type is float
        if (*field->type != typeid(float)) return false;
        
        const float* fieldPtr = static_cast<const float*>(field->access.get_cptr(instance));
        if (!fieldPtr) return false;
        
        outValue = *fieldPtr;
        return true;
    }
    
    /**
     * @brief Set an int field value
     * @param instance Pointer to object instance
     * @param field Field descriptor
     * @param value Value to set
     * @return True if successful
     */
    static bool SetIntField(void* instance, const FieldDecl* field, int value) {
        if (!instance || !field) return false;
        
        // Check type is int
        if (*field->type != typeid(int)) return false;
        
        int* fieldPtr = static_cast<int*>(field->access.get_ptr(instance));
        if (!fieldPtr) return false;
        
        *fieldPtr = value;
        return true;
    }
    
    /**
     * @brief Get an int field value
     * @param instance Pointer to object instance
     * @param field Field descriptor
     * @param outValue Output value
     * @return True if successful
     */
    static bool GetIntField(const void* instance, const FieldDecl* field, int& outValue) {
        if (!instance || !field) return false;
        
        // Check type is int
        if (*field->type != typeid(int)) return false;
        
        const int* fieldPtr = static_cast<const int*>(field->access.get_cptr(instance));
        if (!fieldPtr) return false;
        
        outValue = *fieldPtr;
        return true;
    }
    
    /**
     * @brief Set a bool field value
     * @param instance Pointer to object instance
     * @param field Field descriptor
     * @param value Value to set
     * @return True if successful
     */
    static bool SetBoolField(void* instance, const FieldDecl* field, bool value) {
        if (!instance || !field) return false;
        
        // Check type is bool
        if (*field->type != typeid(bool)) return false;
        
        bool* fieldPtr = static_cast<bool*>(field->access.get_ptr(instance));
        if (!fieldPtr) return false;
        
        *fieldPtr = value;
        return true;
    }
    
    /**
     * @brief Get a bool field value
     * @param instance Pointer to object instance
     * @param field Field descriptor
     * @param outValue Output value
     * @return True if successful
     */
    static bool GetBoolField(const void* instance, const FieldDecl* field, bool& outValue) {
        if (!instance || !field) return false;
        
        // Check type is bool
        if (*field->type != typeid(bool)) return false;
        
        const bool* fieldPtr = static_cast<const bool*>(field->access.get_cptr(instance));
        if (!fieldPtr) return false;
        
        outValue = *fieldPtr;
        return true;
    }
    
    /**
     * @brief Invoke a method with no arguments
     * @param instance Pointer to object instance
     * @param method Method descriptor
     * @return Pointer to return value (nullptr for void methods), caller must manage memory
     */
    static void* InvokeMethod(void* instance, const MethodDesc* method) {
        if (!method || !method->invoker) return nullptr;
        if (!instance && !method->is_static) return nullptr;
        
        return method->invoker(instance, nullptr);
    }
    
    /**
     * @brief Invoke a method with arguments
     * @param instance Pointer to object instance
     * @param method Method descriptor
     * @param args Array of pointers to arguments
     * @return Pointer to return value (nullptr for void methods), caller must manage memory
     */
    static void* InvokeMethod(void* instance, const MethodDesc* method, void** args) {
        if (!method || !method->invoker) return nullptr;
        if (!instance && !method->is_static) return nullptr;
        
        return method->invoker(instance, args);
    }
    
    /**
     * @brief Check if a method returns void
     * @param method Method descriptor
     * @return True if method returns void
     */
    static bool IsVoidMethod(const MethodDesc* method) {
        if (!method || !method->ret_type) return true;
        return *method->ret_type == typeid(void);
    }

};

} // namespace scripting
} // namespace koilo
