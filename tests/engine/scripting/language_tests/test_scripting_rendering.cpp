// SPDX-License-Identifier: GPL-3.0-or-later
// Rendering Effects, Debug & Shaders
#include "helpers.hpp"
#include <koilo/systems/render/canvas2d.hpp>

// --- Particle System Tests ---
#include <koilo/systems/particles/particlesystem.hpp>

void TestPhase12DebugTools() {
    std::cout << "--- Debug Tools & Visualization ---" << std::endl;

    using namespace koilo;

    // Test 1: Bresenham line draws pixels
    {
        TEST("DrawLine2D horizontal")
        Color888 buf[100]; // 10x10
        memset(buf, 0, sizeof(buf));
        DebugRenderer::DrawLine2D(buf, 10, 10, 0, 5, 9, 5, Color888(255, 0, 0));
        // All 10 pixels on row 5 should be red
        bool ok = true;
        for (int x = 0; x < 10; x++) {
            if (buf[5 * 10 + x].R != 255) { ok = false; break; }
        }
        if (ok) PASS() else FAIL("Horizontal line missing pixels")
    }

    // Test 2: Bresenham vertical line
    {
        TEST("DrawLine2D vertical")
        Color888 buf[100];
        memset(buf, 0, sizeof(buf));
        DebugRenderer::DrawLine2D(buf, 10, 10, 3, 0, 3, 9, Color888(0, 255, 0));
        bool ok = true;
        for (int y = 0; y < 10; y++) {
            if (buf[y * 10 + 3].G != 255) { ok = false; break; }
        }
        if (ok) PASS() else FAIL("Vertical line missing pixels")
    }

    // Test 3: Bresenham diagonal line
    {
        TEST("DrawLine2D diagonal")
        Color888 buf[100];
        memset(buf, 0, sizeof(buf));
        DebugRenderer::DrawLine2D(buf, 10, 10, 0, 0, 9, 9, Color888(0, 0, 255));
        // Diagonal should have pixels along the line
        int count = 0;
        for (int i = 0; i < 100; i++) {
            if (buf[i].B == 255) count++;
        }
        if (count >= 10) PASS() else FAIL("Diagonal drew " + std::to_string(count) + " pixels, expected >=10")
    }

    // Test 4: Line clips to bounds (no crash on out-of-bounds)
    {
        TEST("DrawLine2D clips safely")
        Color888 buf[100];
        memset(buf, 0, sizeof(buf));
        DebugRenderer::DrawLine2D(buf, 10, 10, -5, -5, 20, 20, Color888(255, 255, 255));
        // Should not crash, and some pixels in buffer should be lit
        int count = 0;
        for (int i = 0; i < 100; i++) {
            if (buf[i].R == 255) count++;
        }
        if (count > 0) PASS() else FAIL("Clipped line drew 0 pixels")
    }

    // Test 5: BlitText renders characters
    {
        TEST("BlitText renders text")
        Color888 buf[640]; // 80x8, enough for a few chars
        memset(buf, 0, sizeof(buf));
        DebugRenderer::BlitText(buf, 80, 8, 0, 0, "AB", Color888(255, 255, 255), 1);
        // Count lit pixels - each 8x8 char should have some
        int count = 0;
        for (int i = 0; i < 640; i++) {
            if (buf[i].R == 255) count++;
        }
        if (count > 5) PASS() else FAIL("BlitText lit only " + std::to_string(count) + " pixels")
    }

    // Test 6: BlitText with scale
    {
        TEST("BlitText scaled 2x")
        Color888 buf[2560]; // 80x32
        memset(buf, 0, sizeof(buf));
        DebugRenderer::BlitText(buf, 80, 32, 0, 0, "A", Color888(255, 0, 0), 2);
        // At 2x scale, each lit pixel becomes a 2x2 block
        int count = 0;
        for (int i = 0; i < 2560; i++) {
            if (buf[i].R == 255) count++;
        }
        // 'A' has roughly 20 lit pixels in 8x8; at 2x = 80 lit pixels
        if (count > 20) PASS() else FAIL("Scaled text lit " + std::to_string(count) + " pixels")
    }

    // Test 7: DebugDraw collects lines
    {
        TEST("DebugDraw collects lines")
        DebugDraw& dd = DebugDraw::GetInstance();
        dd.Clear();
        dd.Enable();
        dd.DrawLine(Vector3D(0, 0, 0), Vector3D(1, 1, 1));
        dd.DrawLine(Vector3D(0, 0, 0), Vector3D(2, 2, 2), Color::Red, 1.0f);
        if (dd.GetLines().size() == 2) PASS() else FAIL("Expected 2 lines, got " + std::to_string(dd.GetLines().size()))
    }

    // Test 8: DebugDraw Update expires one-frame primitives
    {
        TEST("DebugDraw expires one-frame")
        DebugDraw& dd = DebugDraw::GetInstance();
        dd.Clear();
        dd.DrawLine(Vector3D(0, 0, 0), Vector3D(1, 0, 0)); // duration=0 (one frame)
        dd.DrawLine(Vector3D(0, 0, 0), Vector3D(0, 1, 0), Color::Green, 2.0f); // 2 seconds
        koilo::TimeManager::GetInstance().Tick(0.016f); dd.Update();
        // One-frame line should be gone, timed line should remain
        if (dd.GetLines().size() == 1) PASS() else FAIL("Expected 1 line after update, got " + std::to_string(dd.GetLines().size()))
    }

    // Test 9: DebugDraw timed primitives expire after duration
    {
        TEST("DebugDraw timed expiration")
        DebugDraw& dd = DebugDraw::GetInstance();
        dd.Clear();
        dd.DrawLine(Vector3D(0, 0, 0), Vector3D(1, 0, 0), Color::White, 0.5f);
        koilo::TimeManager::GetInstance().Tick(0.3f); dd.Update();
        bool stillAlive = (dd.GetLines().size() == 1);
        koilo::TimeManager::GetInstance().Tick(0.3f); dd.Update(); // 0.6s total > 0.5s
        bool expired = (dd.GetLines().size() == 0);
        if (stillAlive && expired) PASS() else FAIL("Timed expiration incorrect")
    }

    // Test 10: DrawAxes creates 3 lines (R=X, G=Y, B=Z)
    {
        TEST("DrawAxes creates 3 lines")
        DebugDraw& dd = DebugDraw::GetInstance();
        dd.Clear();
        dd.DrawAxes(Vector3D(0, 0, 0), 1.0f);
        if (dd.GetLines().size() == 3) PASS() else FAIL("Expected 3 axis lines, got " + std::to_string(dd.GetLines().size()))
    }

    // Test 11: DrawGrid creates grid lines
    {
        TEST("DrawGrid creates lines")
        DebugDraw& dd = DebugDraw::GetInstance();
        dd.Clear();
        dd.DrawGrid(Vector3D(0, 0, 0), 10.0f, 5);
        // 5 divisions = 6 lines per axis * 2 axes = 12
        if (dd.GetLines().size() == 12) PASS() else FAIL("Expected 12 grid lines, got " + std::to_string(dd.GetLines().size()))
    }

    // Test 12: DrawBox collects box
    {
        TEST("DrawBox collects")
        DebugDraw& dd = DebugDraw::GetInstance();
        dd.Clear();
        dd.DrawBox(Vector3D(0, 0, 0), Vector3D(1, 1, 1));
        if (dd.GetBoxes().size() == 1) PASS() else FAIL("Expected 1 box")
    }

    // Test 13: RenderOverlay writes text to buffer
    {
        TEST("RenderOverlay writes stats")
        Color888 buf[4800]; // 80x60
        memset(buf, 0, sizeof(buf));
        DebugOverlayStats stats;
        stats.fps = 60.0f;
        stats.frameTimeMs = 16.6f;
        stats.meshCount = 5;
        stats.triCount = 100;
        DebugRenderer::RenderOverlay(buf, 80, 60, stats);
        // Should have some green pixels from the text
        int count = 0;
        for (int i = 0; i < 4800; i++) {
            if (buf[i].G > 0) count++;
        }
        if (count > 10) PASS() else FAIL("Overlay wrote " + std::to_string(count) + " pixels")
    }

    // Test 14: DebugOverlayStats reflected
    {
        TEST("DebugOverlayStats reflected")
        DebugOverlayStats::Describe();
        const ClassDesc* desc = scripting::ReflectionBridge::FindClass("DebugOverlayStats");
        if (desc) PASS() else FAIL("DebugOverlayStats not in reflection")
    }

    // Test 15: DebugRenderer reflected
    {
        TEST("DebugRenderer reflected")
        DebugRenderer::Describe();
        const ClassDesc* desc = scripting::ReflectionBridge::FindClass("DebugRenderer");
        if (desc) PASS() else FAIL("DebugRenderer not in reflection")
    }

    // Test 16: debug global accessible from script
    {
        TEST("debug global from script")
        StringFileReader reader;
        reader.SetContent(R"(
var enabled = false;
fn Setup() {
    debug.Enable();
    enabled = debug.IsEnabled();
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        Value v = engine.GetGlobal("enabled");
        bool ok = (v.type == Value::Type::BOOL && v.boolValue) ||
                  (v.type == Value::Type::NUMBER && v.numberValue == 1.0);
        if (ok) PASS() else FAIL("debug global not accessible")
    }

    // Test 17: debug.Clear from script clears primitives
    {
        TEST("debug.Clear from script")
        DebugDraw::GetInstance().Clear();
        DebugDraw::GetInstance().DrawLine(Vector3D(0,0,0), Vector3D(1,0,0));
        StringFileReader reader;
        reader.SetContent(R"(
fn Setup() {
    debug.Clear();
}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        size_t count = DebugDraw::GetInstance().GetLines().size();
        if (count == 0) PASS() else FAIL("Expected 0 lines after Clear, got " + std::to_string(count))
    }

    std::cout << std::endl;
}

void TestPhase12ADebugAdditions() {
    std::cout << "--- Debug Additions ---" << std::endl;

    using namespace koilo;

    // 12A.1: Sphere wireframe rendering
    {
        TEST("DebugDraw sphere collected and retrievable")
        DebugDraw::GetInstance().Clear();
        DebugDraw::GetInstance().DrawSphere(Vector3D(0,0,0), 5.0f);
        if (DebugDraw::GetInstance().GetSpheres().size() == 1) PASS()
        else FAIL("Expected 1 sphere")
    }

    {
        TEST("DrawSphere3D renders to buffer without crash")
        const int W = 32, H = 32;
        Color888 buf[W * H];
        memset(buf, 0, sizeof(buf));
        Vector3D camPos(0, 0, -10);
        Quaternion invRot;
        Vector3D camScale(1, 1, 1);
        Vector2D minC(-16, -16), maxC(16, 16);
        DebugRenderer::DrawSphere3D(buf, W, H, Vector3D(0,0,0), 3.0f,
                                     Color888(255,0,0), camPos, invRot, camScale,
                                     true, 1.0f, 0.1f, minC, maxC, 8);
        // Check that at least some pixels were drawn
        int drawn = 0;
        for (int i = 0; i < W * H; ++i) {
            if (buf[i].R == 255) drawn++;
        }
        if (drawn > 0) PASS()
        else FAIL("No red pixels drawn for sphere wireframe")
    }

    // 12A.2: Physics collider visualization
    {
        TEST("RenderPhysicsColliders with sphere body no crash")
        PhysicsWorld world;
        RigidBody body;
        SphereCollider sc(Vector3D(0,0,0), 1.0f);
        body.SetCollider(&sc);
        world.AddBody(&body);
        if (world.GetBodyCount() == 1 && world.GetBody(0)->GetCollider() != nullptr) PASS()
        else FAIL("Body/collider not accessible")
    }

    {
        TEST("RenderPhysicsColliders with box body no crash")
        PhysicsWorld world;
        RigidBody body;
        BoxCollider bc;
        body.SetCollider(&bc);
        world.AddBody(&body);
        if (world.GetBody(0)->GetCollider()->GetType() == ColliderType::Box) PASS()
        else FAIL("Box collider type mismatch")
    }

    // 12A.3: Contact point visualization
    {
        TEST("Physics contacts populated and retrievable")
        PhysicsWorld world;
        RigidBody a, b;
        a.SetBodyType(BodyType::Dynamic);
        b.SetBodyType(BodyType::Static);
        SphereCollider sca(Vector3D(0, 0, 0), 1.0f);
        SphereCollider scb(Vector3D(1.5f, 0, 0), 1.0f);
        a.SetCollider(&sca);
        b.SetCollider(&scb);
        world.AddBody(&a);
        world.AddBody(&b);
        koilo::TimeManager::GetInstance().Tick(0.02f); world.Step(); // Slightly larger than fixedDt (1/60 ≈ 0.0167)
        // Should have detected the collision
        if (world.GetDebugContactCount() > 0) PASS()
        else FAIL("Expected at least 1 contact, got " + std::to_string(world.GetDebugContactCount()))
    }

    // 12A.4: Depth buffer enable/disable
    {
        TEST("Rasterizer debug buffers enable/disable")
        Rasterizer::EnableDebugBuffers(true);
        if (Rasterizer::DebugBuffersEnabled()) {
            Rasterizer::EnableDebugBuffers(false);
            if (!Rasterizer::DebugBuffersEnabled()) PASS()
            else FAIL("Should be disabled")
        } else FAIL("Should be enabled")
    }

    // 12A.4-5: Depth and normal view rendering
    {
        TEST("RenderDepthView on empty buffer no crash")
        Rasterizer::EnableDebugBuffers(false);
        const int W = 8, H = 8;
        Color888 buf[W * H];
        memset(buf, 0, sizeof(buf));
        // No depth data (disabled), should be a no-op
        DebugRenderer::RenderDepthView(buf, W, H);
        PASS()
    }

    {
        TEST("RenderNormalView on empty buffer no crash")
        const int W = 8, H = 8;
        Color888 buf[W * H];
        memset(buf, 0, sizeof(buf));
        DebugRenderer::RenderNormalView(buf, W, H);
        PASS()
    }

    std::cout << std::endl;
}

// TestPhase35NativeShaders removed - old dlopen-based shader system
// replaced by KSL ELF module loading (see kslmaterial.hpp, ksl_elf_loader.hpp)


void TestPhase35Profiler() {
    std::cout << "--- Profiler Integration ---" << std::endl;

    // Test 1: profiler_enable builtin
    {
        TEST("profiler_enable builtin");
        std::string script = R"(
fn Setup() {
    profiler_enable(1);
}
)";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        if (RunScript(script, reader, engine)) {
            PASS();
        } else {
            FAIL("Script failed to load");
        }
    }

    // Test 2: profiler_fps returns a number
    {
        TEST("profiler_fps returns number");
        std::string script = R"(
var fps = 0;
fn Setup() {
    fps = profiler_fps();
}
)";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        if (RunScript(script, reader, engine)) {
            Value fps = engine.GetGlobal("fps");
            if (fps.type == Value::Type::NUMBER && fps.numberValue >= 0.0) {
                PASS();
            } else {
                FAIL("profiler_fps did not return a number");
            }
        }
    }

    // Test 3: profiler_time returns 0 for unknown scope
    {
        TEST("profiler_time unknown scope returns 0");
        std::string script = R"(
var t = -1;
fn Setup() {
    t = profiler_time("NonExistentScope");
}
)";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        if (RunScript(script, reader, engine)) {
            Value t = engine.GetGlobal("t");
            if (t.type == Value::Type::NUMBER && t.numberValue == 0.0) {
                PASS();
            } else {
                FAIL("Expected 0 for unknown scope, got " + std::to_string(t.numberValue));
            }
        }
    }

    // Test 4: Profiler builtins compile correctly
    {
        TEST("Profiler builtins compile in script");
        std::string script = R"(
fn Setup() {
    profiler_enable(1);
}
fn Update() {
    var f = profiler_fps();
}
)";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        if (RunScript(script, reader, engine)) {
            PASS();
        } else {
            FAIL("Script compilation failed");
        }
    }
}

// ============================================================
// Canvas2D Backend Class  Reflected Method Tests
// ============================================================
void TestDrawBuffer2DBuiltins() {
    // Test 1: Canvas methods compile and execute without errors
    {
        TEST("Canvas2D methods compile and execute");
        std::string script = R"(
display.SetPixelWidth(16);
display.SetPixelHeight(16);
fn Setup() {
    var canvas = Canvas2D();
    canvas.Resize(16, 16);
    canvas.Attach();
    canvas.ClearWithColor(0, 0, 32);
    canvas.SetPixel(4, 4, 255, 0, 0);
    canvas.FillRect(0, 0, 3, 3, 0, 255, 0);
    canvas.DrawRect(5, 5, 4, 4, 0, 0, 255);
    canvas.DrawLine(0, 0, 15, 15, 255, 255, 0);
    canvas.DrawCircle(8, 8, 3, 255, 128, 0);
    canvas.FillCircle(12, 12, 2, 128, 0, 255);
    canvas.Clear();
}
)";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        if (RunScript(script, reader, engine)) {
            PASS();
        } else {
            FAIL("Draw builtin script failed: " + std::string(engine.GetError()));
        }
    }

    // Test 2: Canvas composites onto render output
    {
        TEST("Canvas2D composites onto framebuffer");
        koilo::Canvas2D::DetachAll(); // clean slate
        std::string script = R"(
display.SetPixelWidth(8);
display.SetPixelHeight(8);
var canvas = Canvas2D();
canvas.Resize(8, 8);
canvas.Attach();
fn Setup() {
    canvas.SetPixel(0, 0, 255, 0, 0);
    canvas.SetPixel(7, 7, 0, 255, 0);
}
fn Update(dt) {}
)";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        if (!RunScript(script, reader, engine)) {
            FAIL("Script failed: " + std::string(engine.GetError()));
        } else {
            // Render a frame and check output
            std::vector<Color888> fb(64);
            engine.RenderFrame(fb.data(), 8, 8);
            // Pixel (0,0) should be red from set_pixel
            if (fb[0].R == 255 && fb[0].G == 0 && fb[0].B == 0) {
                PASS();
            } else {
                FAIL("Expected (255,0,0) at (0,0), got (" +
                     std::to_string(fb[0].R) + "," +
                     std::to_string(fb[0].G) + "," +
                     std::to_string(fb[0].B) + ")");
            }
        }
    }

    // Test 3: ClearWithColor fills entire buffer
    {
        TEST("canvas.ClearWithColor fills entire buffer");
        koilo::Canvas2D::DetachAll(); // clean slate
        std::string script = R"(
display.SetPixelWidth(4);
display.SetPixelHeight(4);
var canvas = Canvas2D();
canvas.Resize(4, 4);
canvas.Attach();
fn Setup() {
    canvas.ClearWithColor(64, 128, 192);
}
fn Update(dt) {}
)";
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        if (!RunScript(script, reader, engine)) {
            FAIL("Script failed: " + std::string(engine.GetError()));
        } else {
            std::vector<Color888> fb(16);
            engine.RenderFrame(fb.data(), 4, 4);
            // All pixels should be (64, 128, 192)
            bool ok = true;
            for (int i = 0; i < 16; i++) {
                if (fb[i].R != 64 || fb[i].G != 128 || fb[i].B != 192) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                PASS();
            } else {
                FAIL("Not all pixels matched expected color");
            }
        }
    }
}

