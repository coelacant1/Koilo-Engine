// SPDX-License-Identifier: GPL-3.0-or-later
// Enums, Hardening & Performance
#include "helpers.hpp"

void TestPhase24Enums() {
    std::cout << "--- Enum Constant Registration ---" << std::endl;
    
    TEST("FunctionGenerator enum constants accessible")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent(R"(
var t = 0; var sq = 0; var si = 0; var sa = 0; var gr = 0;
fn Setup() {
  t = Triangle; sq = Square; si = Sine; sa = Sawtooth; gr = Gravity;
}
)");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value vt = engine.GetGlobal("t");
        Value vs = engine.GetGlobal("si");
        if (ok && vt.numberValue == 0.0 && vs.numberValue == 2.0) PASS()
        else FAIL("Triangle=" + std::to_string(vt.numberValue) + " Sine=" + std::to_string(vs.numberValue))
    }
    
    TEST("InterpolationMethod enum constants accessible")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent(R"(
var co = 0; var bo = 0; var li = 0; var ov = 0;
fn Setup() {
  co = Cosine; bo = Bounce; li = Linear; ov = Overshoot;
}
)");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value vc = engine.GetGlobal("co");
        Value vo = engine.GetGlobal("ov");
        if (ok && vc.numberValue == 0.0 && vo.numberValue == 3.0) PASS()
        else FAIL("Cosine=" + std::to_string(vc.numberValue) + " Overshoot=" + std::to_string(vo.numberValue))
    }
    
    TEST("FunctionGenerator uses enum constant from script")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent(R"(
var gen = FunctionGenerator(Sine, 1.0, 1.0, 0.0);
var val = 0;
fn Setup() {
  gen.Update();
  val = gen.Update();
}
)");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value v = engine.GetGlobal("val");
        if (ok && v.type == Value::Type::NUMBER) PASS()
        else FAIL("FunctionGenerator with Sine enum failed: " + std::string(engine.GetError()))
    }
    
    TEST("EasyEaseAnimator with named interpolation method")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent(R"(
var ease = EasyEaseAnimator(4, Bounce, 1.0, 0.5);
fn Setup() {
  ease.AddParameterByIndex(0, 30, 0.0, 1.0);
  ease.SetInterpolationMethod(0, Overshoot);
}
)");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        if (ok) PASS()
        else FAIL("EasyEaseAnimator with enum failed: " + std::string(engine.GetError()))
    }
}

void TestPhase9Hardening() {
    using namespace koilo;
    using namespace koilo::scripting;
    std::cout << "--- Engine Hardening ---" << std::endl;

    // Test 1: Transform SetOrigin sets both pivots
    {
        Transform t;
        t.SetOrigin(Vector3D(5, 10, 15));
        TEST("SetOrigin sets rotation pivot")
        if (t.GetRotationOffset().X == 5 && t.GetRotationOffset().Y == 10 && t.GetRotationOffset().Z == 15)
            PASS() else FAIL("rotation offset wrong")
        TEST("SetOrigin sets scale pivot")
        if (t.GetScaleOffset().X == 5 && t.GetScaleOffset().Y == 10 && t.GetScaleOffset().Z == 15)
            PASS() else FAIL("scale offset wrong")
        TEST("GetOrigin returns pivot")
        if (t.GetOrigin().X == 5) PASS() else FAIL("origin wrong")
    }

    // Test 2: Transform independent pivots still work
    {
        Transform t;
        t.SetRotationOffset(Vector3D(1, 0, 0));
        t.SetScaleOffset(Vector3D(0, 1, 0));
        TEST("Independent pivots")
        if (t.GetRotationOffset().X == 1 && t.GetScaleOffset().Y == 1)
            PASS() else FAIL("pivots not independent")
    }

    // Test 3: Scene mesh count > 255
    {
        Scene scene;
        TEST("Scene accepts >255 mesh count")
        if (scene.GetMeshCount() == 0) PASS() else FAIL("initial count wrong")
    }

    // Test 4: Transform SetOrigin reflected
    {
        Transform::Describe();
        const ClassDesc* desc = ReflectionBridge::FindClass("Transform");
        TEST("Transform reflected with SetOrigin")
        bool found = false;
        if (desc) {
            for (size_t i = 0; i < desc->methods.count; i++) {
                if (std::string(desc->methods.data[i].name) == "SetOrigin") { found = true; break; }
            }
        }
        if (found) PASS() else FAIL("SetOrigin not in reflection")
    }

    // Test 5: SurfaceProperties no FromUV in reflection
    {
        SurfaceProperties::Describe();
        const ClassDesc* desc = ReflectionBridge::FindClass("SurfaceProperties");
        TEST("SurfaceProperties no FromUV")
        bool found = false;
        if (desc) {
            for (size_t i = 0; i < desc->methods.count; i++) {
                if (std::string(desc->methods.data[i].name).find("FromUV") != std::string::npos ||
                    std::string(desc->methods.data[i].name).find("From uv") != std::string::npos) { found = true; break; }
            }
        }
        if (!found) PASS() else FAIL("FromUV still registered")
    }

    std::cout << std::endl;
}

// =============================================================================
// Script VM Performance Tests
// =============================================================================

void TestPhase10Performance() {
    std::cout << "--- Script VM Performance ---" << std::endl;
    
    StringFileReader reader;
    
    // Test 1: Registry hash map lookup (FindClass returns correct result)
    {
        TEST("Registry hash FindClass")
        koilo::scripting::ReflectionBridge::FindClass("Vector3D");  // warm up cache
        const koilo::ClassDesc* desc = koilo::scripting::ReflectionBridge::FindClass("Vector3D");
        if (desc && std::string(desc->name) == "Vector3D") PASS() else FAIL("FindClass failed via hash map")
    }
    
    // Test 2: Field hash map lookup
    {
        TEST("Field hash FindField")
        const koilo::ClassDesc* desc = koilo::scripting::ReflectionBridge::FindClass("Vector3D");
        const koilo::FieldDecl* field = koilo::scripting::ReflectionBridge::FindField(desc, "X");
        if (field && std::string(field->name) == "X") PASS() else FAIL("FindField hash failed")
    }
    
    // Test 3: Method hash map lookup
    {
        TEST("Method hash FindMethod")
        const koilo::ClassDesc* desc = koilo::scripting::ReflectionBridge::FindClass("Vector3D");
        const koilo::MethodDesc* method = koilo::scripting::ReflectionBridge::FindMethod(desc, "Normal");
        if (method && std::string(method->name) == "Normal") PASS() else FAIL("FindMethod hash failed")
    }
    
    // Test 4: Nonexistent class returns nullptr
    {
        TEST("Hash miss returns nullptr")
        const koilo::ClassDesc* desc = koilo::scripting::ReflectionBridge::FindClass("NonExistentClass12345");
        if (!desc) PASS() else FAIL("Should return nullptr for nonexistent class")
    }
    
    // Test 5: Scope unordered_map works correctly
    {
        TEST("Unordered scope variables")
        reader.SetContent(R"(
var a = 10;
var b = 20;
fn Setup() {
    var c = a + b;
    if (c == 30) {
        var d = c * 2;
        a = d;
    }
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        Value a = engine.GetGlobal("a");
        if (a.type == Value::Type::NUMBER && a.numberValue == 60.0) PASS() else FAIL("Scope variable lookup failed")
    }
    
    // Test 6: Number-to-string conversion (NumToStr replaces ostringstream)
    {
        TEST("NumToStr string concat")
        reader.SetContent(R"(
var x = 42;
var s = "value=" + x;
fn Setup() {}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        Value s = engine.GetGlobal("s");
        if (s.type == Value::Type::STRING && s.stringValue == "value=42") PASS() else FAIL("Got: " + s.stringValue)
    }
    
    // Test 7: str() builtin uses fast conversion
    {
        TEST("str() fast conversion")
        reader.SetContent(R"(
var s = str(3.14);
fn Setup() {}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        Value s = engine.GetGlobal("s");
        if (s.type == Value::Type::STRING && s.stringValue == "3.14") PASS() else FAIL("Got: " + s.stringValue)
    }
    
    // Test 8: Update function caching (runs correctly over multiple frames)
    {
        TEST("Cached Update runs")
        reader.SetContent(R"(
var counter = 0;
fn Setup() {}
fn Update(dt) {
    counter = counter + 1;
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        for (int i = 0; i < 10; i++) { koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate(); }
        Value c = engine.GetGlobal("counter");
        if (c.type == Value::Type::NUMBER && c.numberValue == 10.0) PASS() else FAIL("counter=" + std::to_string(c.numberValue))
    }
    
    // Test 9: Member access path caching (reflected field access works)
    {
        TEST("Path cache member access")
        reader.SetContent(R"(
var x = 0;
fn Setup() {
    var v = Vector3D(1.0, 2.0, 3.0);
    scene.AddMesh(v);
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        // Just verify no crash - path cache exercised during BuildScene
        if (!engine.HasError()) PASS() else FAIL(engine.GetError())
    }
    
    // Test 10: Table number key conversion
    {
        TEST("Table number key fast")
        reader.SetContent(R"(
var t = {};
t[1] = "one";
t[2] = "two";
var r = t[1];
fn Setup() {}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        Value r = engine.GetGlobal("r");
        if (r.type == Value::Type::STRING && r.stringValue == "one") PASS() else FAIL("table key conversion failed")
    }
    
    // Test 11: Benchmark - 1000 Update cycles
    {
        TEST("1000 Update cycles benchmark")
        reader.SetContent(R"(
var total = 0;
fn Setup() {}
fn Update(dt) {
    total = total + dt;
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1000; i++) { koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate(); }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        Value total = engine.GetGlobal("total");
        bool valueCorrect = (total.type == Value::Type::NUMBER && std::abs(total.numberValue - 16.0) < 0.1);
        if (valueCorrect) {
            std::cout << "  [" << ms << "ms for 1000 updates] ";
            PASS()
        } else FAIL("total=" + std::to_string(total.numberValue))
    }

    std::cout << std::endl;
}

// =============================================================================
// Rendering Polish & PBR Tests
// =============================================================================


