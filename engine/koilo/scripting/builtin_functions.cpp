// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/debug/profiler.hpp>
#include <cmath>
#include <cstdio>
#include <iostream>

namespace koilo {
namespace scripting {

// Fast double->string (replaces std::ostringstream on hot path)
static inline std::string NumToStr(double val) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%g", val);
    return std::string(buf, len > 0 ? len : 0);
}

// Deep value inspection for debug() builtin
static std::string InspectValue(const Value& v, int depth = 0) {
    if (depth > 4) return "...";
    switch (v.type) {
        case Value::Type::NUMBER: return NumToStr(v.numberValue);
        case Value::Type::STRING: return "\"" + v.stringValue + "\"";
        case Value::Type::BOOL: return v.boolValue ? "true" : "false";
        case Value::Type::NONE: return "null";
        case Value::Type::ARRAY: {
            std::string s = "[";
            for (size_t i = 0; i < v.arrayValue.size(); i++) {
                if (i > 0) s += ", ";
                s += InspectValue(v.arrayValue[i], depth + 1);
            }
            return s + "]";
        }
        case Value::Type::TABLE: {
            if (!v.tableValue) return "{}";
            std::string s = "{";
            bool first = true;
            for (auto& [k, val] : *v.tableValue) {
                if (!first) s += ", ";
                s += k + ": " + InspectValue(val, depth + 1);
                first = false;
            }
            return s + "}";
        }
        case Value::Type::OBJECT: return "<" + v.objectName + ">";
        case Value::Type::SCRIPT_INSTANCE: return "<instance>";
        case Value::Type::FUNCTION: return "<function>";
    }
    return "<unknown>";
}

Value KoiloScriptEngine::CallBuiltinFunction(const std::string& name, const std::vector<Value>& args) {
    // Math functions
    if (name == "sin" && !args.empty() && args[0].type == Value::Type::NUMBER) {
        return Value(std::sin(args[0].numberValue));
    }
    if (name == "cos" && !args.empty() && args[0].type == Value::Type::NUMBER) {
        return Value(std::cos(args[0].numberValue));
    }
    if (name == "tan" && !args.empty() && args[0].type == Value::Type::NUMBER) {
        return Value(std::tan(args[0].numberValue));
    }
    if (name == "abs" && !args.empty() && args[0].type == Value::Type::NUMBER) {
        return Value(std::abs(args[0].numberValue));
    }
    if (name == "sqrt" && !args.empty() && args[0].type == Value::Type::NUMBER) {
        return Value(std::sqrt(args[0].numberValue));
    }
    
    // Min/max
    if (name == "min" && args.size() >= 2 && args[0].type == Value::Type::NUMBER && args[1].type == Value::Type::NUMBER) {
        return Value(std::min(args[0].numberValue, args[1].numberValue));
    }
    if (name == "max" && args.size() >= 2 && args[0].type == Value::Type::NUMBER && args[1].type == Value::Type::NUMBER) {
        return Value(std::max(args[0].numberValue, args[1].numberValue));
    }
    
    // Lerp (linear interpolation)
    if (name == "lerp" && args.size() >= 3) {
        double a = args[0].numberValue;
        double b = args[1].numberValue;
        double t = args[2].numberValue;
        return Value(a + (b - a) * t);
    }
    
    // Clamp
    if (name == "clamp" && args.size() >= 3) {
        double value = args[0].numberValue;
        double minVal = args[1].numberValue;
        double maxVal = args[2].numberValue;
        return Value(std::max(minVal, std::min(maxVal, value)));
    }
    
    // Print (debug output)
    if (name == "print") {
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) std::cout << " ";
            switch (args[i].type) {
                case Value::Type::NUMBER: std::cout << args[i].numberValue; break;
                case Value::Type::STRING: std::cout << args[i].stringValue; break;
                case Value::Type::BOOL: std::cout << (args[i].boolValue ? "true" : "false"); break;
                case Value::Type::NONE: std::cout << "null"; break;
                case Value::Type::ARRAY:
                case Value::Type::TABLE:
                    std::cout << InspectValue(args[i]); break;
                default: std::cout << "<object>"; break;
            }
        }
        std::cout << std::endl;
        return Value();
    }

    // Debug  prints with type annotations for diagnostics
    if (name == "debug") {
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) std::cout << " ";
            const char* typeName = "unknown";
            switch (args[i].type) {
                case Value::Type::NUMBER: typeName = "number"; break;
                case Value::Type::STRING: typeName = "string"; break;
                case Value::Type::BOOL:   typeName = "bool"; break;
                case Value::Type::NONE:   typeName = "null"; break;
                case Value::Type::ARRAY:  typeName = "array"; break;
                case Value::Type::TABLE:  typeName = "table"; break;
                case Value::Type::OBJECT: typeName = "object"; break;
                case Value::Type::SCRIPT_INSTANCE: typeName = "instance"; break;
                case Value::Type::FUNCTION: typeName = "function"; break;
            }
            std::cout << "(" << typeName << ") " << InspectValue(args[i]);
        }
        std::cout << std::endl;
        return Value();
    }

    // Format  string formatting with {} placeholders
    if (name == "format" && !args.empty() && args[0].type == Value::Type::STRING) {
        std::string fmt = args[0].stringValue;
        std::string result;
        size_t argIdx = 1;
        for (size_t i = 0; i < fmt.size(); i++) {
            if (i + 1 < fmt.size() && fmt[i] == '{' && fmt[i + 1] == '}') {
                if (argIdx < args.size()) {
                    const Value& a = args[argIdx];
                    switch (a.type) {
                        case Value::Type::NUMBER: result += NumToStr(a.numberValue); break;
                        case Value::Type::STRING: result += a.stringValue; break;
                        case Value::Type::BOOL: result += a.boolValue ? "true" : "false"; break;
                        case Value::Type::NONE: result += "null"; break;
                        default: result += InspectValue(a); break;
                    }
                } else {
                    result += "{}";
                }
                argIdx++;
                i++; // skip '}'
            } else {
                result += fmt[i];
            }
        }
        return Value(result);
    }
    
    // Array/string length
    if (name == "length" && !args.empty()) {
        if (args[0].type == Value::Type::ARRAY) return Value((double)args[0].arrayValue.size());
        if (args[0].type == Value::Type::STRING) return Value((double)args[0].stringValue.size());
        return Value(0.0);
    }
    
    // Floor/ceil/round
    if (name == "floor" && !args.empty() && args[0].type == Value::Type::NUMBER) {
        return Value(std::floor(args[0].numberValue));
    }
    if (name == "ceil" && !args.empty() && args[0].type == Value::Type::NUMBER) {
        return Value(std::ceil(args[0].numberValue));
    }
    if (name == "round" && !args.empty() && args[0].type == Value::Type::NUMBER) {
        return Value(std::round(args[0].numberValue));
    }
    
    // Array contains: contains(arr, value) - read-only check on copy
    if (name == "contains" && args.size() >= 2 && args[0].type == Value::Type::ARRAY) {
        for (const Value& elem : args[0].arrayValue) {
            if (elem.type == args[1].type) {
                if (elem.type == Value::Type::NUMBER && elem.numberValue == args[1].numberValue) return Value(true);
                if (elem.type == Value::Type::STRING && elem.stringValue == args[1].stringValue) return Value(true);
                if (elem.type == Value::Type::BOOL && elem.boolValue == args[1].boolValue) return Value(true);
            }
        }
        return Value(false);
    }
    
    // Type checking
    if (name == "type" && !args.empty()) {
        switch (args[0].type) {
            case Value::Type::NUMBER: return Value(std::string("number"));
            case Value::Type::STRING: return Value(std::string("string"));
            case Value::Type::BOOL: return Value(std::string("bool"));
            case Value::Type::ARRAY: return Value(std::string("array"));
            case Value::Type::TABLE: return Value(std::string("table"));
            case Value::Type::FUNCTION: return Value(std::string("function"));
            case Value::Type::OBJECT: return Value(std::string("object"));
            case Value::Type::SCRIPT_INSTANCE: return Value(std::string("instance"));
            case Value::Type::NONE: return Value(std::string("null"));
        }
    }
    
    // String conversion
    if (name == "str" && !args.empty()) {
        if (args[0].type == Value::Type::STRING) return args[0];
        if (args[0].type == Value::Type::NUMBER) {
            return Value(NumToStr(args[0].numberValue));
        }
        if (args[0].type == Value::Type::BOOL) return Value(std::string(args[0].boolValue ? "true" : "false"));
        return Value(std::string("null"));
    }
    
    // Number conversion
    if (name == "num" && !args.empty()) {
        if (args[0].type == Value::Type::NUMBER) return args[0];
        if (args[0].type == Value::Type::STRING) {
            try { return Value(std::stod(args[0].stringValue)); } catch (...) { return Value(0.0); }
        }
        if (args[0].type == Value::Type::BOOL) return Value(args[0].boolValue ? 1.0 : 0.0);
        return Value(0.0);
    }
    
    // Random number generation
    if (name == "random") {
        if (args.empty()) {
            // random() -> [0.0, 1.0)
            return Value((double)rand() / ((double)RAND_MAX + 1.0));
        }
        if (args.size() == 1 && args[0].type == Value::Type::NUMBER) {
            // random(max) -> [0, max)
            return Value(((double)rand() / ((double)RAND_MAX + 1.0)) * args[0].numberValue);
        }
        if (args.size() >= 2 && args[0].type == Value::Type::NUMBER && args[1].type == Value::Type::NUMBER) {
            // random(min, max) -> [min, max)
            double mn = args[0].numberValue, mx = args[1].numberValue;
            return Value(mn + ((double)rand() / ((double)RAND_MAX + 1.0)) * (mx - mn));
        }
        return Value(0.0);
    }
    
    // map(value, inMin, inMax, outMin, outMax) -> mapped value
    if (name == "map" && args.size() >= 5) {
        double v = args[0].numberValue;
        double inMin = args[1].numberValue;
        double inMax = args[2].numberValue;
        double outMin = args[3].numberValue;
        double outMax = args[4].numberValue;
        if (inMax == inMin) return Value(outMin);
        return Value(outMin + (v - inMin) * (outMax - outMin) / (inMax - inMin));
    }
    
    // degrees(radians) -> degrees
    if (name == "degrees" && args.size() >= 1 && args[0].type == Value::Type::NUMBER) {
        return Value(args[0].numberValue * (180.0 / 3.14159265358979323846));
    }
    
    // radians(degrees) -> radians
    if (name == "radians" && args.size() >= 1 && args[0].type == Value::Type::NUMBER) {
        return Value(args[0].numberValue * (3.14159265358979323846 / 180.0));
    }
    
    // Signal builtins: connect(signalName, handlerName)
    if (name == "connect" && args.size() >= 2 && args[0].type == Value::Type::STRING && args[1].type == Value::Type::STRING) {
        signalRegistry_.Connect(args[0].stringValue, args[1].stringValue);
        return Value();
    }
    if (name == "disconnect" && args.size() >= 2 && args[0].type == Value::Type::STRING && args[1].type == Value::Type::STRING) {
        signalRegistry_.Disconnect(args[0].stringValue, args[1].stringValue);
        return Value();
    }
    if (name == "connect_once" && args.size() >= 2 && args[0].type == Value::Type::STRING && args[1].type == Value::Type::STRING) {
        signalRegistry_.ConnectOnce(args[0].stringValue, args[1].stringValue);
        return Value();
    }

    // Profiler builtins
    if (name == "profiler_fps") {
        return Value(static_cast<double>(Profiler::GetInstance().GetFPS()));
    }
    if (name == "profiler_time" && args.size() >= 1 && args[0].type == Value::Type::STRING) {
        const auto* stats = Profiler::GetInstance().GetStats(args[0].stringValue);
        return Value(stats ? static_cast<double>(stats->avgTime) : 0.0);
    }
    if (name == "profiler_enable") {
        bool enable = true;
        if (args.size() >= 1 && args[0].type == Value::Type::NUMBER) {
            enable = args[0].numberValue != 0.0;
        }
        if (enable) Profiler::GetInstance().Enable();
        else Profiler::GetInstance().Disable();
        return Value();
    }
    
    // Module system builtins
    if (name == "has_module") {
        if (args.size() >= 1 && args[0].type == Value::Type::STRING) {
            return Value(moduleLoader_.HasModule(args[0].stringValue) ? 1.0 : 0.0);
        }
        return Value(0.0);
    }
    
    if (name == "list_modules") {
        auto names = moduleLoader_.ListModules();
        Value result;
        result.type = Value::Type::ARRAY;
        for (auto& n : names) {
            result.arrayValue.push_back(Value(n));
        }
        return result;
    }

    if (name == "start_coroutine" && !args.empty() && args[0].type == Value::Type::STRING) {
        coroutineManager_.Start(args[0].stringValue);
        return Value(1.0);
    }
    if (name == "stop_coroutine" && !args.empty() && args[0].type == Value::Type::STRING) {
        return Value(coroutineManager_.StopByName(args[0].stringValue) ? 1.0 : 0.0);
    }

    if (name == "stop_all_coroutines") {
        coroutineManager_.Clear();
        return Value(1.0);
    }

    return Value();
}

} // namespace scripting
} // namespace koilo
