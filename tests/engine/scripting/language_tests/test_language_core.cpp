// SPDX-License-Identifier: GPL-3.0-or-later
// Core Language Features
#include "helpers.hpp"

void TestVarDeclarations() {
    std::cout << "=== Variable Declarations ===" << std::endl;
    
    TEST("var with number");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var x = 42;\nprint(x);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("var with string");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var msg = \"hello\";\nprint(msg);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("var with expression");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var a = 10;\nvar b = 20;\nvar c = a + b;\nprint(c);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
}

void TestWhileLoops() {
    std::cout << "=== While Loops ===" << std::endl;
    
    TEST("basic while loop");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var i = 0;\nwhile i < 5 {\ni = i + 1;\n}\nprint(i);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("while with parens");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var count = 0;\nwhile (count < 3) {\ncount = count + 1;\n}\nprint(count);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
}

void TestForLoops() {
    std::cout << "=== For..In Loops ===" << std::endl;
    
    TEST("for..in array");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var sum = 0;\nfor x in [1, 2, 3, 4, 5] {\nsum = sum + x;\n}\nprint(sum);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
}

void TestBreakContinue() {
    std::cout << "=== Break/Continue ===" << std::endl;

    TEST("break exits while loop early");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        // Declare at top level so they're globals
        std::string script =
            "var i = 0;\n"
            "fn Setup() {\n"
            "  while i < 100 {\n"
            "    if (i == 5) { break; }\n"
            "    i = i + 1;\n"
            "  }\n"
            "}\n";
        if (RunScript(script, reader, engine)) {
            auto val = engine.GetScriptVariable("i");
            if (val.numberValue == 5.0) { PASS(); }
            else { FAIL("Expected i==5, got " + std::to_string(val.numberValue)); }
        }
    }

    TEST("continue skips iteration in while loop");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script =
            "var sum = 0;\n"
            "var i = 0;\n"
            "fn Setup() {\n"
            "  while i < 10 {\n"
            "    i = i + 1;\n"
            "    if (i == 5) { continue; }\n"
            "    sum = sum + i;\n"
            "  }\n"
            "}\n";
        if (RunScript(script, reader, engine)) {
            auto val = engine.GetScriptVariable("sum");
            // sum = 1+2+3+4+6+7+8+9+10 = 50
            if (val.numberValue == 50.0) { PASS(); }
            else { FAIL("Expected sum==50, got " + std::to_string(val.numberValue)); }
        }
    }

    TEST("break exits for-each loop early");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script =
            "var result = 0;\n"
            "fn Setup() {\n"
            "  for x in [10, 20, 30, 40, 50] {\n"
            "    if (x == 30) { break; }\n"
            "    result = result + x;\n"
            "  }\n"
            "}\n";
        if (RunScript(script, reader, engine)) {
            auto val = engine.GetScriptVariable("result");
            // result = 10 + 20 = 30
            if (val.numberValue == 30.0) { PASS(); }
            else { FAIL("Expected result==30, got " + std::to_string(val.numberValue)); }
        }
    }

    TEST("continue skips iteration in for-each loop");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script =
            "var sum = 0;\n"
            "fn Setup() {\n"
            "  for x in [1, 2, 3, 4, 5] {\n"
            "    if (x == 3) { continue; }\n"
            "    sum = sum + x;\n"
            "  }\n"
            "}\n";
        if (RunScript(script, reader, engine)) {
            auto val = engine.GetScriptVariable("sum");
            // sum = 1+2+4+5 = 12
            if (val.numberValue == 12.0) { PASS(); }
            else { FAIL("Expected sum==12, got " + std::to_string(val.numberValue)); }
        }
    }

    TEST("break outside loop is compile error");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("break;");
        reader.SetContent(script);
        bool loaded = engine.LoadScript("test.ks");
        if (!loaded && std::string(engine.GetError()).find("outside of loop") != std::string::npos) {
            PASS();
        } else {
            FAIL("Expected compile error for break outside loop");
        }
    }

    TEST("continue outside loop is compile error");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("continue;");
        reader.SetContent(script);
        bool loaded = engine.LoadScript("test.ks");
        if (!loaded && std::string(engine.GetError()).find("outside of loop") != std::string::npos) {
            PASS();
        } else {
            FAIL("Expected compile error for continue outside loop");
        }
    }
}

void TestFunctions() {
    std::cout << "=== User Functions ===" << std::endl;
    
    TEST("function declaration and call");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapWithFunctions(
            "function add(a, b) {\nreturn a + b;\n}",
            "var result = add(3, 4);\nprint(result);"
        );
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("recursive function");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapWithFunctions(
            "function factorial(n) {\nif n <= 1 {\nreturn 1;\n}\nreturn n * factorial(n - 1);\n}",
            "var result = factorial(5);\nprint(result);"
        );
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
}

void TestLogicalOps() {
    std::cout << "=== Logical Operators ===" << std::endl;
    
    TEST("and operator");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var x = 1;\nvar y = 2;\nif x > 0 and y > 0 {\nprint(\"both positive\");\n}");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("or operator");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var x = 0;\nvar y = 1;\nif x > 0 or y > 0 {\nprint(\"at least one positive\");\n}");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("not operator");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var done = false;\nif not done {\nprint(\"not done yet\");\n}");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
}

void TestTernary() {
    std::cout << "=== Ternary Operator ===" << std::endl;
    
    TEST("ternary expression");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var x = 10;\nvar result = x > 5 ? 1 : 0;\nprint(result);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
}

void TestTypeUtils() {
    std::cout << "=== Type Utilities ===" << std::endl;
    
    TEST("type function");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("print(type(42));\nprint(type(\"hello\"));\nprint(type(true));\nprint(type([1,2]));");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("str/num conversion");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var s = str(42);\nvar n = num(\"3.14\");\nprint(s);\nprint(n);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
}

void TestChainedCalls() {
    std::cout << "=== Chained Method Calls ===" << std::endl;
    
    TEST("simple method call on reflected object");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        
        // Register a Vector3D object
        koilo::Vector3D vec(3.0f, 4.0f, 0.0f);
        const auto& desc = koilo::Vector3D::Describe();
        engine.RegisterObject("myVec", &vec, &desc);
        
        // Call Magnitude() - should return 5.0
        std::string script = WrapInSetup("var m = myVec.Magnitude();\nprint(m);");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("null chain safety");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var result = nonexistent.DoSomething();");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
}

void TestConstructorExpressions() {
    std::cout << "=== Constructor Expressions ===" << std::endl;
    
    TEST("Vector3D constructor");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var v = Vector3D(1.0, 2.0, 3.0);\nprint(type(v));");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
    
    TEST("default constructor");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = WrapInSetup("var v = Vector3D();\nprint(type(v));");
        if (RunScript(script, reader, engine)) {
            PASS();
        }
    }
}

void TestTempCleanup() {
    std::cout << "=== Temp Object Lifecycle ===" << std::endl;
    
    TEST("temp objects cleaned up after update");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
fn Update(dt) {
    var v = Vector3D(1.0, 2.0, 3.0);
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            // Run 100 frames - should not leak
            for (int i = 0; i < 100; ++i) {
                koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
                if (engine.HasError()) {
                    FAIL(engine.GetError());
                    break;
                }
            }
            if (!engine.HasError()) {
                PASS();
            }
        } else {
            FAIL("Script load/build failed");
        }
    }
}

void TestPersistentObjects() {
    std::cout << "=== Persistent Constructed Objects ===" << std::endl;
    
    StringFileReader reader;
    
    // Test 1: Object assigned to global variable persists across frames
    {
        TEST("persistent object across frames");
        KoiloScriptEngine engine(&reader);
        const char* script = R"(
fn Update(dt) {
    if type(_vec) == "null" {
        _vec = Vector3D(3.0, 4.0, 0.0);
    }
    _result = _vec.Magnitude();
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            // Frame 1: creates and assigns _vec
            koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
            if (engine.HasError()) { FAIL(engine.GetError()); }
            else {
                auto val = engine.GetGlobal("_result");
                if (val.type == Value::Type::NUMBER && std::abs(val.numberValue - 5.0) < 0.01) {
                    // Frame 2: _vec should still exist
                    koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
                    if (engine.HasError()) { FAIL(engine.GetError()); }
                    else {
                        auto val2 = engine.GetGlobal("_result");
                        if (val2.type == Value::Type::NUMBER && std::abs(val2.numberValue - 5.0) < 0.01) {
                            PASS();
                        } else {
                            FAIL("object did not persist: frame 2 result = " + std::to_string(val2.numberValue));
                        }
                    }
                } else {
                    FAIL("frame 1 failed: result = " + std::to_string(val.numberValue));
                }
            }
        } else {
            FAIL("Script load/build failed");
        }
    }
    
    // Test 2: Overwriting a persistent object cleans up the old one
    {
        TEST("persistent object overwrite cleanup");
        KoiloScriptEngine engine(&reader);
        const char* script = R"(
fn Update(dt) {
    _vec = Vector3D(3.0, 4.0, 0.0);
    _vec = Vector3D(5.0, 12.0, 0.0);
    _result = _vec.Magnitude();
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            for (int i = 0; i < 50; ++i) {
                koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
                if (engine.HasError()) { FAIL(engine.GetError()); break; }
            }
            if (!engine.HasError()) {
                auto val = engine.GetGlobal("_result");
                if (val.type == Value::Type::NUMBER && std::abs(val.numberValue - 13.0) < 0.01) {
                    PASS();
                } else {
                    FAIL("overwrite result wrong: " + std::to_string(val.numberValue));
                }
            }
        } else {
            FAIL("Script load/build failed");
        }
    }
    
    // Test 3: var declaration with constructor also persists in scope
    {
        TEST("var declaration with constructor");
        KoiloScriptEngine engine(&reader);
        const char* script = R"(
fn Update(dt) {
    if type(_initialized) == "null" {
        var v = Vector3D(7.0, 8.0, 9.0);
        _globalRef = v;
        _initialized = 1;
    }
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
            if (engine.HasError()) { FAIL(engine.GetError()); }
            else {
                // _globalRef should hold the object
                auto val = engine.GetGlobal("_globalRef");
                if (val.type == Value::Type::OBJECT) {
                    PASS();
                } else {
                    FAIL("global ref not an object");
                }
            }
        } else {
            FAIL("Script load/build failed");
        }
    }
    
    // Test 4: Temp objects still cleaned up when NOT assigned
    {
        TEST("temp objects still cleaned (not assigned)");
        KoiloScriptEngine engine(&reader);
        const char* script = R"(
fn Update(dt) {
    var mag = Vector3D(3.0, 4.0, 0.0).Magnitude();
    _result = mag;
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            for (int i = 0; i < 100; ++i) {
                koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
                if (engine.HasError()) { FAIL(engine.GetError()); break; }
            }
            if (!engine.HasError()) {
                auto val = engine.GetGlobal("_result");
                if (val.type == Value::Type::NUMBER && std::abs(val.numberValue - 5.0) < 0.01) {
                    PASS();
                } else {
                    FAIL("magnitude wrong: " + std::to_string(val.numberValue));
                }
            }
        } else {
            FAIL("Script load/build failed");
        }
    }
}


