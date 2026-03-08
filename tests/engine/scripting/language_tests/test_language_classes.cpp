// SPDX-License-Identifier: GPL-3.0-or-later
// Classes, OOP & Code-First Syntax
#include "helpers.hpp"

void TestCodeFirstSyntax() {
    std::cout << "=== Code-First Syntax ===" << std::endl;

    // Test 1: top-level var declarations
    {
        TEST("top-level var declaration");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var x = 42;
var name = "hello";
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            Value x = engine.GetGlobal("x");
            Value name = engine.GetGlobal("name");
            if (x.type == Value::Type::NUMBER && x.numberValue == 42 &&
                name.type == Value::Type::STRING && name.stringValue == "hello") {
                PASS();
            } else { FAIL("var values incorrect"); }
        } else { FAIL(engine.GetError()); }
    }

    // Test 2: fn keyword (alias for function)
    {
        TEST("fn keyword for functions");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var result = 0;

fn add(a, b) {
    return a + b;
}

fn Update(dt) {
    result = add(3, 4);
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
            Value result = engine.GetGlobal("result");
            if (result.type == Value::Type::NUMBER && result.numberValue == 7) {
                PASS();
            } else { FAIL("fn keyword not working"); }
        } else { FAIL(engine.GetError()); }
    }

    // Test 3: fn Update() replaces UPDATE block
    {
        TEST("fn Update() replaces UPDATE block");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var counter = 0;

fn Update(dt) {
    counter = counter + 1;
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
            koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
            koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
            Value counter = engine.GetGlobal("counter");
            if (counter.type == Value::Type::NUMBER && counter.numberValue == 3) {
                PASS();
            } else { FAIL("fn Update not called"); }
        } else { FAIL(engine.GetError()); }
    }

    // Test 4: top-level constructor expression
    {
        TEST("top-level constructor + method call");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var myVec = Vector3D(1, 2, 3);
var mag = 0;

fn Update(dt) {
    mag = myVec.Magnitude();
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
            Value mag = engine.GetGlobal("mag");
            if (mag.type == Value::Type::NUMBER && std::abs(mag.numberValue - 3.7416573) < 0.01) {
                PASS();
            } else { FAIL("constructor or method call failed"); }
        } else { FAIL(engine.GetError()); }
    }

    // Test 5: top-level method calls (init code)
    {
        TEST("top-level init code execution");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var a = 10;
var b = a + 5;
var c = b * 2;
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            Value c = engine.GetGlobal("c");
            if (c.type == Value::Type::NUMBER && c.numberValue == 30) {
                PASS();
            } else { FAIL("init code not evaluated correctly"); }
        } else { FAIL(engine.GetError()); }
    }

    // Test 6: all code-first functions
    {
        TEST("multiple fn declarations");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var x = 99;

function legacyFn() {
    return 42;
}

fn newFn() {
    return x + 1;
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            // Both functions should be registered
            Value legacy = engine.GetGlobal("legacyFn");
            Value newF = engine.GetGlobal("newFn");
            if (legacy.type == Value::Type::FUNCTION && newF.type == Value::Type::FUNCTION) {
                PASS();
            } else { FAIL("functions not registered"); }
        } else { FAIL(engine.GetError()); }
    }

    // Test 7: fully code-first script (no legacy blocks except DISPLAY)
    {
        TEST("fully code-first script");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var speed = 2.5;
var position = 0;

fn moveRight() {
    position = position + speed;
}

fn moveLeft() {
    position = position - speed;
}

fn Update(dt) {
    if position < 100 {
        moveRight();
    }
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
            koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
            Value pos = engine.GetGlobal("position");
            if (pos.type == Value::Type::NUMBER && pos.numberValue == 5.0) {
                PASS();
            } else { FAIL("code-first script execution failed"); }
        } else { FAIL(engine.GetError()); }
    }

    // Test 8: OnKeyPress input handler
    {
        TEST("OnKeyPress input dispatch");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var lastKey = "";

fn OnKeyPress(key) {
    lastKey = key;
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            engine.HandleInput("key", "a");
            Value lastKey = engine.GetGlobal("lastKey");
            if (lastKey.type == Value::Type::STRING && lastKey.stringValue == "a") {
                PASS();
            } else { FAIL("OnKeyPress not dispatched"); }
        } else { FAIL(engine.GetError()); }
    }

    // Test 9: CallFunction from host
    {
        TEST("CallFunction from host code");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var total = 0;

fn addAmount(n) {
    total = total + n;
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            engine.CallFunction("addAmount", { Value(10.0) });
            engine.CallFunction("addAmount", { Value(5.0) });
            Value total = engine.GetGlobal("total");
            if (total.type == Value::Type::NUMBER && total.numberValue == 15) {
                PASS();
            } else { FAIL("CallFunction not working"); }
        } else { FAIL(engine.GetError()); }
    }
}

// ============================================================================
// C Tests
// ============================================================================

void TestPhase31ScriptClasses() {
    std::cout << "  --- Script Classes ---" << std::endl;
    
    // Test 1: Basic class with fields
    {
        TEST("class with default fields");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
class Dog {
    var name = "Rex";
    var age = 3;
}
var d = new Dog();
var n = d.name;
var a = d.age;
fn Setup() {}
fn Update(dt) {}
)";
        if (RunScript(script, reader, engine)) {
            Value n = engine.GetGlobal("n");
            Value a = engine.GetGlobal("a");
            if (n.type == Value::Type::STRING && n.stringValue == "Rex" &&
                a.type == Value::Type::NUMBER && a.numberValue == 3.0) {
                PASS();
            } else {
                FAIL("fields wrong: name=" + n.stringValue + " age=" + std::to_string(a.numberValue));
            }
        } else FAIL(std::string("script failed: ") + engine.GetError())
    }
    
    // Test 2: Class with methods
    {
        TEST("class with method");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
class Counter {
    var count = 0;
    fn Increment() {
        self.count = self.count + 1;
    }
}
var c = new Counter();
c.Increment();
c.Increment();
c.Increment();
var result = c.count;
fn Setup() {}
fn Update(dt) {}
)";
        if (RunScript(script, reader, engine)) {
            Value r = engine.GetGlobal("result");
            if (r.type == Value::Type::NUMBER && r.numberValue == 3.0) {
                PASS();
            } else {
                FAIL("expected 3, got " + std::to_string(r.numberValue));
            }
        } else FAIL(std::string("script failed: ") + engine.GetError())
    }
    
    // Test 3: Method with parameters
    {
        TEST("class method with params");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
class Wallet {
    var balance = 100;
    fn Withdraw(amount) {
        self.balance = self.balance - amount;
    }
    fn Deposit(amount) {
        self.balance = self.balance + amount;
    }
}
var w = new Wallet();
w.Withdraw(30);
w.Deposit(50);
var result = w.balance;
fn Setup() {}
fn Update(dt) {}
)";
        if (RunScript(script, reader, engine)) {
            Value r = engine.GetGlobal("result");
            if (r.type == Value::Type::NUMBER && r.numberValue == 120.0) {
                PASS();
            } else {
                FAIL("expected 120, got " + std::to_string(r.numberValue));
            }
        } else FAIL(std::string("script failed: ") + engine.GetError())
    }
    
    // Test 4: Method with return value
    {
        TEST("class method with return");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
class Calculator {
    var value = 0;
    fn Add(x) {
        self.value = self.value + x;
        return self.value;
    }
}
var calc = new Calculator();
calc.Add(10);
var result = calc.Add(5);
fn Setup() {}
fn Update(dt) {}
)";
        if (RunScript(script, reader, engine)) {
            Value r = engine.GetGlobal("result");
            if (r.type == Value::Type::NUMBER && r.numberValue == 15.0) {
                PASS();
            } else {
                FAIL("expected 15, got " + std::to_string(r.numberValue));
            }
        } else FAIL(std::string("script failed: ") + engine.GetError())
    }
    
    // Test 5: Multiple instances
    {
        TEST("multiple class instances");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
class Pair {
    var x = 0;
    var y = 0;
}
var a = new Pair();
var b = new Pair();
a.x = 10;
a.y = 20;
b.x = 30;
b.y = 40;
var ax = a.x;
var ay = a.y;
var bx = b.x;
var by = b.y;
fn Setup() {}
fn Update(dt) {}
)";
        if (RunScript(script, reader, engine)) {
            Value ax = engine.GetGlobal("ax");
            Value ay = engine.GetGlobal("ay");
            Value bx = engine.GetGlobal("bx");
            Value by = engine.GetGlobal("by");
            if (ax.numberValue == 10.0 && ay.numberValue == 20.0 &&
                bx.numberValue == 30.0 && by.numberValue == 40.0) {
                PASS();
            } else {
                FAIL("instance isolation failed");
            }
        } else FAIL(std::string("script failed: ") + engine.GetError())
    }
    
    // Test 6: typeof returns "instance"
    {
        TEST("typeof script instance");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
class Empty {}
var e = new Empty();
var t = type(e);
fn Setup() {}
fn Update(dt) {}
)";
        if (RunScript(script, reader, engine)) {
            Value t = engine.GetGlobal("t");
            if (t.type == Value::Type::STRING && t.stringValue == "instance") {
                PASS();
            } else {
                FAIL("expected 'instance', got '" + t.stringValue + "'");
            }
        } else FAIL(std::string("script failed: ") + engine.GetError())
    }
}

void TestPhase24StaticMethods() {
    std::cout << "--- Static Method Invocation ---" << std::endl;
    
    TEST("Mathematics.Lerp static call returns correct value")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent("var result = 0;\nfn Setup() {\n  result = Mathematics.Lerp(0.0, 10.0, 0.5);\n}\n");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value v = engine.GetGlobal("result");
        if (ok && v.type == Value::Type::NUMBER && std::abs(v.numberValue - 5.0) < 0.01) PASS()
        else FAIL("Mathematics.Lerp(0,10,0.5) expected 5, got " + std::to_string(v.numberValue))
    }
    
    TEST("Mathematics.SmoothStep static call")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent("var result = 0;\nfn Setup() {\n  result = Mathematics.SmoothStep(0.0, 1.0, 0.5);\n}\n");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value v = engine.GetGlobal("result");
        if (ok && v.type == Value::Type::NUMBER && v.numberValue > 0.4 && v.numberValue < 0.6) PASS()
        else FAIL("Mathematics.SmoothStep expected ~0.5, got " + std::to_string(v.numberValue))
    }
    
    TEST("Mathematics.Sign static call")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent("var a = 0;\nvar b = 0;\nfn Setup() {\n  a = Mathematics.Sign(-5.0);\n  b = Mathematics.Sign(3.0);\n}\n");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value va = engine.GetGlobal("a");
        Value vb = engine.GetGlobal("b");
        if (ok && va.numberValue < 0 && vb.numberValue > 0) PASS()
        else FAIL("Sign(-5)=" + std::to_string(va.numberValue) + " Sign(3)=" + std::to_string(vb.numberValue))
    }
    
    TEST("Mathematics.Pow static call")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent("var result = 0;\nfn Setup() {\n  result = Mathematics.Pow(2.0, 8.0);\n}\n");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value v = engine.GetGlobal("result");
        if (ok && std::abs(v.numberValue - 256.0) < 0.01) PASS()
        else FAIL("Mathematics.Pow(2,8) expected 256, got " + std::to_string(v.numberValue))
    }
    
    TEST("Mathematics.PingPong static call")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent("var result = 0;\nfn Setup() {\n  result = Mathematics.PingPong(3.0, 2.0);\n}\n");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value v = engine.GetGlobal("result");
        if (ok && v.type == Value::Type::NUMBER && std::abs(v.numberValue - 1.0) < 0.01) PASS()
        else FAIL("Mathematics.PingPong(3,2) expected 1, got " + std::to_string(v.numberValue))
    }
    
    TEST("Mathematics.CosineInterpolation static call")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent("var result = 0;\nfn Setup() {\n  result = Mathematics.CosineInterpolation(0.0, 10.0, 0.5);\n}\n");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value v = engine.GetGlobal("result");
        if (ok && v.type == Value::Type::NUMBER && v.numberValue > 4.0 && v.numberValue < 6.0) PASS()
        else FAIL("CosineInterp(0,10,0.5) expected ~5, got " + std::to_string(v.numberValue))
    }
    
    TEST("Non-static method on static-only class fails gracefully")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent("var result = 0;\nfn Setup() {\n  result = Mathematics.NonExistentMethod(1.0);\n}\n");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        // Error can occur at compile, build, or during Setup() execution
        if (!ok || engine.HasError()) PASS()
        else FAIL("Should have errored on non-existent method")
    }
}


void TestPhase24RefParams() {
    std::cout << "--- Reference Parameter Overloads ---" << std::endl;
}


void TestPhase24TemplateRefactor() {
    std::cout << "--- Template Refactor + New Reflections ---" << std::endl;
}

// ============================================================================
// Script-Side Classes Tests
// ============================================================================

void TestPhase24AuditFixes() {
    std::cout << "--- Audit: Newly Unblocked Classes ---" << std::endl;

    TEST("SphereCollider ScriptRaycast from KoiloScript")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent(R"(
var col = SphereCollider(Vector3D(0, 0, 0), 5.0);
var hit = false;
fn Setup() {
  hit = col.ScriptRaycast(Vector3D(-10, 0, 0), Vector3D(1, 0, 0), 100.0);
}
)");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        if (ok) {
            auto v = engine.GetGlobal("hit");
            if (v.type == Value::Type::BOOL && v.boolValue == true) PASS()
            else FAIL("ScriptRaycast should return true for ray hitting sphere")
        }
        else FAIL("Load failed: " + std::string(engine.GetError()))
    }

    TEST("BoxCollider ScriptRaycast from KoiloScript")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent(R"(
var col = BoxCollider(Vector3D(0, 0, 0), Vector3D(10, 10, 10));
var hit = false;
fn Setup() {
  hit = col.ScriptRaycast(Vector3D(-20, 0, 0), Vector3D(1, 0, 0), 100.0);
}
)");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        if (ok) {
            auto v = engine.GetGlobal("hit");
            if (v.type == Value::Type::BOOL && v.boolValue == true) PASS()
            else FAIL("BoxCollider ScriptRaycast should hit")
        }
        else FAIL("Load failed: " + std::string(engine.GetError()))
    }

    TEST("CapsuleCollider ScriptRaycast from KoiloScript")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent(R"(
var col = CapsuleCollider(Vector3D(0, 0, 0), 5.0, 10.0);
var hit = false;
fn Setup() {
  hit = col.ScriptRaycast(Vector3D(-20, 0, 0), Vector3D(1, 0, 0), 100.0);
}
)");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        if (ok) {
            auto v = engine.GetGlobal("hit");
            if (v.type == Value::Type::BOOL && v.boolValue == true) PASS()
            else FAIL("CapsuleCollider ScriptRaycast should hit")
        }
        else FAIL("Load failed: " + std::string(engine.GetError()))
    }

    TEST("MemoryProfiler accessible from KoiloScript")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent(R"(
var mp = MemoryProfiler();
var result = 0;
fn Setup() {
  mp.SetEnabled(false);
  if (mp.IsEnabled()) { result = 1; }
  else { result = 2; }
}
)");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        if (ok) {
            auto v = engine.GetGlobal("result");
            if (v.type == Value::Type::NUMBER && v.numberValue == 2.0) PASS()
            else FAIL("MemoryProfiler SetEnabled/IsEnabled failed, result=" + std::to_string(v.numberValue))
        }
        else FAIL("Load failed: " + std::string(engine.GetError()))
    }

    TEST("PerformanceProfiler accessible from KoiloScript")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent(R"(
var pp = PerformanceProfiler();
var result = 0;
fn Setup() {
  pp.SetEnabled(false);
  if (pp.IsEnabled()) { result = 1; }
  else { result = 2; }
}
)");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        if (ok) {
            auto v = engine.GetGlobal("result");
            if (v.type == Value::Type::NUMBER && v.numberValue == 2.0) PASS()
            else FAIL("PerformanceProfiler SetEnabled/IsEnabled failed, result=" + std::to_string(v.numberValue))
        }
        else FAIL("Load failed: " + std::string(engine.GetError()))
    }

    TEST("PathfinderGrid accessible from KoiloScript")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent(R"(
var grid = PathfinderGrid(10, 10, true);
var ok = false;
fn Setup() {
  grid.SetWalkable(5, 5, false);
  ok = true;
}
)");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        if (ok) {
            auto v = engine.GetGlobal("ok");
            if (v.type == Value::Type::BOOL && v.boolValue == true) PASS()
            else FAIL("PathfinderGrid access failed")
        }
        else FAIL("Load failed: " + std::string(engine.GetError()))
    }

}



