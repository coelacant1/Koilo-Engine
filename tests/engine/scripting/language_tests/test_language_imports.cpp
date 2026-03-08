// SPDX-License-Identifier: GPL-3.0-or-later
// Module / Import System
#include "helpers.hpp"

void TestPhase32Imports() {
    std::cout << "\n--- Module/Import System ---" << std::endl;
    
    // Test 1: Import functions from another file
    {
        TEST("Import functions from module");
        MultiFileReader reader;
        reader.AddFile("scripts/main.ks", R"(
            import "math_utils.ks";
            var r = add(3, 4);
            var r2 = mul(5, 6);
        )");
        reader.AddFile("scripts/math_utils.ks", R"(
            fn add(a, b) { return a + b; }
            fn mul(a, b) { return a * b; }
        )");
        KoiloScriptEngine engine(&reader);
        if (engine.LoadScript("scripts/main.ks") && engine.BuildScene()) {
            Value r = engine.GetGlobal("r");
            Value r2 = engine.GetGlobal("r2");
            if (r.type == Value::Type::NUMBER && r.numberValue == 7.0 &&
                r2.type == Value::Type::NUMBER && r2.numberValue == 30.0) {
                PASS();
            } else {
                FAIL("Expected r=7, r2=30, got r=" + std::to_string(r.numberValue) +
                     " r2=" + std::to_string(r2.numberValue));
            }
        }
    }
    
    // Test 2: Import classes from another file
    {
        TEST("Import classes from module");
        MultiFileReader reader;
        reader.AddFile("scripts/main.ks", R"(
            import "point.ks";
            var p = new Point(10, 20);
            var s = p.sum();
        )");
        reader.AddFile("scripts/point.ks", R"(
            class Point {
                fn __init(x, y) {
                    self.x = x;
                    self.y = y;
                }
                fn sum() {
                    return self.x + self.y;
                }
            }
        )");
        KoiloScriptEngine engine(&reader);
        if (engine.LoadScript("scripts/main.ks") && engine.BuildScene()) {
            Value s = engine.GetGlobal("s");
            if (s.type == Value::Type::NUMBER && s.numberValue == 30.0) {
                PASS();
            } else {
                FAIL("Expected s=30, got " + std::to_string(s.numberValue));
            }
        }
    }
    
    // Test 3: Circular import detection (should not infinite-loop)
    {
        TEST("Circular import handled gracefully");
        MultiFileReader reader;
        reader.AddFile("scripts/a.ks", R"(
            import "b.ks";
            fn fromA() { return 1; }
        )");
        reader.AddFile("scripts/b.ks", R"(
            import "a.ks";
            fn fromB() { return 2; }
        )");
        KoiloScriptEngine engine(&reader);
        // Should not hang - circular import is silently skipped
        if (engine.LoadScript("scripts/a.ks") && engine.BuildScene()) {
            PASS();
        }
    }
    
    // Test 4: Missing import file error
    {
        TEST("Missing import file produces error");
        MultiFileReader reader;
        reader.AddFile("scripts/main.ks", R"(
            import "nonexistent.ks";
            fn Setup() {}
        )");
        KoiloScriptEngine engine(&reader);
        bool loaded = engine.LoadScript("scripts/main.ks");
        if (!loaded) {
            std::string err = engine.GetError();
            if (err.find("nonexistent.ks") != std::string::npos) {
                PASS();
            } else {
                FAIL("Error didn't mention file: " + err);
            }
        } else {
            FAIL("Expected LoadScript to fail for missing import");
        }
    }
    
    // Test 5: Transitive imports (A imports B, B imports C)
    {
        TEST("Transitive imports (A->B->C)");
        MultiFileReader reader;
        reader.AddFile("scripts/main.ks", R"(
            import "mid.ks";
            var r = triple(5);
        )");
        reader.AddFile("scripts/mid.ks", R"(
            import "base.ks";
            fn triple(x) { return double(x) + x; }
        )");
        reader.AddFile("scripts/base.ks", R"(
            fn double(x) { return x * 2; }
        )");
        KoiloScriptEngine engine(&reader);
        if (engine.LoadScript("scripts/main.ks") && engine.BuildScene()) {
            Value r = engine.GetGlobal("r");
            if (r.type == Value::Type::NUMBER && r.numberValue == 15.0) {
                PASS();
            } else {
                FAIL("Expected 15, got " + std::to_string(r.numberValue));
            }
        }
    }
    
    // Test 6: Multiple imports in one file
    {
        TEST("Multiple imports in one file");
        MultiFileReader reader;
        reader.AddFile("scripts/main.ks", R"(
            import "alpha.ks";
            import "beta.ks";
            var r = getAlpha() + getBeta();
        )");
        reader.AddFile("scripts/alpha.ks", R"(
            fn getAlpha() { return 100; }
        )");
        reader.AddFile("scripts/beta.ks", R"(
            fn getBeta() { return 200; }
        )");
        KoiloScriptEngine engine(&reader);
        if (engine.LoadScript("scripts/main.ks") && engine.BuildScene()) {
            Value r = engine.GetGlobal("r");
            if (r.type == Value::Type::NUMBER && r.numberValue == 300.0) {
                PASS();
            } else {
                FAIL("Expected 300, got " + std::to_string(r.numberValue));
            }
        }
    }
}


