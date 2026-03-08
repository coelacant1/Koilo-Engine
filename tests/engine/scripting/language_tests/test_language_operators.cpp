// SPDX-License-Identifier: GPL-3.0-or-later
// Operator Overloading & Coroutines
#include "helpers.hpp"

void TestPhase40OperatorOverloading() {
    std::cout << "--- Operator Overloading ---" << std::endl;

    // 40.1.1: Vector3D + Vector3D
    {
        TEST("40.1.1: Vector3D + Vector3D");
        StringFileReader reader;
        reader.SetContent(R"(
var result = 0;
fn Setup() {
    var a = new Vector3D(1.0, 2.0, 3.0);
    var b = new Vector3D(4.0, 5.0, 6.0);
    var c = a + b;
    var rx = c.X;
    var ry = c.Y;
    var rz = c.Z;
    result = rx + ry * 100 + rz * 10000;
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        Value v = engine.GetScriptVariable("result");
        // c = (5, 7, 9), result = 5 + 700 + 90000 = 90705
        if (v.type == Value::Type::NUMBER && v.numberValue == 90705.0) { PASS(); }
        else { FAIL("expected 90705, got " + std::to_string(v.numberValue)); }
    }

    // 40.1.2: Vector3D - Vector3D
    {
        TEST("40.1.2: Vector3D - Vector3D");
        StringFileReader reader;
        reader.SetContent(R"(
var result = 0;
fn Setup() {
    var a = new Vector3D(10.0, 20.0, 30.0);
    var b = new Vector3D(3.0, 5.0, 7.0);
    var c = a - b;
    var rx = c.X;
    var ry = c.Y;
    var rz = c.Z;
    result = rx + ry * 100 + rz * 10000;
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        Value v = engine.GetScriptVariable("result");
        // c = (7, 15, 23), result = 7 + 1500 + 230000 = 231507
        if (v.type == Value::Type::NUMBER && v.numberValue == 231507.0) { PASS(); }
        else { FAIL("expected 231507, got " + std::to_string(v.numberValue)); }
    }

    // 40.1.3: Vector3D * Vector3D (component-wise)
    {
        TEST("40.1.3: Vector3D * Vector3D");
        StringFileReader reader;
        reader.SetContent(R"(
var result = 0;
fn Setup() {
    var a = new Vector3D(2.0, 3.0, 4.0);
    var b = new Vector3D(5.0, 6.0, 7.0);
    var c = a * b;
    var rx = c.X;
    var ry = c.Y;
    var rz = c.Z;
    result = rx + ry * 100 + rz * 10000;
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        Value v = engine.GetScriptVariable("result");
        // c = (10, 18, 28), result = 10 + 1800 + 280000 = 281810
        if (v.type == Value::Type::NUMBER && v.numberValue == 281810.0) { PASS(); }
        else { FAIL("expected 281810, got " + std::to_string(v.numberValue)); }
    }

    // 40.1.4: Vector3D / Vector3D (component-wise)
    {
        TEST("40.1.4: Vector3D / Vector3D");
        StringFileReader reader;
        reader.SetContent(R"(
var result = 0;
fn Setup() {
    var a = new Vector3D(10.0, 20.0, 30.0);
    var b = new Vector3D(2.0, 4.0, 5.0);
    var c = a / b;
    var rx = c.X;
    var ry = c.Y;
    var rz = c.Z;
    result = rx + ry * 100 + rz * 10000;
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        Value v = engine.GetScriptVariable("result");
        // c = (5, 5, 6), result = 5 + 500 + 60000 = 60505
        if (v.type == Value::Type::NUMBER && v.numberValue == 60505.0) { PASS(); }
        else { FAIL("expected 60505, got " + std::to_string(v.numberValue)); }
    }

    // 40.1.5: Chained operations (a + b - c)
    {
        TEST("40.1.5: Chained Vector3D ops (a + b - c)");
        StringFileReader reader;
        reader.SetContent(R"(
var result = 0;
fn Setup() {
    var a = new Vector3D(1.0, 2.0, 3.0);
    var b = new Vector3D(4.0, 5.0, 6.0);
    var c = new Vector3D(2.0, 3.0, 4.0);
    var d = a + b - c;
    var rx = d.X;
    var ry = d.Y;
    var rz = d.Z;
    result = rx + ry * 100 + rz * 10000;
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        Value v = engine.GetScriptVariable("result");
        // d = (1+4-2, 2+5-3, 3+6-4) = (3, 4, 5), result = 3 + 400 + 50000 = 50403
        if (v.type == Value::Type::NUMBER && v.numberValue == 50403.0) { PASS(); }
        else { FAIL("expected 50403, got " + std::to_string(v.numberValue)); }
    }

    // 40.1.6: Numeric addition still works (regression check)
    {
        TEST("40.1.6: Numeric addition unchanged");
        StringFileReader reader;
        reader.SetContent("var result = 10 + 20;\nfn Update(dt) {}\n");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        Value v = engine.GetScriptVariable("result");
        if (v.type == Value::Type::NUMBER && v.numberValue == 30.0) { PASS(); }
        else { FAIL("expected 30, got " + std::to_string(v.numberValue)); }
    }

    // 40.1.7: String concat still works (regression check)
    {
        TEST("40.1.7: String concat unchanged");
        StringFileReader reader;
        reader.SetContent("var result = \"hello\" + \" world\";\nfn Update(dt) {}\n");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        Value v = engine.GetScriptVariable("result");
        if (v.type == Value::Type::STRING && v.stringValue == "hello world") { PASS(); }
        else { FAIL("expected 'hello world'"); }
    }
}

void TestPhase40Coroutines() {
    std::cout << "--- Coroutines ---" << std::endl;

    // 40.2.1: Basic yield  coroutine runs to yield, then resumes next frame
    {
        TEST("40.2.1: Basic yield and resume");
        StringFileReader reader;
        reader.SetContent(R"(
var step = 0;
fn MySequence() {
    step = 1;
    yield;
    step = 2;
    yield;
    step = 3;
}
fn Setup() {
    start_coroutine("MySequence");
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        // Setup ran, start_coroutine queued
        Value v = engine.GetScriptVariable("step");
        // step is still 0 (coroutine not started yet, just queued)
        bool ok = (v.type == Value::Type::NUMBER && v.numberValue == 0.0);
        // Frame 1: starts coroutine, runs to first yield -> step=1
        engine.ExecuteUpdate();
        v = engine.GetScriptVariable("step");
        ok = ok && (v.type == Value::Type::NUMBER && v.numberValue == 1.0);
        // Frame 2: resumes, runs to second yield -> step=2
        engine.ExecuteUpdate();
        v = engine.GetScriptVariable("step");
        ok = ok && (v.type == Value::Type::NUMBER && v.numberValue == 2.0);
        // Frame 3: resumes, runs to end -> step=3
        engine.ExecuteUpdate();
        v = engine.GetScriptVariable("step");
        ok = ok && (v.type == Value::Type::NUMBER && v.numberValue == 3.0);
        if (ok) { PASS(); }
        else { FAIL("step progression failed, final step=" + std::to_string(v.numberValue)); }
    }

    // 40.2.2: Coroutine that completes without yielding
    {
        TEST("40.2.2: Coroutine without yield runs once");
        StringFileReader reader;
        reader.SetContent(R"(
var done = 0;
fn QuickTask() {
    done = 1;
}
fn Setup() {
    start_coroutine("QuickTask");
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        engine.ExecuteUpdate();
        Value v = engine.GetScriptVariable("done");
        if (v.type == Value::Type::NUMBER && v.numberValue == 1.0) { PASS(); }
        else { FAIL("expected done=1"); }
    }

    // 40.2.3: start_coroutine from Update
    {
        TEST("40.2.3: start_coroutine from Update");
        StringFileReader reader;
        reader.SetContent(R"(
var step = 0;
var started = 0;
fn MySeq() {
    step = 1;
    yield;
    step = 2;
}
fn Update(dt) {
    if (started == 0) {
        start_coroutine("MySeq");
        started = 1;
    }
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        // Frame 1: Update starts coroutine -> runs to yield -> step=1
        engine.ExecuteUpdate();
        Value v = engine.GetScriptVariable("step");
        bool ok = (v.type == Value::Type::NUMBER && v.numberValue == 1.0);
        // Frame 2: resumes -> step=2
        engine.ExecuteUpdate();
        v = engine.GetScriptVariable("step");
        ok = ok && (v.type == Value::Type::NUMBER && v.numberValue == 2.0);
        if (ok) { PASS(); }
        else { FAIL("step=" + std::to_string(v.numberValue)); }
    }

    // 40.2.4: Variable mutation across yields
    {
        TEST("40.2.4: Variable mutation across yields");
        StringFileReader reader;
        reader.SetContent(R"(
var counter = 0;
fn CountUp() {
    counter = counter + 1;
    yield;
    counter = counter + 10;
    yield;
    counter = counter + 100;
}
fn Setup() {
    start_coroutine("CountUp");
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        engine.ExecuteUpdate(); // start -> counter=1
        Value v = engine.GetScriptVariable("counter");
        bool ok = (v.numberValue == 1.0);
        engine.ExecuteUpdate(); // resume -> counter=11
        v = engine.GetScriptVariable("counter");
        ok = ok && (v.numberValue == 11.0);
        engine.ExecuteUpdate(); // resume -> counter=111
        v = engine.GetScriptVariable("counter");
        ok = ok && (v.numberValue == 111.0);
        if (ok) { PASS(); }
        else { FAIL("counter=" + std::to_string(v.numberValue)); }
    }

    // 40.2.5: stop_coroutine cancels execution
    {
        TEST("40.2.5: stop_coroutine cancels");
        StringFileReader reader;
        reader.SetContent(R"(
var step = 0;
fn LongTask() {
    step = 1;
    yield;
    step = 2;
    yield;
    step = 3;
}
fn Setup() {
    start_coroutine("LongTask");
}
fn Update(dt) {
    if (step == 1) {
        stop_coroutine("LongTask");
    }
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        // Frame 1: start -> step=1
        engine.ExecuteUpdate();
        Value v = engine.GetScriptVariable("step");
        bool ok = (v.numberValue == 1.0);
        // Frame 2: Update sees step==1, stops coroutine. Resume skips it (finished).
        engine.ExecuteUpdate();
        v = engine.GetScriptVariable("step");
        ok = ok && (v.numberValue == 1.0); // Should NOT advance to 2
        // Frame 3: coroutine removed, step unchanged
        engine.ExecuteUpdate();
        v = engine.GetScriptVariable("step");
        ok = ok && (v.numberValue == 1.0);
        if (ok) { PASS(); }
        else { FAIL("step should stay at 1, got " + std::to_string(v.numberValue)); }
    }

    // 40.2.6: Multiple coroutines running simultaneously
    {
        TEST("40.2.6: Multiple simultaneous coroutines");
        StringFileReader reader;
        reader.SetContent(R"(
var a = 0;
var b = 0;
fn SeqA() {
    a = 1;
    yield;
    a = 2;
}
fn SeqB() {
    b = 10;
    yield;
    b = 20;
}
fn Setup() {
    start_coroutine("SeqA");
    start_coroutine("SeqB");
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        // Frame 1: both start -> a=1, b=10
        engine.ExecuteUpdate();
        Value va = engine.GetScriptVariable("a");
        Value vb = engine.GetScriptVariable("b");
        bool ok = (va.numberValue == 1.0 && vb.numberValue == 10.0);
        // Frame 2: both resume -> a=2, b=20
        engine.ExecuteUpdate();
        va = engine.GetScriptVariable("a");
        vb = engine.GetScriptVariable("b");
        ok = ok && (va.numberValue == 2.0 && vb.numberValue == 20.0);
        if (ok) { PASS(); }
        else { FAIL("a=" + std::to_string(va.numberValue) + " b=" + std::to_string(vb.numberValue)); }
    }

    // 40.2.7: Coroutine count tracking
    {
        TEST("40.2.7: Coroutine manager count");
        StringFileReader reader;
        reader.SetContent(R"(
var step = 0;
fn MySeq() {
    step = 1;
    yield;
    step = 2;
}
fn Setup() {
    start_coroutine("MySeq");
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        bool ok = (engine.GetCoroutineManager().Count() == 0); // not started yet
        engine.ExecuteUpdate(); // started, yielded -> 1 active
        ok = ok && (engine.GetCoroutineManager().Count() == 1);
        engine.ExecuteUpdate(); // resumed, finished -> marked finished
        // Finished coroutine is cleaned up at start of NEXT frame's ResumeAll()
        ok = ok && (engine.GetCoroutineManager().Count() == 1);
        engine.ExecuteUpdate(); // ResumeAll() cleans up finished -> count=0
        ok = ok && (engine.GetCoroutineManager().Count() == 0);
        if (ok) { PASS(); }
        else { FAIL("count tracking wrong, final count=" + std::to_string(engine.GetCoroutineManager().Count())); }
    }

    // 40.2.8: Yield inside while loop (stack leak regression test)
    {
        TEST("40.2.8: Yield inside while loop");
        StringFileReader reader;
        reader.SetContent(R"(
var counter = 0;
var done = false;
fn LoopCoroutine() {
    var i = 0;
    while i < 5 {
        i = i + 1;
        counter = counter + 1;
        yield;
    }
    done = true;
}
fn Setup() {
    start_coroutine("LoopCoroutine");
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        // Run 10 frames (coroutine should finish in 5)
        for (int i = 0; i < 10; i++) {
            engine.ExecuteUpdate();
        }
        Value counter = engine.GetScriptVariable("counter");
        Value done = engine.GetScriptVariable("done");
        bool ok = (counter.type == Value::Type::NUMBER && counter.numberValue == 5.0 &&
                   done.type == Value::Type::BOOL && done.boolValue == true);
        if (ok) { PASS(); }
        else { FAIL("counter=" + std::to_string(counter.numberValue) + " done=" + std::to_string(done.boolValue)); }
    }

    // 40.2.9: Array of class instances  field access after store
    {
        TEST("40.2.9: Array instance field access");
        MultiFileReader reader;
        reader.AddFile("test.ks", R"(
class Obj {
    var alive = true;
    var val = 42;
}

var items = [];
var aliveResult = 0;
var valResult = 0;
var filterResult = 0;

fn Setup() {
    var o = new Obj();
    items[0] = o;
    
    if items[0].alive { aliveResult = 1; } else { aliveResult = -1; }
    valResult = items[0].val;
    
    var kept = [];
    var i = 0;
    while i < length(items) {
        if items[i].alive {
            kept[length(kept)] = items[i];
        }
        i = i + 1;
    }
    filterResult = length(kept);
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        Value alive = engine.GetScriptVariable("aliveResult");
        Value val = engine.GetScriptVariable("valResult");
        Value filter = engine.GetScriptVariable("filterResult");
        bool ok = (alive.numberValue == 1.0 && val.numberValue == 42.0 && filter.numberValue == 1.0);
        if (ok) { PASS(); }
        else { FAIL("alive=" + std::to_string(alive.numberValue) + " val=" + std::to_string(val.numberValue) + " filter=" + std::to_string(filter.numberValue)); }
    }

    // 40.2.10: Coroutine array instance persistence
    {
        TEST("40.2.10: Coroutine array instance persistence");
        MultiFileReader reader;
        reader.AddFile("enemies.ks", R"(
class Enemy {
    var alive = true;
    var y = 0.0;
    var hp = 1;
    fn Update(dt) {
        self.y = self.y + 10.0 * dt;
        if self.y > 50.0 { self.alive = false; }
    }
}

fn MakeEnemy(ypos) {
    var e = new Enemy();
    e.y = ypos;
    return e;
}
)");
        reader.AddFile("test.ks", R"(
import "enemies.ks";

var enemies = [];
var waveSpawning = false;
var wave = 0;
var aliveCount = 0;
var waveComplete = 0;
var spawnDone = 0;

signal on_wave_complete(waveNum);

fn OnWaveComplete(waveNum) {
    waveComplete = waveComplete + 1;
    start_coroutine("SpawnNextWave");
}

fn SpawnNextWave() {
    waveSpawning = true;
    wave = wave + 1;
    
    var i = 0;
    while i < 3 {
        enemies[length(enemies)] = MakeEnemy(-3.0);
        i = i + 1;
        yield;
    }
    
    spawnDone = 1;
    waveSpawning = false;
}

fn Setup() {
    connect("on_wave_complete", "OnWaveComplete");
    start_coroutine("SpawnNextWave");
}

fn Update(dt) {
    // Update enemies
    var ei = 0;
    while ei < length(enemies) {
        enemies[ei].Update(dt);
        ei = ei + 1;
    }
    
    // Filter dead enemies
    var newEnemies = [];
    ei = 0;
    while ei < length(enemies) {
        if enemies[ei].alive {
            newEnemies[length(newEnemies)] = enemies[ei];
        }
        ei = ei + 1;
    }
    enemies = newEnemies;
    aliveCount = length(enemies);
    
    // Check wave complete
    if length(enemies) == 0 and not waveSpawning and wave > 0 {
        emit on_wave_complete(wave);
    }
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        // Run enough frames for coroutine to finish spawning (3 yields + processing)
        for (int i = 0; i < 10; i++) engine.ExecuteUpdate();
        Value count = engine.GetScriptVariable("aliveCount");
        Value spawn = engine.GetScriptVariable("spawnDone");
        Value wc = engine.GetScriptVariable("waveComplete");
        if (count.numberValue == 3.0 && spawn.numberValue == 1.0) { PASS(); }
        else { FAIL("aliveCount=" + std::to_string(count.numberValue) + 
                     " spawnDone=" + std::to_string(spawn.numberValue) + 
                     " waveComplete=" + std::to_string(wc.numberValue)); }
    }

    // 40.2.11: NOT operator on boolean global
    {
        TEST("40.2.11: not false == true");
        StringFileReader reader;
        reader.SetContent(R"(
var a = false;
var result1 = 0;
var result2 = 0;
var result3 = 0;

fn Setup() {
    result1 = not a;
    var b = not a;
    result2 = b;
    if b { result3 = 1; } else { result3 = -1; }
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        if (engine.HasError()) { FAIL("Error: " + std::string(engine.GetError())); }
        Value r1 = engine.GetScriptVariable("result1");
        Value r2 = engine.GetScriptVariable("result2");
        Value r3 = engine.GetScriptVariable("result3");
        if (r1.boolValue == true && r1.type == Value::Type::BOOL &&
            r2.boolValue == true && r3.numberValue == 1.0) { PASS(); }
        else { FAIL("r1.type=" + std::to_string((int)r1.type) + " r1.num=" + std::to_string(r1.numberValue) +
                     " r1.bool=" + std::to_string(r1.boolValue) +
                     " r2.type=" + std::to_string((int)r2.type) + " r2.num=" + std::to_string(r2.numberValue) +
                     " r2.bool=" + std::to_string(r2.boolValue) +
                     " r3=" + std::to_string(r3.numberValue)); }
    }
}

// =============================================================================
// Script Debugging & Error Hardening
// =============================================================================

void TestPhase43DebugErrorHardening() {
    std::cout << "--- Debugging & Error Hardening ---" << std::endl;

    // 43.1: Stack trace on errors  function call chain reported
    {
        TEST("43.1: Stack trace on nested call error");
        StringFileReader reader;
        reader.SetContent(R"(
fn inner() {
    var x = "hello" - 5;
}
fn outer() {
    inner();
}
fn Setup() {
    outer();
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        std::string err = engine.HasError() ? engine.GetError() : "";
        // Should have "Cannot subtract" and "Stack trace:" with function names
        bool hasSubtractError = err.find("Cannot subtract") != std::string::npos;
        bool hasStackTrace = err.find("Stack trace:") != std::string::npos;
        bool hasInner = err.find("inner") != std::string::npos;
        if (hasSubtractError && hasStackTrace && hasInner) { PASS(); }
        else { FAIL("err=" + err); }
    }

    // 43.2a: Type mismatch on addition
    {
        TEST("43.2a: Type mismatch error on bool + array");
        StringFileReader reader;
        reader.SetContent(R"(
var r = 0;
fn Setup() {
    var a = true;
    var b = [1, 2, 3];
    r = a + b;
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        std::string err = engine.HasError() ? engine.GetError() : "";
        bool hasTypeError = err.find("Cannot add") != std::string::npos;
        if (hasTypeError) { PASS(); }
        else { FAIL("err=" + err); }
    }

    // 43.2b: Type mismatch on comparison
    {
        TEST("43.2b: Type mismatch error on string < number");
        StringFileReader reader;
        reader.SetContent(R"(
var r = 0;
fn Setup() {
    var a = "hello";
    var b = 5;
    if a < b { r = 1; }
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        std::string err = engine.HasError() ? engine.GetError() : "";
        bool hasCompareError = err.find("Cannot compare") != std::string::npos;
        if (hasCompareError) { PASS(); }
        else { FAIL("err=" + err); }
    }

    // 43.3a: format() builtin
    {
        TEST("43.3a: format() string formatting");
        StringFileReader reader;
        reader.SetContent(R"(
var result = "";
fn Setup() {
    result = format("Hello {} you are {} years old", "world", 25);
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        if (engine.HasError()) { FAIL(engine.GetError()); }
        else {
            Value r = engine.GetScriptVariable("result");
            if (r.type == Value::Type::STRING && r.stringValue == "Hello world you are 25 years old") { PASS(); }
            else { FAIL("result=" + r.stringValue); }
        }
    }

    // 43.3b: debug() builtin runs without crash (output goes to stdout, we just check no error)
    {
        TEST("43.3b: debug() builtin with type annotations");
        StringFileReader reader;
        reader.SetContent(R"(
var ok = 0;
fn Setup() {
    debug(42, "hello", true, [1,2,3]);
    ok = 1;
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        if (engine.HasError()) { FAIL(engine.GetError()); }
        else {
            Value r = engine.GetScriptVariable("ok");
            if (r.numberValue == 1.0) { PASS(); }
            else { FAIL("ok=" + std::to_string(r.numberValue)); }
        }
    }

    // 43.3c: print() handles tables and nested arrays
    {
        TEST("43.3c: print() handles tables");
        StringFileReader reader;
        reader.SetContent(R"(
var ok = 0;
fn Setup() {
    var t = { name: "test", value: 42 };
    print(t);
    ok = 1;
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        if (engine.HasError()) { FAIL(engine.GetError()); }
        else {
            Value r = engine.GetScriptVariable("ok");
            if (r.numberValue == 1.0) { PASS(); }
            else { FAIL("ok=" + std::to_string(r.numberValue)); }
        }
    }

    // 43.4: Hot-reload error recovery
    {
        TEST("43.4: Reload recovers from parse error");
        MultiFileReader* reader = new MultiFileReader();
        reader->AddFile("test.ks", "var x = 10;\nfn Setup() { x = 42; }\n");
        KoiloScriptEngine engine(reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        Value x1 = engine.GetScriptVariable("x");
        bool initialOk = (x1.numberValue == 42.0);

        // Now simulate a bad reload with broken syntax
        reader->AddFile("test.ks", "fn Setup() { this is broken syntax !!! }}}");
        bool reloadResult = engine.Reload();
        // Reload should fail but engine should still be usable
        bool reloadFailed = !reloadResult;
        std::string err = engine.HasError() ? engine.GetError() : "";
        bool hasRecoveryMsg = err.find("previous script restored") != std::string::npos;

        if (initialOk && reloadFailed && hasRecoveryMsg) { PASS(); }
        else { FAIL("initial=" + std::to_string(initialOk) + " reloadFailed=" + std::to_string(reloadFailed) +
                     " err=" + err); }
    }
}

// =============================================================================
// Competitive Analysis Validation
// =============================================================================

void TestPhase41Validation() {
    std::cout << "--- Error & Diagnostics Validation ---" << std::endl;

    // 41.2: Undefined variables return NONE without crash
    {
        TEST("41.2: Undefined variable returns NONE gracefully");
        StringFileReader reader;
        reader.SetContent(R"(
var result = 0;
fn Setup() {
    var x = undeclaredVar;
    if x == null {
        result = 1;
    }
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        // Should not crash; undeclared var returns NONE which equals null
        Value r = engine.GetScriptVariable("result");
        if (r.numberValue == 1.0) { PASS(); }
        else { FAIL("result=" + std::to_string(r.numberValue) +
                     " err=" + std::string(engine.HasError() ? engine.GetError() : "none")); }
    }

    // 41.3a: Runtime errors include line numbers
    {
        TEST("41.3a: Runtime error includes line number");
        StringFileReader reader;
        reader.SetContent(R"(
fn Setup() {
    var a = "text";
    var b = [1,2];
    var c = a - b;
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        std::string err = engine.HasError() ? engine.GetError() : "";
        bool hasLine = err.find("[Line") != std::string::npos;
        bool hasError = err.find("Cannot subtract") != std::string::npos;
        if (hasLine && hasError) { PASS(); }
        else { FAIL("err=" + err); }
    }

    // 41.3b: Parser errors include line and column
    {
        TEST("41.3b: Parse error includes line and column");
        StringFileReader reader;
        reader.SetContent(R"(
fn Setup() {
    var x = ;
}
)");
        KoiloScriptEngine engine(&reader);
        bool loaded = engine.LoadScript("test.ks");
        std::string err = engine.HasError() ? engine.GetError() : "";
        bool hasLineCol = err.find("line") != std::string::npos && err.find("column") != std::string::npos;
        if (!loaded && hasLineCol) { PASS(); }
        else { FAIL("loaded=" + std::to_string(loaded) + " err=" + err); }
    }
}

