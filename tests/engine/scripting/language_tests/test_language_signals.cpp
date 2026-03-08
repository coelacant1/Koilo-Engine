// SPDX-License-Identifier: GPL-3.0-or-later
// Signal Registry
#include "helpers.hpp"

void TestPhase33Signals() {
    std::cout << "\n--- Event/Signal System ---" << std::endl;
    
    // Test 1: Basic signal + emit + handler
    {
        TEST("Signal emit calls handler");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
signal on_hit(damage);
var result = 0;
fn onHit(dmg) {
    result = dmg;
}
connect("on_hit", "onHit");
emit on_hit(42);
fn Setup() {}
fn Update(dt) {}
)";
        if (RunScript(script, reader, engine)) {
            Value r = engine.GetGlobal("result");
            if (r.type == Value::Type::NUMBER && r.numberValue == 42.0) {
                PASS();
            } else {
                FAIL("Expected 42, got " + std::to_string(r.numberValue));
            }
        }
    }
    
    // Test 2: Multiple listeners
    {
        TEST("Multiple listeners on one signal");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
signal on_score();
var a = 0;
var b = 0;
fn handlerA() { a = a + 1; }
fn handlerB() { b = b + 10; }
connect("on_score", "handlerA");
connect("on_score", "handlerB");
emit on_score();
fn Setup() {}
fn Update(dt) {}
)";
        if (RunScript(script, reader, engine)) {
            Value a = engine.GetGlobal("a");
            Value b = engine.GetGlobal("b");
            if (a.type == Value::Type::NUMBER && a.numberValue == 1.0 &&
                b.type == Value::Type::NUMBER && b.numberValue == 10.0) {
                PASS();
            } else {
                FAIL("Expected a=1,b=10 got a=" + std::to_string(a.numberValue) +
                     " b=" + std::to_string(b.numberValue));
            }
        }
    }
    
    // Test 3: Disconnect removes listener
    {
        TEST("Disconnect removes listener");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
signal on_event();
var count = 0;
fn handler() { count = count + 1; }
connect("on_event", "handler");
emit on_event();
disconnect("on_event", "handler");
emit on_event();
fn Setup() {}
fn Update(dt) {}
)";
        if (RunScript(script, reader, engine)) {
            Value c = engine.GetGlobal("count");
            if (c.type == Value::Type::NUMBER && c.numberValue == 1.0) {
                PASS();
            } else {
                FAIL("Expected 1, got " + std::to_string(c.numberValue));
            }
        }
    }
    
    // Test 4: Emit with no listeners (no crash)
    {
        TEST("Emit with no listeners");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
signal on_nothing();
emit on_nothing();
var ok = 1;
fn Setup() {}
fn Update(dt) {}
)";
        if (RunScript(script, reader, engine)) {
            Value ok = engine.GetGlobal("ok");
            if (ok.type == Value::Type::NUMBER && ok.numberValue == 1.0) {
                PASS();
            } else {
                FAIL("Expected 1");
            }
        }
    }
    
    // Test 5: connect_once auto-disconnects
    {
        TEST("connect_once auto-disconnects after first emit");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
signal on_trigger();
var count = 0;
fn handler() { count = count + 1; }
connect_once("on_trigger", "handler");
emit on_trigger();
emit on_trigger();
emit on_trigger();
fn Setup() {}
fn Update(dt) {}
)";
        if (RunScript(script, reader, engine)) {
            Value c = engine.GetGlobal("count");
            if (c.type == Value::Type::NUMBER && c.numberValue == 1.0) {
                PASS();
            } else {
                FAIL("Expected 1, got " + std::to_string(c.numberValue));
            }
        }
    }
    
    // Test 6: Signal with multiple args
    {
        TEST("Signal with multiple arguments");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
signal on_move(x, y, z);
var rx = 0;
var ry = 0;
var rz = 0;
fn onMove(x, y, z) {
    rx = x;
    ry = y;
    rz = z;
}
connect("on_move", "onMove");
emit on_move(10, 20, 30);
fn Setup() {}
fn Update(dt) {}
)";
        if (RunScript(script, reader, engine)) {
            Value x = engine.GetGlobal("rx");
            Value y = engine.GetGlobal("ry");
            Value z = engine.GetGlobal("rz");
            if (x.numberValue == 10.0 && y.numberValue == 20.0 && z.numberValue == 30.0) {
                PASS();
            } else {
                FAIL("Expected 10,20,30 got " + std::to_string(x.numberValue) + "," +
                     std::to_string(y.numberValue) + "," + std::to_string(z.numberValue));
            }
        }
    }
    
    // Test 7: Emit inside function body
    {
        TEST("Emit inside function body");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
signal on_damage(amount);
var total_damage = 0;
fn onDamage(amount) {
    total_damage = total_damage + amount;
}
fn doDamage(val) {
    emit on_damage(val);
}
connect("on_damage", "onDamage");
doDamage(25);
doDamage(15);
fn Setup() {}
fn Update(dt) {}
)";
        if (RunScript(script, reader, engine)) {
            Value d = engine.GetGlobal("total_damage");
            if (d.type == Value::Type::NUMBER && d.numberValue == 40.0) {
                PASS();
            } else {
                FAIL("Expected 40, got " + std::to_string(d.numberValue));
            }
        }
    }
}

// =====================================================================
// Skeletal Skinning Tests
// =====================================================================


