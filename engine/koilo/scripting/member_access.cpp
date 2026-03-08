// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/scripting/value_marshaller.hpp>
#include <koilo/registry/type_registry.hpp>

namespace koilo {
namespace scripting {

Value KoiloScriptEngine::EvaluateMemberAccess(const std::string& path) {
    // Check scope stack first
    Value scopeVal = GetVariable(path);
    if (scopeVal.type != Value::Type::NONE) return scopeVal;
    
    // Check global activeCtx_->variables
    auto gvit = activeCtx_->variables.find(path);
    if (gvit != activeCtx_->variables.end()) return gvit->second;
    
    // Use cached path split
    const SplitPath& sp = SplitMemberPath(path);
    if (sp.fieldChain.empty()) return Value(0.0);
    
    // Check for table member access: "table.key"
    Value baseVal = GetVariable(sp.baseName);
    if (baseVal.type == Value::Type::TABLE && baseVal.tableValue) {
        auto it = baseVal.tableValue->find(sp.fieldChain[0]);
        if (it != baseVal.tableValue->end()) return it->second;
    }
    
    // Check for reflected object field access
    void* selfInstance = nullptr;
    const ClassDesc* selfClassDesc = nullptr;
    if (sp.baseName == "self") {
        ScriptContext& actx = ActiveCtx();
        if (actx.selfObject && actx.selfDesc) {
            selfInstance = actx.selfObject;
            selfClassDesc = actx.selfDesc;
        }
    }
    auto refIt = activeCtx_->reflectedObjects.find(sp.baseName);
    bool hasRef = (refIt != activeCtx_->reflectedObjects.end()) || selfInstance;
    if (hasRef) {
        void* instance = selfInstance ? selfInstance : refIt->second.instance;
        const ClassDesc* classDesc = selfClassDesc ? selfClassDesc : refIt->second.classDesc;
        
        // Navigate through cached field chain
        for (size_t fi = 0; fi < sp.fieldChain.size() && classDesc && instance; ++fi) {
            const FieldDecl* field = ReflectionBridge::FindField(classDesc, sp.fieldChain[fi]);
            if (!field) break;
            
            instance = ReflectionBridge::GetFieldPointer(instance, field);
            if (!instance) break;
            
            if (fi == sp.fieldChain.size() - 1) {
                // Terminal field - read value via FieldKind switch (no typeid chain)
                switch (field->kind) {
                    case FieldKind::Float:  return Value(static_cast<double>(*static_cast<float*>(instance)));
                    case FieldKind::Int:    return Value(static_cast<double>(*static_cast<int*>(instance)));
                    case FieldKind::Double: return Value(*static_cast<double*>(instance));
                    case FieldKind::Bool:   return Value(*static_cast<bool*>(instance) ? 1.0 : 0.0);
                    case FieldKind::UInt8:  return Value(static_cast<double>(*static_cast<uint8_t*>(instance)));
                    case FieldKind::UInt16: return Value(static_cast<double>(*static_cast<uint16_t*>(instance)));
                    case FieldKind::UInt32: return Value(static_cast<double>(*static_cast<uint32_t*>(instance)));
                    case FieldKind::SizeT:  return Value(static_cast<double>(*static_cast<std::size_t*>(instance)));
                    case FieldKind::Complex: {
                        const ClassDesc* fieldClass = ClassForType(*field->type);
                        if (fieldClass) return Value::Object(path);
                        return Value(0.0);
                    }
                }
                return Value(0.0);
            }
            
            // Navigate deeper - use TypeRegistry instead of mangled name parsing
            classDesc = ClassForType(*field->type);
        }
    }
    
    return Value(0.0);
}

const KoiloScriptEngine::SplitPath& KoiloScriptEngine::SplitMemberPath(const std::string& path) {
    auto it = pathCache_.find(path);
    if (it != pathCache_.end()) return it->second;
    
    SplitPath& sp = pathCache_[path];
    size_t dot = path.find('.');
    if (dot == std::string::npos) {
        sp.baseName = path;
        return sp;
    }
    sp.baseName = path.substr(0, dot);
    size_t pos = dot + 1;
    while (pos < path.size()) {
        size_t next = path.find('.', pos);
        if (next == std::string::npos) {
            sp.fieldChain.push_back(path.substr(pos));
            break;
        }
        sp.fieldChain.push_back(path.substr(pos, next - pos));
        pos = next + 1;
    }
    return sp;
}

void KoiloScriptEngine::SetMemberValue(const std::string& path, const Value& value) {
    
    if (value.type == Value::Type::OBJECT) {
        
    }
    
    // Use cached path split
    const SplitPath& sp = SplitMemberPath(path);
    
    if (sp.fieldChain.empty()) {
        // Simple variable - use scope-aware setter
        SetVariable(path, value);
        return;
    }
    
    const std::string& objectId = sp.baseName;
    
    // Check if it's a reflected object first
    if (activeCtx_->reflectedObjects.count(objectId)) {
        // Rebuild property path from fieldChain
        std::string property;
        for (size_t i = 0; i < sp.fieldChain.size(); ++i) {
            if (i > 0) property += '.';
            property += sp.fieldChain[i];
        }
        SetReflectedMemberValue(objectId, sp, value);
        return;
    }
    
    // Check if it's a table variable
    Value tableVal = GetVariable(objectId);
    if (tableVal.type == Value::Type::TABLE && tableVal.tableValue) {
        (*tableVal.tableValue)[sp.fieldChain[0]] = value;
        return;
    }
    if (activeCtx_->variables.count(objectId) && activeCtx_->variables[objectId].type == Value::Type::TABLE 
        && activeCtx_->variables[objectId].tableValue) {
        (*activeCtx_->variables[objectId].tableValue)[sp.fieldChain[0]] = value;
        return;
    }
    
    // Check if object exists (legacy ObjectData)
    if (objects.count(objectId)) {
        std::string property;
        for (size_t i = 0; i < sp.fieldChain.size(); ++i) {
            if (i > 0) property += '.';
            property += sp.fieldChain[i];
        }
        objects[objectId].properties[property] = value;
    } else {
        // Store as variable
        activeCtx_->variables[path] = value;
    }
}

void KoiloScriptEngine::SetReflectedMemberValue(const std::string& objectId, const SplitPath& sp, const Value& value) {
    auto it = activeCtx_->reflectedObjects.find(objectId);
    if (it == activeCtx_->reflectedObjects.end()) {
        SetError(std::string("Reflected object not found: ") + objectId);
        return;
    }
    
    ReflectedObject& refObj = it->second;
    void* currentInstance = refObj.instance;
    const ClassDesc* currentClass = refObj.classDesc;
    
    // fieldChain has 1+ entries. Last entry is terminal field, preceding ones are nested navigation.
    if (sp.fieldChain.size() == 1) {
        // Simple field access (e.g., "intensity")
        const FieldDecl* field = ReflectionBridge::FindField(currentClass, sp.fieldChain[0]);
        if (!field) {
            SetError(std::string("Field not found: ") + sp.fieldChain[0] + " in class " + refObj.classDesc->name);
            return;
        }
        
        if (!MarshalToField(currentInstance, field, value)) {
            SetError(std::string("Failed to set field: ") + sp.fieldChain[0] + " (type: " + field->type->name() + ")");
        }
    } else {
        // Nested field access - navigate through intermediate fields
        for (size_t i = 0; i + 1 < sp.fieldChain.size(); ++i) {
            const FieldDecl* field = ReflectionBridge::FindField(currentClass, sp.fieldChain[i]);
            if (!field) {
                SetError(std::string("Field not found: ") + sp.fieldChain[i] + " in class " + currentClass->name);
                return;
            }
            currentInstance = ReflectionBridge::GetFieldPointer(currentInstance, field);
            if (!currentInstance) {
                SetError(std::string("Failed to get field pointer: ") + sp.fieldChain[i]);
                return;
            }
            currentClass = ClassForType(*field->type);
            if (!currentClass) {
                SetError(std::string("Class not found for field type: ") + field->type->name());
                return;
            }
        }
        
        // Now set the terminal field
        const std::string& termField = sp.fieldChain.back();
        const FieldDecl* nestedField = ReflectionBridge::FindField(currentClass, termField);
        if (!nestedField) {
            SetError(std::string("Nested field not found: ") + termField + " in class " + currentClass->name);
            return;
        }
        
        if (!MarshalToField(currentInstance, nestedField, value)) {
            SetError(std::string("Failed to set nested field: ") + termField + " (type: " + nestedField->type->name() + ")");
        }
    }
}

} // namespace scripting
} // namespace koilo
