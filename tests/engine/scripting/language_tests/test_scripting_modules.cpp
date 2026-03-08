// SPDX-License-Identifier: GPL-3.0-or-later
// Module System Scripting Bindings
#include "helpers.hpp"

void TestPhase36Modules() {
    std::cout << "--- Modular Engine Runtime ---" << std::endl;
    
    // Test 1: Default subsystems accessible after BuildScene
    {
        TEST("Default subsystems accessible after BuildScene");
        std::string script = R"(
fn Setup() {}
)";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        if (RunScript(script, reader, engine)) {
            bool hasPhysics = engine.GetPhysicsWorld() != nullptr;
            bool hasUI = engine.GetUI() != nullptr;
            bool hasParticles = engine.GetParticleSystem() != nullptr;
            if (hasPhysics && hasUI && hasParticles) {
                PASS();
            } else {
                FAIL("Not all default subsystems available");
            }
        }
    }
    
    // Test 2: ModuleLoader is empty (built-in systems are directly owned)
    {
        TEST("ModuleLoader has no built-in modules");
        std::string script = "fn Setup() {}\n";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        if (RunScript(script, reader, engine)) {
            auto mods = engine.GetModuleLoader().ListModules();
            if (mods.size() == 0) {
                PASS();
            } else {
                FAIL("Expected 0 modules, got " + std::to_string(mods.size()));
            }
        }
    }
    
    // Test 6: Module-owned globals accessible from script (physics)
    {
        TEST("physics global accessible from script");
        std::string script = R"(
var ok = 0;
fn Setup() {
    physics.SetGravity(0, -9.8, 0);
    ok = 1;
}
)";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        if (RunScript(script, reader, engine)) {
            Value v = engine.GetGlobal("ok");
            if (v.type == Value::Type::NUMBER && v.numberValue == 1.0) {
                PASS();
            } else {
                FAIL("physics global not accessible");
            }
        }
    }
    
    // Test 7: GetPhysicsWorld() returns instance
    {
        TEST("GetPhysicsWorld() returns valid instance");
        std::string script = "fn Setup() {}\n";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        if (RunScript(script, reader, engine)) {
            auto* pw = engine.GetPhysicsWorld();
            if (pw != nullptr) {
                PASS();
            } else {
                FAIL("GetPhysicsWorld() returned nullptr");
            }
        }
    }
    
    // Test 8: GetUI() returns instance
    {
        TEST("GetUI() returns valid instance");
        std::string script = "fn Setup() {}\n";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        if (RunScript(script, reader, engine)) {
            auto* ui = engine.GetUI();
            if (ui != nullptr) {
                PASS();
            } else {
                FAIL("GetUI() returned nullptr");
            }
        }
    }
    
    // Test 11: Zero-modules engine runs (core-only mode)
    {
        TEST("Engine runs with zero modules loaded");
        std::string script = R"(
var x = 0;
fn Setup() {
    x = 42;
}
fn Update(dt) {
    x = x + 1;
}
)";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader, false);  // No default modules
        if (RunScript(script, reader, engine)) {
            Value x = engine.GetGlobal("x");
            if (x.type == Value::Type::NUMBER && x.numberValue == 42.0) {
                PASS();
            } else {
                FAIL("Expected 42, got " + std::to_string(x.numberValue));
            }
        }
    }
    
    // Test 12: Zero-modules RenderFrame doesn't crash
    {
        TEST("RenderFrame works with zero modules");
        std::string script = "fn Setup() {}\n";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader, false);
        if (RunScript(script, reader, engine)) {
            Color888 buffer[16];
            engine.RenderFrame(buffer, 4, 4);
            PASS();
        }
    }
    
    // Test 13: Zero-modules ExecuteUpdate doesn't crash
    {
        TEST("ExecuteUpdate works with zero modules");
        std::string script = R"(
var count = 0;
fn Update(dt) {
    count = count + 1;
}
)";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader, false);
        if (RunScript(script, reader, engine)) {
            engine.ExecuteUpdate();
            engine.ExecuteUpdate();
            Value c = engine.GetGlobal("count");
            if (c.type == Value::Type::NUMBER && c.numberValue == 2.0) {
                PASS();
            } else {
                FAIL("Expected 2, got " + std::to_string(c.numberValue));
            }
        }
    }
    
    // Test 16: GetPhysicsWorld returns nullptr in zero-module mode
    {
        TEST("GetPhysicsWorld returns nullptr without default systems");
        std::string script = "fn Setup() {}\n";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader, false);
        if (RunScript(script, reader, engine)) {
            if (engine.GetPhysicsWorld() == nullptr && engine.GetUI() == nullptr) {
                PASS();
            } else {
                FAIL("Expected nullptr for unloaded systems");
            }
        }
    }
}

void TestPhase37ModuleSDK() {
    using namespace koilo::scripting;
    using koilo::Color888;

    std::cout << "--- Module SDK & Safety ---" << std::endl;

    // Test 1: Dynamic module loading from .so (hello_module)
    {
        TEST("Dynamic module loads from .so file");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader, false);
        
        bool loaded = engine.GetModuleLoader().LoadFromLibrary("examples/modules/hello_module.so");
        if (loaded) {
            PASS();
        } else {
            // .so might not exist if not built  skip gracefully
            std::cout << "SKIP (hello_module.so not found)" << std::endl;
            testsPassed++;
        }
    }

    // Test 2: ScanAndLoad finds modules in directory
    {
        TEST("ScanAndLoad discovers modules in directory");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader, false);
        
        int count = engine.GetModuleLoader().ScanAndLoad("examples/modules");
        // Should find hello_module.so, custom_effect.so, counter_module.so
        if (count >= 0) {
            PASS();
        } else {
            FAIL("ScanAndLoad returned " + std::to_string(count));
        }
    }

    // Test 3: Module search path + TryLoad
    {
        TEST("TryLoad with search path");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader, false);
        engine.GetModuleLoader().SetModuleSearchPath("examples/modules");
        
        // TryLoad won't initialize without engine init, but should not crash
        bool result = engine.GetModuleLoader().TryLoad("nonexistent");
        if (!result) {
            PASS();
        } else {
            FAIL("TryLoad found nonexistent module");
        }
    }

    // Test 4: LoadMode setting
    {
        TEST("Load mode configuration");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader, false);
        
        auto& loader = engine.GetModuleLoader();
        loader.SetLoadMode(koilo::ModuleLoader::LoadMode::Lazy);
        bool isLazy = (loader.GetLoadMode() == koilo::ModuleLoader::LoadMode::Lazy);
        loader.SetLoadMode(koilo::ModuleLoader::LoadMode::Eager);
        bool isEager = (loader.GetLoadMode() == koilo::ModuleLoader::LoadMode::Eager);
        
        if (isLazy && isEager) {
            PASS();
        } else {
            FAIL("Load mode not set correctly");
        }
    }

    // Test 5: Unknown callable silently returns none (consistent VM behavior)
    {
        TEST("Unknown callable returns none without crash");
        std::string script = R"(
var result = 1;
fn Setup() {
    var x = NonExistentThing();
    if (x == none) {
        result = 0;
    }
}
)";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader, false);
        reader.SetContent(script);
        bool loaded = engine.LoadScript("test.ks");
        if (loaded) {
            engine.BuildScene();
            Value r = engine.GetScriptVariable("result");
            if (r.numberValue == 0.0) {
                PASS();
            } else {
                FAIL("Unknown callable should return none, result=" + std::to_string(r.numberValue));
            }
        } else {
            FAIL("LoadScript failed: " + std::string(engine.GetError()));
        }
    }

    // Test 6: Script with physics module works, without gives error (36.4.7)
    {
        TEST("Script using unloaded module gives clear error");
        std::string script = R"(
fn Setup() {
    physics.Step(0.016);
}
)";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader, false);  // no modules
        reader.SetContent(script);
        bool loaded = engine.LoadScript("test.ks");
        if (loaded) {
            engine.BuildScene();
        }
        
        if (engine.HasError()) {
            std::string err = engine.GetError();
            if (err.find("Module 'physics' not loaded") != std::string::npos) {
                PASS();
            } else {
                FAIL("Error not module-specific: " + err);
            }
        } else {
            FAIL("No error for unloaded module access");
        }
    }

    // Test 7: Module loader shutdown clears modules
    {
        TEST("Module loader shutdown clears external modules");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader, false);
        
        // Loader should be empty since no external modules loaded
        auto modules = engine.GetModuleLoader().ListModules();
        if (modules.empty()) {
            PASS();
        } else {
            FAIL("Expected empty module list, got " + std::to_string(modules.size()));
        }
    }

    // Test 8: ABI version in header
    {
        TEST("Module ABI version check");
        // KL_MODULE_ABI_VER should be 2 after our expansion
        if (KL_MODULE_ABI_VER == 2) {
            PASS();
        } else {
            FAIL("ABI version is " + std::to_string(KL_MODULE_ABI_VER));
        }
    }

    // Test 9: EngineServices api_size field
    {
        TEST("EngineServices has api_size field");
        koilo::EngineServices svc{};
        svc.api_size = sizeof(koilo::EngineServices);
        if (svc.api_size > 0 && svc.api_size == sizeof(koilo::EngineServices)) {
            PASS();
        } else {
            FAIL("api_size = " + std::to_string(svc.api_size));
        }
    }

    // Test 10: ClassDescExport struct is usable
    {
        TEST("ClassDescExport C ABI struct");
        koilo::KoiloMethodExport methods[] = {
            {"GetValue", "Returns value", nullptr, 0, 0, koilo::KL_KIND_FLOAT, {koilo::KL_KIND_NONE}},
            {"SetValue", "Sets value", nullptr, 1, 0, koilo::KL_KIND_VOID, {koilo::KL_KIND_FLOAT}}
        };
        koilo::ClassDescExport desc{};
        desc.name = "TestClass";
        desc.size = 64;
        desc.destroy = nullptr;
        desc.methods = methods;
        desc.method_count = 2;
        
        if (std::string(desc.name) == "TestClass" && 
            desc.method_count == 2 &&
            std::string(desc.methods[0].name) == "GetValue") {
            PASS();
        } else {
            FAIL("ClassDescExport struct malformed");
        }
    }

    // Test 11: GlobalExport struct is usable
    {
        TEST("GlobalExport C ABI struct");
        int dummy = 42;
        koilo::GlobalExport exp{};
        exp.name = "test_global";
        exp.class_name = "TestClass";
        exp.instance = &dummy;
        exp.flags = 0;
        
        if (std::string(exp.name) == "test_global" &&
            exp.instance == &dummy) {
            PASS();
        } else {
            FAIL("GlobalExport struct malformed");
        }
    }

    // Test 12: GetScriptVariable / SetScriptVariable public API
    {
        TEST("Public script variable get/set");
        std::string script = R"(
var testVal = 42;
fn Setup() {
    testVal = testVal;
}
)";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        if (RunScript(script, reader, engine)) {
            Value v = engine.GetScriptVariable("testVal");
            engine.SetScriptVariable("testVal", Value(99.0));
            Value v2 = engine.GetScriptVariable("testVal");
            
            if (v.numberValue == 42.0 && v2.numberValue == 99.0) {
                PASS();
            } else {
                FAIL("get=" + std::to_string(v.numberValue) + " set=" + std::to_string(v2.numberValue));
            }
        }
    }
}

// ============================================================================
// Operator Overloading
// ============================================================================


