// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file bytecode_vm.cpp
 * @brief Stack-based virtual machine for executing KoiloScript bytecode.
 *
 * Uses NaN-boxed 8-byte values internally for cache efficiency.
 * Converts to/from Value at engine API boundaries.
 *
 * @date 23/02/2026
 * @version 2.0
 * @author Coela
 */

#include <koilo/scripting/bytecode_vm.hpp>
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/scripting/reflection_bridge.hpp>
#include <koilo/scripting/value_marshaller.hpp>
#include <koilo/scripting/coroutine.hpp>
#include <koilo/registry/type_registry.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>

namespace koilo {
namespace scripting {

// ============================================================================
// Construction
// ============================================================================

BytecodeVM::BytecodeVM(KoiloScriptEngine* engine)
    : engine_(engine) {}

// Helper: resolve string constant from atom table
inline const std::string& ResolveString(const BytecodeChunk* chunk, uint16_t idx) {
    return chunk->atoms->GetString(chunk->constants[idx].stringAtom);
}

// Type name helper for error messages
static const char* NbTypeName(const NanBoxedValue& v) {
    if (v.IsNumber()) return "Number";
    if (v.IsString()) return "String";
    if (v.IsBool())   return "Bool";
    if (v.IsNone())   return "Null";
    if (v.IsArray())  return "Array";
    if (v.IsTable())  return "Table";
    if (v.IsObject()) return "Object";
    if (v.IsFunction()) return "Function";
    return "Unknown";
}

// ============================================================================
// Heap allocation helpers
// ============================================================================

NanBoxedValue BytecodeVM::AllocString(const std::string& s) {
    std::string& slot = stringPool_.Acquire();
    slot = s;
    return NanBoxedValue::String(&slot);
}

NanBoxedValue BytecodeVM::AllocObjectRef(const std::string& name) {
    std::string& slot = stringPool_.Acquire();
    slot = name;
    return NanBoxedValue::Object(&slot);
}

NanBoxedValue BytecodeVM::AllocFuncRef(const std::string& name) {
    std::string& slot = stringPool_.Acquire();
    slot = name;
    return NanBoxedValue::Function(&slot);
}

HeapArray& BytecodeVM::AllocArray() {
    HeapArray& slot = arrayPool_.Acquire();
    slot.elements.clear();
    return slot;
}

std::unordered_map<std::string, Value>& BytecodeVM::AllocMap() {
    auto& slot = mapPool_.Acquire();
    slot.clear();
    return slot;
}

// ============================================================================
// Value conversion (VM <-> engine boundary)
// ============================================================================

Value BytecodeVM::ToValue(NanBoxedValue nb) {
    if (nb.IsNumber()) return Value(nb.AsNumber());
    if (nb.IsBool()) return Value(nb.AsBool());
    if (nb.IsNone()) return Value();
    if (nb.IsString()) return Value(*nb.AsString());
    if (nb.IsArray()) {
        Value v;
        v.type = Value::Type::ARRAY;
        for (auto& elem : nb.AsArray()->elements)
            v.arrayValue.push_back(ToValue(elem));
        return v;
    }
    if (nb.IsTable()) {
        Value v;
        v.type = Value::Type::TABLE;
        v.tableValue = static_cast<std::unordered_map<std::string, Value>*>(nb.AsTablePtr());
        return v;
    }
    if (nb.IsObject()) return Value::Object(*nb.AsObjectName());
    if (nb.IsFunction()) {
        Value v;
        v.type = Value::Type::FUNCTION;
        v.stringValue = *nb.AsFunctionName();
        return v;
    }
    if (nb.IsInstance()) return Value::Instance(nb.AsInstance());
    return Value();
}

NanBoxedValue BytecodeVM::FromValue(const Value& v) {
    switch (v.type) {
        case Value::Type::NUMBER: return NanBoxedValue::Number(v.numberValue);
        case Value::Type::BOOL:   return NanBoxedValue::Bool(v.boolValue);
        case Value::Type::NONE:   return NanBoxedValue::None();
        case Value::Type::STRING: return AllocString(v.stringValue);
        case Value::Type::ARRAY: {
            auto& arr = AllocArray();
            for (auto& elem : v.arrayValue)
                arr.elements.push_back(FromValue(elem));
            return NanBoxedValue::Array(&arr);
        }
        case Value::Type::TABLE: {
            // Share the raw map pointer to preserve table identity
            return NanBoxedValue::Table(v.tableValue);
        }
        case Value::Type::OBJECT:
            return AllocObjectRef(v.objectName);
        case Value::Type::FUNCTION: {
            return AllocFuncRef(v.stringValue);
        }
        case Value::Type::SCRIPT_INSTANCE:
            return NanBoxedValue::Instance(v.instanceRef);
    }
    return NanBoxedValue::None();
}

// ============================================================================
// Public entry points
// ============================================================================

Value BytecodeVM::Execute(const BytecodeChunk& chunk, const std::vector<Value>& args) {
    hasError_ = false;
    error_.clear();
    ++executeDepth_;

    frameCount_ = 0;
    stackTop_ = 0;

    for (auto& arg : args) {
        Push(FromValue(arg));
    }
    while (stackTop_ < chunk.localCount) {
        Push(NanBoxedValue::None());
    }

    CallFrame& frame = frames_[frameCount_++];
    frame.chunk = &chunk;
    frame.ip = 0;
    frame.stackBase = 0;

    Value result = Run();
    --executeDepth_;
    return result;
}

Value BytecodeVM::CallFunction(const CompiledScript& script, const std::string& name,
                                const std::vector<Value>& args) {
    auto it = script.functions.find(name);
    if (it == script.functions.end()) {
        RuntimeError("Function not found: " + name);
        return Value();
    }
    return Execute(it->second, args);
}

// ============================================================================
// Stack operations
// ============================================================================

void BytecodeVM::Push(NanBoxedValue v) {
    if (stackTop_ >= MAX_STACK) {
        RuntimeError("Stack overflow");
        return;
    }
    stack_[stackTop_++] = v;
}

NanBoxedValue BytecodeVM::Pop() {
    if (stackTop_ <= 0) {
        RuntimeError("Stack underflow");
        return NanBoxedValue::None();
    }
    return stack_[--stackTop_];
}

NanBoxedValue& BytecodeVM::Peek(int distance) {
    return stack_[stackTop_ - 1 - distance];
}

// ============================================================================
// Byte reading
// ============================================================================

uint8_t BytecodeVM::ReadByte() {
    return CurrentFrame().chunk->code[CurrentFrame().ip++];
}

uint16_t BytecodeVM::ReadU16() {
    CallFrame& f = CurrentFrame();
    uint16_t val = f.chunk->ReadU16(f.ip);
    f.ip += 2;
    return val;
}

OpCode BytecodeVM::ReadOpCode() {
    return static_cast<OpCode>(ReadByte());
}

// ============================================================================
// Error handling
// ============================================================================

void BytecodeVM::RuntimeError(const std::string& msg) {
    if (!hasError_) {
        hasError_ = true;
        std::ostringstream oss;
        if (currentLine_ > 0) {
            oss << "[Line " << currentLine_ << "] ";
        }
        oss << "VM Error: " << msg;

        // Build stack trace from call frames
        if (frameCount_ > 1) {
            oss << "\nStack trace:";
            for (int i = frameCount_ - 1; i >= 0; --i) {
                const CallFrame& f = frames_[i];
                if (!f.chunk) continue;
                int line = f.chunk->GetLine(f.ip > 0 ? f.ip - 1 : 0);
                oss << "\n  in " << f.chunk->name;
                if (line > 0) oss << " (line " << line << ")";
            }
        }
        error_ = oss.str();
    }
}

// ============================================================================
// Main dispatch loop
// ============================================================================

// Promote bools to numbers for arithmetic (true->1.0, false->0.0)
static inline void PromoteBool(NanBoxedValue& v) {
    if (v.IsBool()) v = NanBoxedValue::Number(v.AsBool() ? 1.0 : 0.0);
}

Value BytecodeVM::Run() {
    int instructionCount = 0;

    while (!hasError_ && frameCount_ > 0) {
        if (++instructionCount > MAX_INSTRUCTIONS) {
            RuntimeError("Execution limit exceeded (infinite loop?)");
            return Value();
        }

        CallFrame& frame = CurrentFrame();
        if (frame.ip >= frame.chunk->code.size()) {
            frameCount_--;
            if (frameCount_ > 0) {
                stackTop_ = frame.stackBase;
                Push(NanBoxedValue::None());
            }
            continue;
        }

        OpCode op = ReadOpCode();

        switch (op) {
            case OpCode::PUSH_CONST: {
                uint16_t idx = ReadU16();
                auto& c = frame.chunk->constants[idx];
                if (c.type == Constant::Type::NUMBER) {
                    Push(NanBoxedValue::Number(c.numberValue));
                } else {
                    Push(AllocString(frame.chunk->atoms->GetString(c.stringAtom)));
                }
                break;
            }

            case OpCode::PUSH_TRUE:
                Push(NanBoxedValue::Bool(true));
                break;

            case OpCode::PUSH_FALSE:
                Push(NanBoxedValue::Bool(false));
                break;

            case OpCode::PUSH_NONE:
                Push(NanBoxedValue::None());
                break;

            case OpCode::POP:
                Pop();
                break;

            case OpCode::LOAD_LOCAL: {
                uint16_t slot = ReadU16();
                Push(stack_[frame.stackBase + slot]);
                break;
            }

            case OpCode::STORE_LOCAL: {
                uint16_t slot = ReadU16();
                stack_[frame.stackBase + slot] = Peek();
                Pop();
                break;
            }

            case OpCode::DECLARE_LOCAL: {
                uint16_t slot = ReadU16();
                NanBoxedValue val = Pop();
                if (frame.stackBase + slot < stackTop_) {
                    stack_[frame.stackBase + slot] = val;
                } else {
                    while (stackTop_ <= frame.stackBase + static_cast<int>(slot)) {
                        Push(NanBoxedValue::None());
                    }
                    stack_[frame.stackBase + slot] = val;
                }
                break;
            }

            case OpCode::LOAD_GLOBAL: {
                uint16_t idx = ReadU16();
                const std::string& name = ResolveString(frame.chunk, idx);
                Value val = engine_->GetVariable(name);
                if (val.type == Value::Type::NONE) {
                    if (engine_->ActiveCtx().reflectedObjects.count(name)) {
                        Push(AllocObjectRef(name));
                        break;
                    }
                    if (compiledScript_ && compiledScript_->functions.count(name)) {
                        Push(AllocFuncRef(name));
                        break;
                    }
                    // Check for unloaded module globals
                    static const char* MODULE_GLOBALS[] = {"physics", "audio", "ai", "particles", "effects", "ui"};
                    for (auto* mg : MODULE_GLOBALS) {
                        if (name == mg && !engine_->GetModuleLoader().HasModule(mg)) {
                            RuntimeError("Module '" + name + "' not loaded. Register it with GetModuleLoader().Register() or add the module .so/.bin");
                            break;
                        }
                    }
                }
                if (val.type == Value::Type::FUNCTION && val.stringValue.empty()) {
                    if (compiledScript_ && compiledScript_->functions.count(name)) {
                        val.stringValue = name;
                    }
                }
                Push(FromValue(val));
                break;
            }

            case OpCode::LOAD_CLASS: {
                uint16_t idx = ReadU16();
                const std::string& name = ResolveString(frame.chunk, idx);
                auto& ctx = engine_->ActiveCtx();
                if (!ctx.reflectedObjects.count(name)) {
                    const ClassDesc* cls = ReflectionBridge::FindClass(name);
                    if (!cls) {
                        RuntimeError("Unknown class: " + name);
                        break;
                    }
                    ctx.reflectedObjects[name] = ReflectedObject(nullptr, cls, name, false);
                }
                Push(AllocObjectRef(name));
                break;
            }

            case OpCode::STORE_GLOBAL: {
                uint16_t idx = ReadU16();
                const std::string& name = ResolveString(frame.chunk, idx);
                NanBoxedValue val = Pop();
                engine_->SetVariable(name, ToValue(val));
                break;
            }

            // Arithmetic
            case OpCode::ADD: {
                NanBoxedValue b = Pop();
                NanBoxedValue a = Pop();
                if (a.IsString() || b.IsString()) {
                    ExecuteConcat(a, b);
                } else {
                    PromoteBool(a); PromoteBool(b);
                    if (a.IsNumber() && b.IsNumber()) {
                        Push(NanBoxedValue::Number(a.AsNumber() + b.AsNumber()));
                    } else {
                        NanBoxedValue out;
                        if (TryOperatorOverload(a, b, "__add", out)) {
                            Push(out);
                        } else {
                            RuntimeError(std::string("Cannot add ") + NbTypeName(a) + " + " + NbTypeName(b));
                            Push(NanBoxedValue::None());
                        }
                    }
                }
                break;
            }

            case OpCode::SUB: {
                NanBoxedValue b = Pop();
                NanBoxedValue a = Pop();
                PromoteBool(a); PromoteBool(b);
                if (a.IsNumber() && b.IsNumber()) {
                    Push(NanBoxedValue::Number(a.AsNumber() - b.AsNumber()));
                } else {
                    NanBoxedValue out;
                    if (TryOperatorOverload(a, b, "__sub", out)) {
                        Push(out);
                    } else {
                        RuntimeError(std::string("Cannot subtract ") + NbTypeName(a) + " - " + NbTypeName(b));
                        Push(NanBoxedValue::Number(0.0));
                    }
                }
                break;
            }

            case OpCode::MUL: {
                NanBoxedValue b = Pop();
                NanBoxedValue a = Pop();
                PromoteBool(a); PromoteBool(b);
                if (a.IsNumber() && b.IsNumber()) {
                    Push(NanBoxedValue::Number(a.AsNumber() * b.AsNumber()));
                } else {
                    NanBoxedValue out;
                    if (TryOperatorOverload(a, b, "__mul", out)) {
                        Push(out);
                    } else {
                        RuntimeError(std::string("Cannot multiply ") + NbTypeName(a) + " * " + NbTypeName(b));
                        Push(NanBoxedValue::Number(0.0));
                    }
                }
                break;
            }

            case OpCode::DIV: {
                NanBoxedValue b = Pop();
                NanBoxedValue a = Pop();
                PromoteBool(a); PromoteBool(b);
                if (a.IsNumber() && b.IsNumber()) {
                    if (b.AsNumber() == 0.0) {
                        Push(NanBoxedValue::Number(0.0));
                    } else {
                        Push(NanBoxedValue::Number(a.AsNumber() / b.AsNumber()));
                    }
                } else {
                    NanBoxedValue out;
                    if (TryOperatorOverload(a, b, "__div", out)) {
                        Push(out);
                    } else {
                        RuntimeError(std::string("Cannot divide ") + NbTypeName(a) + " / " + NbTypeName(b));
                        Push(NanBoxedValue::Number(0.0));
                    }
                }
                break;
            }

            case OpCode::MOD: {
                NanBoxedValue b = Pop();
                NanBoxedValue a = Pop();
                PromoteBool(a); PromoteBool(b);
                if (a.IsNumber() && b.IsNumber()) {
                    if (b.AsNumber() == 0.0) {
                        Push(NanBoxedValue::Number(0.0));
                    } else {
                        Push(NanBoxedValue::Number(std::fmod(a.AsNumber(), b.AsNumber())));
                    }
                } else {
                    NanBoxedValue out;
                    if (TryOperatorOverload(a, b, "__mod", out)) {
                        Push(out);
                    } else {
                        RuntimeError(std::string("Cannot modulo ") + NbTypeName(a) + " % " + NbTypeName(b));
                        Push(NanBoxedValue::Number(0.0));
                    }
                }
                break;
            }

            case OpCode::NEG: {
                NanBoxedValue a = Pop();
                Push(NanBoxedValue::Number(-a.AsNumber()));
                break;
            }

            // Comparison
            case OpCode::EQ: {
                NanBoxedValue b = Pop();
                NanBoxedValue a = Pop();
                bool eq = false;
                if (a.IsNumber() && b.IsNumber())
                    eq = a.AsNumber() == b.AsNumber();
                else if (a.IsString() && b.IsString())
                    eq = *a.AsString() == *b.AsString();
                else if (a.IsBool() && b.IsBool())
                    eq = a.AsBool() == b.AsBool();
                else if (a.IsNone() && b.IsNone())
                    eq = true;
                Push(NanBoxedValue::Bool(eq));
                break;
            }

            case OpCode::NEQ: {
                NanBoxedValue b = Pop();
                NanBoxedValue a = Pop();
                bool eq = false;
                if (a.IsNumber() && b.IsNumber())
                    eq = a.AsNumber() == b.AsNumber();
                else if (a.IsString() && b.IsString())
                    eq = *a.AsString() == *b.AsString();
                else if (a.IsBool() && b.IsBool())
                    eq = a.AsBool() == b.AsBool();
                else if (a.IsNone() && b.IsNone())
                    eq = true;
                Push(NanBoxedValue::Bool(!eq));
                break;
            }

            case OpCode::LT: {
                NanBoxedValue b = Pop();
                NanBoxedValue a = Pop();
                if (a.IsNumber() && b.IsNumber()) {
                    Push(NanBoxedValue::Bool(a.AsNumber() < b.AsNumber()));
                } else {
                    RuntimeError(std::string("Cannot compare ") + NbTypeName(a) + " < " + NbTypeName(b));
                    Push(NanBoxedValue::Bool(false));
                }
                break;
            }

            case OpCode::GT: {
                NanBoxedValue b = Pop();
                NanBoxedValue a = Pop();
                if (a.IsNumber() && b.IsNumber()) {
                    Push(NanBoxedValue::Bool(a.AsNumber() > b.AsNumber()));
                } else {
                    RuntimeError(std::string("Cannot compare ") + NbTypeName(a) + " > " + NbTypeName(b));
                    Push(NanBoxedValue::Bool(false));
                }
                break;
            }

            case OpCode::LTE: {
                NanBoxedValue b = Pop();
                NanBoxedValue a = Pop();
                if (a.IsNumber() && b.IsNumber()) {
                    Push(NanBoxedValue::Bool(a.AsNumber() <= b.AsNumber()));
                } else {
                    RuntimeError(std::string("Cannot compare ") + NbTypeName(a) + " <= " + NbTypeName(b));
                    Push(NanBoxedValue::Bool(false));
                }
                break;
            }

            case OpCode::GTE: {
                NanBoxedValue b = Pop();
                NanBoxedValue a = Pop();
                if (a.IsNumber() && b.IsNumber()) {
                    Push(NanBoxedValue::Bool(a.AsNumber() >= b.AsNumber()));
                } else {
                    RuntimeError(std::string("Cannot compare ") + NbTypeName(a) + " >= " + NbTypeName(b));
                    Push(NanBoxedValue::Bool(false));
                }
                break;
            }

            // Logic
            case OpCode::AND: {
                NanBoxedValue b = Pop();
                NanBoxedValue a = Pop();
                Push(NanBoxedValue::Bool(a.IsTruthy() && b.IsTruthy()));
                break;
            }

            case OpCode::OR: {
                NanBoxedValue b = Pop();
                NanBoxedValue a = Pop();
                Push(NanBoxedValue::Bool(a.IsTruthy() || b.IsTruthy()));
                break;
            }

            case OpCode::NOT: {
                NanBoxedValue a = Pop();
                Push(NanBoxedValue::Bool(!a.IsTruthy()));
                break;
            }

            case OpCode::CONCAT: {
                NanBoxedValue b = Pop();
                NanBoxedValue a = Pop();
                ExecuteConcat(a, b);
                break;
            }

            // Control flow
            case OpCode::JUMP: {
                uint16_t offset = ReadU16();
                frame.ip += offset;
                break;
            }

            case OpCode::JUMP_BACK: {
                uint16_t offset = ReadU16();
                frame.ip -= offset;
                break;
            }

            case OpCode::JUMP_IF_FALSE: {
                uint16_t offset = ReadU16();
                if (!Peek().IsTruthy()) {
                    frame.ip += offset;
                }
                break;
            }

            case OpCode::JUMP_IF_TRUE: {
                uint16_t offset = ReadU16();
                if (Peek().IsTruthy()) {
                    frame.ip += offset;
                }
                break;
            }

            // Function calls
            case OpCode::CALL: {
                uint8_t argc = ReadByte();
                NanBoxedValue funcRef = Pop();

                if (funcRef.IsFunction()) {
                    std::string* funcName = funcRef.AsFunctionName();
                    if (compiledScript_ && funcName && !funcName->empty()) {
                        auto it = compiledScript_->functions.find(*funcName);
                        if (it != compiledScript_->functions.end()) {
                            if (frameCount_ >= MAX_FRAMES) {
                                RuntimeError("Call stack overflow");
                                break;
                            }

                            const BytecodeChunk& callee = it->second;
                            int base = stackTop_ - argc;

                            while (stackTop_ < base + callee.localCount) {
                                Push(NanBoxedValue::None());
                            }

                            CallFrame& newFrame = frames_[frameCount_++];
                            newFrame.chunk = &callee;
                            newFrame.ip = 0;
                            newFrame.stackBase = base;
                            break;
                        }
                    }

                    // Function not found in compiled script
                    if (funcName && !funcName->empty()) {
                        RuntimeError("Undefined function: " + *funcName);
                    }
                    stackTop_ -= argc;
                    Push(NanBoxedValue::None());
                    break;
                }

                // Try as constructor (string carrying class name)
                if (funcRef.IsObject() || funcRef.IsNone() || funcRef.IsString()) {
                    std::string className;
                    if (funcRef.IsObject()) className = *funcRef.AsObjectName();
                    else if (funcRef.IsString()) className = *funcRef.AsString();
                    if (!className.empty()) {
                        Value result = ConstructObject(className, argc);
                        Push(FromValue(result));
                        break;
                    }
                }

                // Unknown callable
                stackTop_ -= argc;
                Push(NanBoxedValue::None());
                break;
            }

            case OpCode::RETURN: {
                NanBoxedValue result = Pop();
                int base = frame.stackBase;
                frameCount_--;
                stackTop_ = base;

                if (frameCount_ == 0) {
                    return ToValue(result);
                }
                Push(result);
                break;
            }

            case OpCode::RETURN_NONE: {
                int base = frame.stackBase;
                frameCount_--;
                stackTop_ = base;

                if (frameCount_ == 0) {
                    return Value();
                }
                Push(NanBoxedValue::None());
                break;
            }

            // Builtins
            case OpCode::CALL_BUILTIN: {
                uint16_t builtinId = ReadU16();
                uint8_t argc = ReadByte();
                Value result = CallBuiltin(builtinId, argc);
                Push(FromValue(result));
                break;
            }

            // Object operations
            case OpCode::GET_MEMBER: {
                uint16_t nameIdx = ReadU16();
                const std::string& memberName = ResolveString(frame.chunk, nameIdx);
                NanBoxedValue obj = Pop();
                if (obj.IsInstance() && obj.AsInstance()) {
                    Push(FromValue(obj.AsInstance()->GetField(memberName)));
                } else {
                    Value objVal = ToValue(obj);
                    Push(FromValue(GetMember(objVal, memberName)));
                }
                break;
            }

            case OpCode::SET_MEMBER: {
                uint16_t nameIdx = ReadU16();
                const std::string& memberName = ResolveString(frame.chunk, nameIdx);
                NanBoxedValue obj = Pop();
                NanBoxedValue val = Pop();

                // Promote constructed objects stored in fields to persistent
                if (val.IsObject() && val.AsObjectName()) {
                    engine_->PromoteToPersistent(*val.AsObjectName());
                }

                if (obj.IsInstance() && obj.AsInstance()) {
                    // Clean up old persistent object in instance field being overwritten
                    Value oldVal = obj.AsInstance()->GetField(memberName);
                    if (oldVal.type == Value::Type::OBJECT && !oldVal.objectName.empty()
                        && oldVal.objectName != (val.AsObjectName() ? *val.AsObjectName() : "")) {
                        engine_->CleanupPersistentObject(oldVal.objectName);
                    }
                    obj.AsInstance()->SetField(memberName, ToValue(val));
                } else if (obj.IsTable()) {
                    auto* map = static_cast<std::unordered_map<std::string, Value>*>(obj.AsTablePtr());
                    if (map) {
                        // Clean up old persistent object being overwritten
                        auto oldIt = map->find(memberName);
                        if (oldIt != map->end() && oldIt->second.type == Value::Type::OBJECT
                            && !oldIt->second.objectName.empty()
                            && oldIt->second.objectName != (val.AsObjectName() ? *val.AsObjectName() : "")) {
                            engine_->CleanupPersistentObject(oldIt->second.objectName);
                        }
                        (*map)[memberName] = ToValue(val);
                    }
                } else if (obj.IsObject()) {
                    engine_->SetVariable(*obj.AsObjectName() + "." + memberName, ToValue(val));
                } else {
                    Value objVal = ToValue(obj);
                    SetMember(objVal, memberName, ToValue(val));
                }
                break;
            }

            case OpCode::CALL_METHOD: {
                uint16_t nameIdx = ReadU16();
                uint8_t argc = ReadByte();
                const std::string& methodName = ResolveString(frame.chunk, nameIdx);

                // Collect args into reusable buffer
                methodArgs_.resize(argc);
                for (int i = argc - 1; i >= 0; --i) {
                    methodArgs_[i] = ToValue(Pop());
                }
                NanBoxedValue obj = Pop();

                // Array methods
                if (obj.IsArray()) {
                    Value arrVal = ToValue(obj);
                    Value result = engine_->DispatchArrayMethod(&arrVal, methodName, methodArgs_);
                    Push(FromValue(result));
                    break;
                }
                // Table methods
                if (obj.IsTable()) {
                    Value tblVal = ToValue(obj);
                    Value result = engine_->DispatchTableMethod(tblVal.tableValue, methodName, methodArgs_);
                    Push(FromValue(result));
                    break;
                }

                // Reflected object - direct dispatch (no string concat)
                if (obj.IsObject()) {
                    const std::string* objName = obj.AsObjectName();
                    if (objName && !objName->empty()) {
                        auto& ctx = engine_->ActiveCtx();
                        // Check reflectedObjects directly first
                        auto it = ctx.reflectedObjects.find(*objName);
                        if (it != ctx.reflectedObjects.end() && it->second.classDesc) {
                            Value result = engine_->CallReflectedMethodDirect(
                                it->second.instance, it->second.classDesc,
                                methodName, argc, methodArgs_);
                            Push(FromValue(result));
                            break;
                        }
                        // Check if it's a variable holding an object reference
                        auto vit = ctx.variables.find(*objName);
                        if (vit != ctx.variables.end() && vit->second.type == Value::Type::OBJECT) {
                            auto rit = ctx.reflectedObjects.find(vit->second.objectName);
                            if (rit != ctx.reflectedObjects.end() && rit->second.instance) {
                                Value result = engine_->CallReflectedMethodDirect(
                                    rit->second.instance, rit->second.classDesc,
                                    methodName, argc, methodArgs_);
                                Push(FromValue(result));
                                break;
                            }
                        }
                    }
                    // Fallback: full path-based dispatch
                    std::string fullPath = (objName ? *objName : std::string()) + "." + methodName;
                    Value result = engine_->CallReflectedMethod(fullPath, methodArgs_);
                    Push(FromValue(result));
                    break;
                }

                // Script instance method
                if (obj.IsInstance() && obj.AsInstance()) {
                    ScriptInstance* inst = obj.AsInstance();
                    std::string qualifiedName = inst->klass->name + "." + methodName;
                    if (compiledScript_ && compiledScript_->functions.count(qualifiedName)) {
                        auto& methodChunk = compiledScript_->functions.at(qualifiedName);
                        if (frameCount_ >= MAX_FRAMES) {
                            RuntimeError("Stack overflow");
                            break;
                        }
                        int base = stackTop_;
                        Push(obj); // self at slot 0
                        for (auto& a : methodArgs_) Push(FromValue(a));
                        while (stackTop_ < base + methodChunk.localCount) Push(NanBoxedValue::None());
                        CallFrame& nf = frames_[frameCount_++];
                        nf.chunk = &methodChunk;
                        nf.ip = 0;
                        nf.stackBase = base;
                    } else {
                        RuntimeError("Method not found: " + methodName + " on class " + inst->klass->name);
                    }
                    break;
                }

                // None/unknown  silently returns NONE (method on non-object)
                Push(NanBoxedValue::None());
                break;
            }

            case OpCode::CONSTRUCT: {
                uint16_t nameIdx = ReadU16();
                uint8_t argc = ReadByte();
                const std::string& className = ResolveString(frame.chunk, nameIdx);
                Value result = ConstructObject(className, argc);
                Push(FromValue(result));
                break;
            }

            // Collections
            case OpCode::MAKE_ARRAY: {
                uint16_t count = ReadU16();
                auto& arr = AllocArray();
                arr.elements.reserve(count);
                for (int i = count - 1; i >= 0; --i) {
                    NanBoxedValue val = Pop();
                    // Promote constructed objects stored in array literals to persistent
                    if (val.IsObject() && val.AsObjectName()) {
                        engine_->PromoteToPersistent(*val.AsObjectName());
                    }
                    arr.elements.push_back(val);
                }
                std::reverse(arr.elements.begin(), arr.elements.end());
                Push(NanBoxedValue::Array(&arr));
                break;
            }

            case OpCode::MAKE_TABLE: {
                uint16_t count = ReadU16();
                auto& map = AllocMap();
                for (int i = 0; i < count; ++i) {
                    NanBoxedValue val = Pop();
                    NanBoxedValue key = Pop();
                    std::string keyStr = key.IsString() ? *key.AsString() : "";
                    // Promote objects stored in table literals to persistent
                    if (val.IsObject() && val.AsObjectName()) {
                        engine_->PromoteToPersistent(*val.AsObjectName());
                    }
                    map[keyStr] = ToValue(val);
                }
                Push(NanBoxedValue::Table(&map));
                break;
            }

            case OpCode::INDEX_GET: {
                NanBoxedValue index = Pop();
                NanBoxedValue collection = Pop();
                if (collection.IsArray()) {
                    int idx = static_cast<int>(index.AsNumber());
                    auto& elems = collection.AsArray()->elements;
                    if (idx >= 0 && idx < static_cast<int>(elems.size())) {
                        Push(elems[idx]);
                    } else {
                        Push(NanBoxedValue::None());
                    }
                } else if (collection.IsTable()) {
                    auto* map = static_cast<std::unordered_map<std::string, Value>*>(collection.AsTablePtr());
                    if (map) {
                        std::string key = index.IsNumber()
                            ? std::to_string(static_cast<int>(index.AsNumber()))
                            : (index.IsString() ? *index.AsString() : "");
                        auto it = map->find(key);
                        Push(it != map->end() ? FromValue(it->second) : NanBoxedValue::None());
                    } else {
                        Push(NanBoxedValue::None());
                    }
                } else {
                    Push(NanBoxedValue::None());
                }
                break;
            }

            case OpCode::INDEX_SET: {
                NanBoxedValue val = Pop();
                NanBoxedValue index = Pop();
                NanBoxedValue collection = Pop();

                // Promote constructed objects stored in collections to persistent
                if (val.IsObject() && val.AsObjectName()) {
                    engine_->PromoteToPersistent(*val.AsObjectName());
                }

                if (collection.IsArray()) {
                    int idx = static_cast<int>(index.AsNumber());
                    auto& elems = collection.AsArray()->elements;
                    if (idx >= 0 && idx < static_cast<int>(elems.size())) {
                        // Clean up old persistent object being overwritten
                        auto& oldElem = elems[idx];
                        if (oldElem.IsObject() && oldElem.AsObjectName()
                            && !(val.IsObject() && val.AsObjectName() && *val.AsObjectName() == *oldElem.AsObjectName())) {
                            engine_->CleanupPersistentObject(*oldElem.AsObjectName());
                        }
                        elems[idx] = val;
                    } else if (idx == static_cast<int>(elems.size())) {
                        elems.push_back(val);
                    }
                } else if (collection.IsTable()) {
                    auto* map = static_cast<std::unordered_map<std::string, Value>*>(collection.AsTablePtr());
                    if (map) {
                        std::string key = index.IsNumber()
                            ? std::to_string(static_cast<int>(index.AsNumber()))
                            : (index.IsString() ? *index.AsString() : "");
                        // Clean up old persistent object being overwritten
                        auto oldIt = map->find(key);
                        if (oldIt != map->end() && oldIt->second.type == Value::Type::OBJECT
                            && !oldIt->second.objectName.empty()
                            && oldIt->second.objectName != (val.AsObjectName() ? *val.AsObjectName() : "")) {
                            engine_->CleanupPersistentObject(oldIt->second.objectName);
                        }
                        (*map)[key] = ToValue(val);
                    }
                }
                // Push modified collection back for writeback
                Push(collection);
                break;
            }

            // Iteration
            case OpCode::ITER_INIT: {
                NanBoxedValue collection = Pop();
                Iterator iter;
                if (collection.IsArray()) {
                    iter.values = collection.AsArray()->elements;
                } else if (collection.IsTable()) {
                    auto* map = static_cast<std::unordered_map<std::string, Value>*>(collection.AsTablePtr());
                    if (map) {
                        for (auto& [k, v] : *map) {
                            iter.values.push_back(AllocString(k));
                        }
                    }
                }
                iter.index = 0;
                iterators_.push_back(std::move(iter));
                Push(NanBoxedValue::Number(static_cast<double>(iterators_.size() - 1)));
                break;
            }

            case OpCode::ITER_NEXT: {
                uint16_t offset = ReadU16();
                if (iterators_.empty()) {
                    frame.ip += offset;
                    break;
                }
                Iterator& iter = iterators_.back();
                if (iter.index >= iter.values.size()) {
                    iterators_.pop_back();
                    frame.ip += offset;
                }
                break;
            }

            case OpCode::ITER_VALUE: {
                if (!iterators_.empty()) {
                    Iterator& iter = iterators_.back();
                    if (iter.index < iter.values.size()) {
                        Push(iter.values[iter.index]);
                        iter.index++;
                    } else {
                        Push(NanBoxedValue::None());
                    }
                } else {
                    Push(NanBoxedValue::None());
                }
                break;
            }

            case OpCode::DECLARE_SIGNAL: {
                uint16_t nameIdx = ReadU16();
                const std::string& sigName = ResolveString(frame.chunk, nameIdx);
                engine_->GetSignalRegistry().DeclareSignal(sigName);
                break;
            }

            case OpCode::EMIT_SIGNAL: {
                uint16_t nameIdx = ReadU16();
                uint8_t argc = ReadByte();
                const std::string& sigName = ResolveString(frame.chunk, nameIdx);
                
                // Collect args from stack
                std::vector<Value> args;
                args.reserve(argc);
                for (int i = 0; i < argc; ++i) {
                    args.push_back(ToValue(Pop()));
                }
                std::reverse(args.begin(), args.end());
                
                // Save VM state before calling handlers
                int savedStackTop = stackTop_;
                int savedFrameCount = frameCount_;
                NanBoxedValue savedStack[MAX_STACK];
                CallFrame savedFrames[MAX_FRAMES];
                std::memcpy(savedStack, stack_, savedStackTop * sizeof(NanBoxedValue));
                std::memcpy(savedFrames, frames_, savedFrameCount * sizeof(CallFrame));
                
                // Get handlers and call each one
                auto handlers = engine_->GetSignalRegistry().GetHandlers(sigName);
                for (auto& handlerName : handlers) {
                    if (compiledScript_ && compiledScript_->functions.count(handlerName)) {
                        auto& chunk = compiledScript_->functions.at(handlerName);
                        Execute(chunk, args);
                    }
                }
                
                // Restore VM state
                std::memcpy(stack_, savedStack, savedStackTop * sizeof(NanBoxedValue));
                std::memcpy(frames_, savedFrames, savedFrameCount * sizeof(CallFrame));
                stackTop_ = savedStackTop;
                frameCount_ = savedFrameCount;
                break;
            }

            case OpCode::NEW_INSTANCE: {
                uint16_t nameIdx = ReadU16();
                uint8_t argc = ReadByte();
                const std::string& className = ResolveString(frame.chunk, nameIdx);

                if (!compiledScript_ || !compiledScript_->classes.count(className)) {
                    Value result = ConstructObject(className, argc);
                    Push(FromValue(result));
                    break;
                }

                auto& klass = compiledScript_->classes.at(className);
                auto instance = std::make_unique<ScriptInstance>();
                instance->klass = klass.get();
                instance->fields.resize(klass->fieldDefs.size());

                // Collect constructor arguments (on stack in reverse order)
                std::vector<Value> initArgs;
                initArgs.resize(argc);
                for (int i = argc - 1; i >= 0; --i) {
                    initArgs[i] = ToValue(Pop());
                }

                ScriptInstance* rawPtr = instance.get();
                ownedInstances_.push_back(std::move(instance));
                NanBoxedValue instanceVal = NanBoxedValue::Instance(rawPtr);

                // Call __init with self + constructor args
                std::string initName = className + ".__init";
                if (compiledScript_->functions.count(initName)) {
                    auto& initChunk = compiledScript_->functions.at(initName);
                    int savedStackTop = stackTop_;
                    int savedFrameCount = frameCount_;
                    NanBoxedValue savedStack[MAX_STACK];
                    CallFrame savedFrames[MAX_FRAMES];
                    for (int i = 0; i < stackTop_; ++i) savedStack[i] = stack_[i];
                    for (int i = 0; i < frameCount_; ++i) savedFrames[i] = frames_[i];

                    std::vector<Value> fullArgs;
                    fullArgs.reserve(1 + argc);
                    fullArgs.push_back(Value::Instance(rawPtr));
                    for (auto& a : initArgs) fullArgs.push_back(std::move(a));
                    Execute(initChunk, fullArgs);

                    for (int i = 0; i < savedStackTop; ++i) stack_[i] = savedStack[i];
                    for (int i = 0; i < savedFrameCount; ++i) frames_[i] = savedFrames[i];
                    stackTop_ = savedStackTop;
                    frameCount_ = savedFrameCount;
                }

                Push(instanceVal);
                break;
            }

            case OpCode::LOAD_SELF: {
                NanBoxedValue selfVal = stack_[frame.stackBase];
                if (selfVal.IsInstance()) {
                    Push(selfVal);
                } else {
                    Push(NanBoxedValue::None());
                }
                break;
            }

            case OpCode::GET_FIELD: {
                uint16_t nameIdx = ReadU16();
                const std::string& fieldName = ResolveString(frame.chunk, nameIdx);
                NanBoxedValue obj = Pop();
                if (obj.IsInstance() && obj.AsInstance()) {
                    Push(FromValue(obj.AsInstance()->GetField(fieldName)));
                } else if (obj.IsTable()) {
                    auto* map = static_cast<std::unordered_map<std::string, Value>*>(obj.AsTablePtr());
                    if (map && map->count(fieldName)) {
                        Push(FromValue((*map)[fieldName]));
                    } else {
                        Push(NanBoxedValue::None());
                    }
                } else {
                    Push(NanBoxedValue::None());
                }
                break;
            }

            case OpCode::SET_FIELD: {
                uint16_t nameIdx = ReadU16();
                const std::string& fieldName = ResolveString(frame.chunk, nameIdx);
                NanBoxedValue val = Pop();
                NanBoxedValue obj = Pop();

                // Promote constructed objects stored in fields to persistent
                if (val.IsObject() && val.AsObjectName()) {
                    engine_->PromoteToPersistent(*val.AsObjectName());
                }

                if (obj.IsInstance() && obj.AsInstance()) {
                    // Clean up old persistent object in instance field being overwritten
                    Value oldVal = obj.AsInstance()->GetField(fieldName);
                    if (oldVal.type == Value::Type::OBJECT && !oldVal.objectName.empty()
                        && oldVal.objectName != (val.AsObjectName() ? *val.AsObjectName() : "")) {
                        engine_->CleanupPersistentObject(oldVal.objectName);
                    }
                    obj.AsInstance()->SetField(fieldName, ToValue(val));
                } else if (obj.IsTable()) {
                    auto* map = static_cast<std::unordered_map<std::string, Value>*>(obj.AsTablePtr());
                    if (map) {
                        // Clean up old persistent object being overwritten
                        auto oldIt = map->find(fieldName);
                        if (oldIt != map->end() && oldIt->second.type == Value::Type::OBJECT
                            && !oldIt->second.objectName.empty()
                            && oldIt->second.objectName != (val.AsObjectName() ? *val.AsObjectName() : "")) {
                            engine_->CleanupPersistentObject(oldIt->second.objectName);
                        }
                        (*map)[fieldName] = ToValue(val);
                    }
                }
                break;
            }

            case OpCode::CALL_SCRIPT_METHOD: {
                uint16_t nameIdx = ReadU16();
                uint8_t argc = ReadByte();
                const std::string& methodName = ResolveString(frame.chunk, nameIdx);

                int instanceSlot = stackTop_ - argc - 1;
                NanBoxedValue instanceVal = stack_[instanceSlot];

                if (!instanceVal.IsInstance() || !instanceVal.AsInstance()) {
                    RuntimeError("Cannot call method on non-instance");
                    break;
                }

                ScriptInstance* inst = instanceVal.AsInstance();
                std::string qualifiedName = inst->klass->name + "." + methodName;

                if (compiledScript_ && compiledScript_->functions.count(qualifiedName)) {
                    auto& methodChunk = compiledScript_->functions.at(qualifiedName);
                    if (frameCount_ >= MAX_FRAMES) {
                        RuntimeError("Stack overflow: max call depth reached");
                        break;
                    }
                    CallFrame& newFrame = frames_[frameCount_++];
                    newFrame.chunk = &methodChunk;
                    newFrame.ip = 0;
                    newFrame.stackBase = instanceSlot;
                    while (stackTop_ < newFrame.stackBase + methodChunk.localCount) {
                        Push(NanBoxedValue::None());
                    }
                } else {
                    RuntimeError("Method not found: " + methodName);
                }
                break;
            }

            case OpCode::YIELD: {
                // Suspend coroutine: save state and return sentinel
                if (suspendedState_) {
                    // Save frame state
                    suspendedState_->frameCount = frameCount_;
                    for (int i = 0; i < frameCount_; i++) {
                        suspendedState_->frames[i].chunk = frames_[i].chunk;
                        suspendedState_->frames[i].ip = frames_[i].ip;
                        suspendedState_->frames[i].stackBase = frames_[i].stackBase;
                    }
                    // Save stack
                    suspendedState_->stackTop = stackTop_;
                    for (int i = 0; i < stackTop_; i++) {
                        suspendedState_->stack[i] = stack_[i];
                    }
                    // Save iterators
                    suspendedState_->iterators.clear();
                    for (auto& it : iterators_) {
                        CoroutineState::SavedIterator si;
                        si.values = it.values;
                        si.index = it.index;
                        suspendedState_->iterators.push_back(std::move(si));
                    }
                    suspendedState_->finished = false;
                    yielded_ = true;
                    return Value(); // Exit Run() loop
                }
                // If not in coroutine context, yield is a no-op
                break;
            }

            case OpCode::LINE: {
                currentLine_ = ReadU16();
                break;
            }

        } // switch
    } // while

    return Value();
}

// ============================================================================
// Operator overloading for reflected objects
// ============================================================================

bool BytecodeVM::TryOperatorOverload(NanBoxedValue& a, NanBoxedValue& b,
                                     const char* opMethod, NanBoxedValue& out) {
    if (!a.IsObject()) return false;

    const std::string& objName = *a.AsObjectName();
    auto& reflObjs = engine_->ActiveCtx().reflectedObjects;
    auto it = reflObjs.find(objName);
    if (it == reflObjs.end()) return false;

    void* instance = it->second.instance;
    const ClassDesc* classDesc = it->second.classDesc;
    if (!instance || !classDesc) return false;

    // Find operator method (e.g., "__add") with type-aware resolution
    Value rhsVal = ToValue(b);
    const ClassDesc* rhsDesc = nullptr;
    if (rhsVal.type == Value::Type::OBJECT) {
        auto rhsIt = reflObjs.find(rhsVal.objectName);
        if (rhsIt != reflObjs.end()) rhsDesc = rhsIt->second.classDesc;
    }
    const MethodDesc* method = ReflectionBridge::FindMethodTyped(classDesc, opMethod, 1, &rhsDesc);
    if (!method) return false;

    // Marshal the RHS argument
    static ArgMarshaller marshaller;
    marshaller.Clear();
    void* argPtr = marshaller.Marshal(rhsVal, method->arg_types.data[0],
                                      engine_->ActiveCtx().reflectedObjects);
    if (!argPtr) return false;

    // Invoke
    void* result = ReflectionBridge::InvokeMethod(instance, method, &argPtr);
    if (!result) return false;

    // Marshal return value  operator methods return by value (new object)
    Value retVal = MarshalReturnValue(result, method->ret_type, method->ret_owns);
    if (retVal.type != Value::Type::NONE) {
        out = FromValue(retVal);
        return true;
    }

    // Complex return (e.g., Vector3D)  register as temp reflected object
    const ClassDesc* retClass = ClassForType(*method->ret_type);
    if (retClass && result) {
        std::string tempName = "_temp_return_" + std::to_string(engine_->ActiveCtx().tempCounter++);
        void* newInstance = ::operator new(method->ret_size);
        if (retClass->copy_construct) {
            retClass->copy_construct(newInstance, result);
        } else {
            std::memcpy(newInstance, result, method->ret_size);
        }
        engine_->ActiveCtx().reflectedObjects[tempName] =
            ReflectedObject(newInstance, retClass, tempName, true);
        engine_->ActiveCtx().frameTempObjects.push_back(tempName);
        // Free the BoxReturn allocation now that we've copied it
        if (method->ret_owns) {
            if (retClass->destroy) {
                retClass->destroy(result);
            } else {
                ::operator delete(result);
            }
        }
        out = AllocObjectRef(tempName);
        return true;
    }

    return false;
}

// ============================================================================
// String concatenation
// ============================================================================

void BytecodeVM::ExecuteConcat(NanBoxedValue a, NanBoxedValue b) {
    auto toString = [this](NanBoxedValue v) -> std::string {
        if (v.IsString()) return *v.AsString();
        if (v.IsNumber()) {
            double d = v.AsNumber();
            if (d == static_cast<int>(d)) return std::to_string(static_cast<int>(d));
            std::ostringstream oss;
            oss << d;
            return oss.str();
        }
        if (v.IsBool()) return v.AsBool() ? "true" : "false";
        if (v.IsNone()) return "none";
        if (v.IsObject()) return *v.AsObjectName();
        return "";
    };
    Push(AllocString(toString(a) + toString(b)));
}

// ============================================================================
// Builtin function dispatch
// ============================================================================

Value BytecodeVM::CallBuiltin(uint16_t builtinId, int argc) {
    std::vector<Value> args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        args.push_back(ToValue(Pop()));
    }
    std::reverse(args.begin(), args.end());

    static const char* BUILTIN_NAMES[] = {
        "sin", "cos", "tan", "abs", "sqrt",
        "min", "max", "lerp", "clamp", "print",
        "length", "floor", "ceil", "round",
        "contains", "type", "str", "num", "random",
        "map", "degrees", "radians",
        "connect", "disconnect", "connect_once",
        "profiler_fps", "profiler_time", "profiler_enable",
        "has_module", "list_modules",
        "start_coroutine", "stop_coroutine",
        "stop_all_coroutines",
        "debug",
        "format",
    };

    // Handle coroutine builtins locally (need access to VM/engine coroutine manager)
    if (builtinId == 30 && argc >= 1 && args[0].type == Value::Type::STRING) {
        // start_coroutine("functionName")
        engine_->GetCoroutineManager().Start(args[0].stringValue);
        return Value(1.0);
    }
    if (builtinId == 31 && argc >= 1 && args[0].type == Value::Type::STRING) {
        // stop_coroutine("functionName")  delegate to CoroutineManager
        return Value(engine_->GetCoroutineManager().StopByName(args[0].stringValue) ? 1.0 : 0.0);
    }
    if (builtinId == 32) {
        // stop_all_coroutines()  clear all active and pending coroutines
        engine_->GetCoroutineManager().Clear();
        return Value(1.0);
    }

    if (builtinId < sizeof(BUILTIN_NAMES) / sizeof(BUILTIN_NAMES[0])) {
        return engine_->CallBuiltinFunction(BUILTIN_NAMES[builtinId], args);
    }

    return Value();
}

// ============================================================================
// Reflection bridge
// ============================================================================

Value BytecodeVM::GetMember(const Value& object, const std::string& memberName) {
    if (object.type == Value::Type::OBJECT && !object.objectName.empty()) {
        std::string path = object.objectName + "." + memberName;
        return engine_->EvaluateMemberAccess(path);
    }
    if (object.type == Value::Type::TABLE && object.tableValue) {
        auto it = object.tableValue->find(memberName);
        if (it != object.tableValue->end()) return it->second;
    }
    if (object.type == Value::Type::ARRAY) {
        if (memberName == "length") return Value(static_cast<double>(object.arrayValue.size()));
    }
    return Value();
}

void BytecodeVM::SetMember(const Value& object, const std::string& memberName, const Value& value) {
    if (object.type == Value::Type::OBJECT && !object.objectName.empty()) {
        std::string path = object.objectName + "." + memberName;
        engine_->SetVariable(path, value);
    } else if (object.type == Value::Type::TABLE && object.tableValue) {
        (*object.tableValue)[memberName] = value;
    }
}

Value BytecodeVM::ConstructObject(const std::string& className, int argc) {
    constructArgs_.resize(argc);
    for (int i = argc - 1; i >= 0; --i) {
        constructArgs_[i] = ToValue(Pop());
    }

    const ClassDesc* classDesc = ReflectionBridge::FindClass(className);
    if (classDesc) {
        return engine_->ConstructObject(classDesc, constructArgs_);
    }

    RuntimeError("Unknown class '" + className + "'  no reflected type registered. Check that the required module is loaded.");
    return Value();
}

// ============================================================================
// Coroutine resume
// ============================================================================

Value BytecodeVM::ResumeCoroutine(CoroutineState& state) {
    // Restore VM state from coroutine snapshot
    frameCount_ = state.frameCount;
    for (int i = 0; i < frameCount_; i++) {
        frames_[i].chunk = state.frames[i].chunk;
        frames_[i].ip = state.frames[i].ip;
        frames_[i].stackBase = state.frames[i].stackBase;
    }
    stackTop_ = state.stackTop;
    for (int i = 0; i < stackTop_; i++) {
        stack_[i] = state.stack[i];
    }
    iterators_.clear();
    for (auto& si : state.iterators) {
        Iterator it;
        it.values = si.values;
        it.index = si.index;
        iterators_.push_back(std::move(it));
    }

    // Set up for yield detection
    suspendedState_ = &state;
    yielded_ = false;

    // Continue execution
    Value result = Run();

    // If the function returned normally (no yield), mark finished
    if (!yielded_) {
        state.finished = true;
    }

    suspendedState_ = nullptr;
    return result;
}

} // namespace scripting
} // namespace koilo
