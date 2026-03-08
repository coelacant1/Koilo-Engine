// SPDX-License-Identifier: GPL-3.0-or-later
// Strings, Collections & Builtins
#include "helpers.hpp"

void TestStringOps() {
    std::cout << "=== String Operations ===" << std::endl;
    
    TEST("string concatenation");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var greeting = \"hello\" + \" \" + \"world\";\nprint(greeting);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("string comparison");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var name = \"ptx\";\nif name == \"ptx\" {\nprint(\"match!\");\n}");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
}

void TestBuiltins() {
    std::cout << "=== Built-in Functions ===" << std::endl;
    
    TEST("length of array");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var len = length([1, 2, 3]);\nprint(len);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("floor/ceil/round");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("print(floor(3.7));\nprint(ceil(3.2));\nprint(round(3.5));");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
}

void TestTableOps() {
    std::cout << "=== Table Operations ===" << std::endl;
    
    TEST("table literal and read");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var config = { speed: 5.0, name: \"player\" };\nprint(config.speed);\nprint(config.name);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("table property write");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var config = { hp: 100 };\nconfig.hp = 50;\nprint(config.hp);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("table add new property");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var config = { hp: 100 };\nconfig.armor = 25;\nprint(config.armor);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("table has method");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var t = { a: 1 };\nprint(t.has(\"a\"));\nprint(t.has(\"b\"));");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
}

void TestArrayOps() {
    std::cout << "=== Array Operations ===" << std::endl;
    
    TEST("mixed-type array");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var arr = [1, \"hello\", true];\nprint(arr[0]);\nprint(arr[1]);\nprint(arr[2]);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("array push");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var arr = [1, 2];\narr.push(3);\nprint(length(arr));\nprint(arr[2]);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("array pop");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var arr = [10, 20, 30];\nvar last = arr.pop();\nprint(last);\nprint(length(arr));");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("array remove");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var arr = [\"a\", \"b\", \"c\"];\narr.remove(1);\nprint(arr[0]);\nprint(arr[1]);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("array contains");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var arr = [1, 2, 3];\nprint(arr.contains(2));\nprint(arr.contains(5));");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("for..in mixed array");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var items = [\"apple\", \"banana\", \"cherry\"];\nfor item in items {\nprint(item);\n}");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
}

void TestColorBuiltins() {
    std::cout << "=== Color Builtins ===" << std::endl;
    
    TEST("Color888.FromHSV creates Color888");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var c = Color888.FromHSV(0, 1.0, 1.0);
var r_val = c.r;
var g_val = c.g;
var b_val = c.b;
)";
        if (RunScript(script, reader, engine)) {
            Value r = engine.GetGlobal("r_val");
            Value g = engine.GetGlobal("g_val");
            Value b = engine.GetGlobal("b_val");
            // HSV(0, 1, 1) = pure red = (255, 0, 0)
            if (r.type == Value::Type::NUMBER && r.numberValue == 255
                && g.type == Value::Type::NUMBER && g.numberValue == 0
                && b.type == Value::Type::NUMBER && b.numberValue == 0) {
                PASS();
            } else {
                FAIL("Color888.FromHSV(0,1,1) expected (255,0,0) got (" + std::to_string(r.numberValue) + "," + std::to_string(g.numberValue) + "," + std::to_string(b.numberValue) + ")");
            }
        }
    }
    
    TEST("Color888.FromHSV green");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var c = Color888.FromHSV(120, 1.0, 1.0);
var r_val = c.r;
var g_val = c.g;
var b_val = c.b;
)";
        if (RunScript(script, reader, engine)) {
            Value r = engine.GetGlobal("r_val");
            Value g = engine.GetGlobal("g_val");
            // HSV(120, 1, 1) = pure green
            if (r.numberValue == 0 && g.numberValue == 255) {
                PASS();
            } else {
                FAIL("Color888.FromHSV(120,1,1) expected green");
            }
        }
    }
    
    TEST("Color888 HueShift via instance method");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var c = Color888(255, 0, 0);
var shifted = c.HueShift(120.0);
var g_val = shifted.g;
)";
        if (RunScript(script, reader, engine)) {
            Value g = engine.GetGlobal("g_val");
            // Red shifted 120 degrees = green (g should be 255 or close)
            if (g.type == Value::Type::NUMBER && g.numberValue > 200) {
                PASS();
            } else {
                FAIL("HueShift(120) expected green-ish g=" + std::to_string(g.numberValue));
            }
        }
    }
    
    TEST("Color888.Lerp via static method");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var a = Color888(0, 0, 0);
var b = Color888(200, 200, 200);
var c = Color888.Lerp(a, b, 0.5);
var r_val = c.r;
)";
        if (RunScript(script, reader, engine)) {
            Value r = engine.GetGlobal("r_val");
            // Lerp 0-200 at 0.5 = 100
            if (r.type == Value::Type::NUMBER && r.numberValue == 100) {
                PASS();
            } else {
                FAIL("Color888.Lerp expected r=100, got " + std::to_string(r.numberValue));
            }
        }
    }
    
    TEST("Color888 HueShift via reflection");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var c = Color888(255, 0, 0);
var shifted = c.HueShift(120.0);
var g_val = shifted.g;
)";
        if (RunScript(script, reader, engine)) {
            Value g = engine.GetGlobal("g_val");
            if (g.type == Value::Type::NUMBER && g.numberValue > 200) {
                PASS();
            } else {
                FAIL("Color888.HueShift reflection expected green-ish g=" + std::to_string(g.numberValue));
            }
        }
    }
}

void TestPhase24Builtins() {
    std::cout << "--- Missing Builtins ---" << std::endl;
    
    TEST("map() builtin linear mapping")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent("var result = 0;\nfn Setup() {\n  result = map(5, 0, 10, 0, 100);\n}\n");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value v = engine.GetGlobal("result");
        if (ok && std::abs(v.numberValue - 50.0) < 0.01) PASS()
        else FAIL("map(5,0,10,0,100) expected 50, got " + std::to_string(v.numberValue))
    }
    
    TEST("map() builtin negative range")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent("var result = 0;\nfn Setup() {\n  result = map(0.5, 0, 1, -10, 10);\n}\n");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value v = engine.GetGlobal("result");
        if (ok && std::abs(v.numberValue - 0.0) < 0.01) PASS()
        else FAIL("map(0.5,0,1,-10,10) expected 0, got " + std::to_string(v.numberValue))
    }
    
    TEST("degrees() builtin")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent("var result = 0;\nfn Setup() {\n  result = degrees(3.14159265358979);\n}\n");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value v = engine.GetGlobal("result");
        if (ok && std::abs(v.numberValue - 180.0) < 0.01) PASS()
        else FAIL("degrees(pi) expected 180, got " + std::to_string(v.numberValue))
    }
    
    TEST("radians() builtin")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent("var result = 0;\nfn Setup() {\n  result = radians(180);\n}\n");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value v = engine.GetGlobal("result");
        if (ok && std::abs(v.numberValue - 3.14159) < 0.001) PASS()
        else FAIL("radians(180) expected pi, got " + std::to_string(v.numberValue))
    }
    
    TEST("degrees/radians round-trip")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent("var result = 0;\nfn Setup() {\n  result = degrees(radians(45.0));\n}\n");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value v = engine.GetGlobal("result");
        if (ok && std::abs(v.numberValue - 45.0) < 0.001) PASS()
        else FAIL("degrees(radians(45)) expected 45, got " + std::to_string(v.numberValue))
    }
}



