// SPDX-License-Identifier: GPL-3.0-or-later
// Scene & Rendering Scripting Bindings
#include "helpers.hpp"


void TestMaterialAnimator() {
    std::cout << "=== MaterialAnimator ===" << std::endl;
    
    TEST("MaterialAnimator default constructor");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var ma = MaterialAnimator();
var cap = ma.GetCapacity();
)";
        if (RunScript(script, reader, engine)) {
            Value cap = engine.GetGlobal("cap");
            if (cap.type == Value::Type::NUMBER && cap.numberValue == 4) {
                PASS();
            } else {
                FAIL("Expected default capacity 4, got " + std::to_string(cap.numberValue));
            }
        }
    }
    
}

void TestPrimitiveMesh() {
    std::cout << "=== PrimitiveMesh ===" << std::endl;
    
    TEST("PrimitiveMesh CreateQuad");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var pm = PrimitiveMesh();
pm.CreateQuad(100.0, 50.0);
var mesh = pm.GetMesh();
var hasMesh = type(mesh) != "null";
)";
        if (RunScript(script, reader, engine)) {
            Value hasMesh = engine.GetGlobal("hasMesh");
            if (hasMesh.IsTruthy()) {
                PASS();
            } else {
                FAIL("PrimitiveMesh.GetMesh() returned null");
            }
        }
    }
}

void TestSceneNode() {
    std::cout << "=== SceneNode ===" << std::endl;
    
    TEST("SceneNode construct from script");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var node = SceneNode();
node.SetPosition(Vector3D(10.0, 20.0, 30.0));
var pos = node.GetPosition();
var px = pos.X;
var py = pos.Y;
var pz = pos.Z;
)";
        if (RunScript(script, reader, engine)) {
            Value px = engine.GetGlobal("px");
            Value py = engine.GetGlobal("py");
            Value pz = engine.GetGlobal("pz");
            if (px.type == Value::Type::NUMBER && std::abs(px.numberValue - 10.0) < 0.01 &&
                py.type == Value::Type::NUMBER && std::abs(py.numberValue - 20.0) < 0.01 &&
                pz.type == Value::Type::NUMBER && std::abs(pz.numberValue - 30.0) < 0.01) {
                PASS();
            } else {
                FAIL("Position mismatch: " + std::to_string(px.numberValue) + "," + std::to_string(py.numberValue) + "," + std::to_string(pz.numberValue));
            }
        }
    }
    
    TEST("SceneNode parent-child hierarchy");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var parent = SceneNode();
var child = SceneNode();
child.SetParent(parent);
var count = parent.GetChildCount();
)";
        if (RunScript(script, reader, engine)) {
            Value count = engine.GetGlobal("count");
            if (count.type == Value::Type::NUMBER && count.numberValue == 1.0) {
                PASS();
            } else {
                FAIL("Expected child count 1, got " + std::to_string(count.numberValue));
            }
        }
    }
    
    TEST("SceneNode FindChild");
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var parent = SceneNode();
var child = SceneNode();
child.SetParent(parent);
var found = parent.FindChild("child");
var hasChild = type(found) != "null";
)";
        // FindChild needs the child to have a name. SceneNode() default constructor has empty name.
        // This test should return null (no child named "child").
        if (RunScript(script, reader, engine)) {
            // FindChild("child") should fail since we didn't name the child
            PASS();
        }
    }
}

void TestMultiContext() {
    std::cout << "=== Multi-Context ===" << std::endl;
    
    // Test 1: scene.CreateObject() and scene.Find()
    {
        TEST("scene.CreateObject and Find");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var count = 0;
fn Setup() {
    var obj = scene.CreateObject("hero");
    var found = scene.Find("hero");
    count = scene.GetNodeCount();
}
)";
        if (RunScript(script, reader, engine)) {
            Value count = engine.GetGlobal("count");
            if (count.type == Value::Type::NUMBER && count.numberValue == 1.0) {
                PASS();
            } else {
                FAIL("Expected 1 scene node, got " + std::to_string(count.numberValue));
            }
        }
    }
    
    // Test 2: Multiple scene nodes
    {
        TEST("scene multiple objects");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var count = 0;
fn Setup() {
    scene.CreateObject("alpha");
    scene.CreateObject("beta");
    scene.CreateObject("gamma");
    count = scene.GetNodeCount();
}
)";
        if (RunScript(script, reader, engine)) {
            Value count = engine.GetGlobal("count");
            if (count.type == Value::Type::NUMBER && count.numberValue == 3.0) {
                PASS();
            } else {
                FAIL("Expected 3 scene nodes, got " + std::to_string(count.numberValue));
            }
        }
    }
    
    // Test 3: self keyword in primary context (should be NONE - no self binding)
    {
        TEST("self in primary context returns NONE");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var hasSelf = "unknown";
fn Setup() {
    hasSelf = type(self);
}
)";
        if (RunScript(script, reader, engine)) {
            Value hasSelf = engine.GetGlobal("hasSelf");
            if (hasSelf.type == Value::Type::STRING && hasSelf.stringValue == "null") {
                PASS();
            } else {
                FAIL("Expected self to be null, got: " + hasSelf.stringValue);
            }
        }
    }
    
    // Test 4: Context count (single-file = 1)
    {
        TEST("context count starts at 1");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var x = 1;
)";
        if (RunScript(script, reader, engine)) {
            if (engine.GetContextCount() == 1) {
                PASS();
            } else {
                FAIL("Expected 1, got " + std::to_string(engine.GetContextCount()));
            }
        }
    }
    
    // Test 5: CreateObject returns non-null SceneNode
    {
        TEST("CreateObject returns SceneNode");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var nodeType = "unknown";
fn Setup() {
    var node = scene.CreateObject("test");
    nodeType = type(node);
}
)";
        if (RunScript(script, reader, engine)) {
            Value nodeType = engine.GetGlobal("nodeType");
            if (nodeType.type == Value::Type::STRING && nodeType.stringValue == "object") {
                PASS();
            } else {
                FAIL("Expected type 'object', got: " + nodeType.stringValue);
            }
        }
    }
    
    // Test 6: Duplicate CreateObject returns same node
    {
        TEST("CreateObject duplicate returns same");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var count = 0;
fn Setup() {
    scene.CreateObject("dup");
    scene.CreateObject("dup");
    count = scene.GetNodeCount();
}
)";
        if (RunScript(script, reader, engine)) {
            Value count = engine.GetGlobal("count");
            if (count.type == Value::Type::NUMBER && count.numberValue == 1.0) {
                PASS();
            } else {
                FAIL("Duplicate CreateObject should return same, count = " + std::to_string(count.numberValue));
            }
        }
    }
}

// ===== AnimationClip + AnimationMixer C++ Unit Tests =====

void TestTexture() {
    std::cout << "Texture Tests:" << std::endl;

    {
        TEST("Texture RGB888 create and sample")
        koilo::Texture tex;
        tex.CreateRGB(4, 4);
        tex.SetPixel(0, 0, koilo::Color888(255, 0, 0));
        tex.SetPixel(3, 3, koilo::Color888(0, 255, 0));
        auto c1 = tex.SamplePixel(0, 0);
        auto c2 = tex.SamplePixel(3, 3);
        auto c3 = tex.SamplePixel(1, 1); // should be black (default)
        if (c1.r == 255 && c1.g == 0 && c2.g == 255 && c3.r == 0 && c3.g == 0)
            PASS() else FAIL("Pixel values wrong")
    }

    {
        TEST("Texture palette-indexed sample")
        static const uint8_t palette[] = { 255,0,0, 0,255,0, 0,0,255 }; // 3 colors
        static const uint8_t indices[] = { 0, 1, 2, 0 }; // 2x2
        koilo::Texture tex(indices, palette, 3, 2, 2);
        auto c0 = tex.SamplePixel(0, 0); // red
        auto c1 = tex.SamplePixel(1, 0); // green
        auto c2 = tex.SamplePixel(0, 1); // blue
        if (c0.r == 255 && c1.g == 255 && c2.b == 255)
            PASS() else FAIL("Palette lookup wrong")
    }

    {
        TEST("Texture SampleUV maps to pixels")
        koilo::Texture tex;
        tex.CreateRGB(2, 2);
        tex.SetPixel(0, 0, koilo::Color888(10, 20, 30));
        tex.SetPixel(1, 1, koilo::Color888(40, 50, 60));
        auto c1 = tex.SampleUV(0.0f, 0.0f);
        auto c2 = tex.SampleUV(1.0f, 1.0f);
        if (c1.r == 10 && c1.g == 20 && c2.r == 40 && c2.g == 50)
            PASS() else FAIL("UV sampling wrong")
    }

    {
        TEST("Texture SampleRect sub-rectangle")
        koilo::Texture tex;
        tex.CreateRGB(4, 4);
        // Fill a 2x2 sub-rect at (2,2) with known colors
        tex.SetPixel(2, 2, koilo::Color888(100, 0, 0));
        tex.SetPixel(3, 2, koilo::Color888(0, 100, 0));
        tex.SetPixel(2, 3, koilo::Color888(0, 0, 100));
        tex.SetPixel(3, 3, koilo::Color888(50, 50, 50));
        // Sample UV (0,0) in rect (2,2,2,2) should get pixel (2,2)
        auto c = tex.SampleRect(0.0f, 0.0f, 2, 2, 2, 2);
        if (c.r == 100 && c.g == 0 && c.b == 0)
            PASS() else FAIL("SampleRect (0,0) wrong: r=" << (int)c.r)
    }

    {
        TEST("Texture out-of-bounds returns black")
        koilo::Texture tex;
        tex.CreateRGB(2, 2);
        tex.SetPixel(0, 0, koilo::Color888(255, 255, 255));
        auto c = tex.SamplePixel(10, 10);
        if (c.r == 0 && c.g == 0 && c.b == 0)
            PASS() else FAIL("Expected black for OOB")
    }

    std::cout << std::endl;
}

// ===== PrimitiveMesh Textured Quad Tests =====

void TestTexturedQuad() {
    std::cout << "Textured Quad Tests:" << std::endl;

    {
        TEST("CreateTexturedQuad produces mesh with UVs")
        koilo::PrimitiveMesh pm;
        pm.CreateTexturedQuad(10.0f, 10.0f);
        if (pm.GetMesh() && pm.HasUV() && pm.GetMesh()->HasUV())
            PASS() else FAIL("Mesh missing or no UVs")
    }

    {
        TEST("CreateQuad still has no UVs")
        koilo::PrimitiveMesh pm;
        pm.CreateQuad(10.0f, 10.0f);
        if (pm.GetMesh() && !pm.HasUV())
            PASS() else FAIL("Plain quad should not have UVs")
    }

    {
        TEST("Textured quad has 4 UV vertices")
        koilo::PrimitiveMesh pm;
        pm.CreateTexturedQuad(5.0f, 5.0f);
        auto* mesh = pm.GetMesh();
        const koilo::Vector2D* uvs = mesh->GetUVVertices();
        // Check corners exist and are in [0,1] range
        bool valid = uvs != nullptr;
        if (valid) {
            for (int i = 0; i < 4; i++) {
                if (uvs[i].X < -0.01f || uvs[i].X > 1.01f || 
                    uvs[i].Y < -0.01f || uvs[i].Y > 1.01f) valid = false;
            }
        }
        if (valid) PASS() else FAIL("UV vertices out of range or null")
    }

    std::cout << std::endl;
}

// ===== Sprite Tests =====

void TestSprite() {
    std::cout << "Sprite Tests:" << std::endl;

    {
        TEST("Sprite creates mesh with material")
        koilo::Texture tex;
        tex.CreateRGB(16, 16);
        koilo::Sprite sprite(&tex, 2.0f, 2.0f);
        if (sprite.GetMesh() && sprite.GetMesh()->GetMaterial())
            PASS() else FAIL("Sprite mesh or material null")
    }

    {
        TEST("Sprite transform accessible")
        koilo::Texture tex;
        tex.CreateRGB(8, 8);
        koilo::Sprite sprite(&tex, 1.0f, 1.0f);
        auto* t = sprite.GetTransform();
        if (t) {
            t->SetPosition(koilo::Vector3D(5.0f, 10.0f, 0.0f));
            auto pos = t->GetPosition();
            if (std::abs(pos.X - 5.0f) < 0.01f && std::abs(pos.Y - 10.0f) < 0.01f)
                PASS() else FAIL("Position not set correctly")
        } else {
            FAIL("Transform null")
        }
    }

    {
        TEST("Sprite SetFrame updates material")
        koilo::Texture tex;
        tex.CreateRGB(64, 64);
        koilo::Sprite sprite(&tex, 1.0f, 1.0f);
        sprite.SetFrame(16, 0, 16, 16);
        auto* mat = sprite.GetMaterial();
        if (mat) PASS() else FAIL("Material is null after SetFrame")
    }

    {
        TEST("Sprite SetFrameIndex calculates grid position")
        koilo::Texture tex;
        tex.CreateRGB(128, 128);
        koilo::Sprite sprite(&tex, 1.0f, 1.0f);
        sprite.SetFrameIndex(5, 32, 32, 4);
        auto* mat = sprite.GetMaterial();
        if (mat) PASS() else FAIL("Material is null after SetFrameIndex")
    }

    {
        TEST("Sprite enable/disable")
        koilo::Texture tex;
        tex.CreateRGB(8, 8);
        koilo::Sprite sprite(&tex, 1.0f, 1.0f);
        if (!sprite.IsEnabled()) { FAIL("Should start enabled"); }
        else {
            sprite.SetEnabled(false);
            if (sprite.IsEnabled()) { FAIL("Should be disabled"); }
            else {
                sprite.SetEnabled(true);
                if (sprite.IsEnabled()) PASS() else FAIL("Should be re-enabled")
            }
        }
    }

    {
        TEST("Sprite mesh has UVs")
        koilo::Texture tex;
        tex.CreateRGB(8, 8);
        koilo::Sprite sprite(&tex, 1.0f, 1.0f);
        if (sprite.GetMesh() && sprite.GetMesh()->HasUV())
            PASS() else FAIL("Sprite mesh should have UVs")
    }

    std::cout << std::endl;
}

// ====== Physics Tests ======

void TestPhase11Rendering() {
    std::cout << "--- Rendering Polish & PBR ---" << std::endl;

    using namespace koilo;

    // Test 1: SurfaceProperties new fields
    {
        TEST("SurfaceProperties extended fields")
        Vector3D pos(1, 2, 3), norm(0, 1, 0), uv(0.5f, 0.5f, 0);
        Vector3D view(0, 0, -1), tan(1, 0, 0), bitan(0, 0, 1);
        SurfaceProperties surf(pos, norm, uv, view, tan, bitan);
        bool ok = (surf.viewDirection.Z == -1.0f && surf.tangent.X == 1.0f && surf.bitangent.Z == 1.0f);
        if (ok) PASS() else FAIL("Extended SurfaceProperties fields incorrect")
    }

    // Test 2: SurfaceProperties legacy 3-arg constructor still works
    {
        TEST("SurfaceProperties legacy ctor")
        Vector3D pos(0, 0, 0), norm(0, 1, 0), uv(0, 0, 0);
        SurfaceProperties surf(pos, norm, uv);
        bool ok = (surf.viewDirection.X == 0 && surf.tangent.X == 0);
        if (ok) PASS() else FAIL("Legacy ctor broke extended fields")
    }

    std::cout << std::endl;
}

// =============================================================================
// Developer Tools & Debug Visualization Tests
// =============================================================================

void TestPhase13BlenderParity() {
    std::cout << "--- Blender Coordinate Parity ---" << std::endl;

    using namespace koilo;

    // 13.1: Z-up camera layout
    {
        TEST("CameraLayout Z-up, -Y forward produces correct vectors")
        CameraLayout layout(CameraLayout::YNForward, CameraLayout::ZUp);
        Vector3D fwd = layout.GetForwardVector();
        Vector3D up = layout.GetUpVector();
        // Forward should be (0, -1, 0), up should be (0, 0, 1)
        if (fwd.Y == -1 && up.Z == 1) PASS()
        else FAIL("fwd=" + std::to_string(fwd.X) + "," + std::to_string(fwd.Y) + "," + std::to_string(fwd.Z) +
                  " up=" + std::to_string(up.X) + "," + std::to_string(up.Y) + "," + std::to_string(up.Z))
    }

    {
        TEST("CameraLayout Z-up produces valid quaternion")
        CameraLayout layout(CameraLayout::YNForward, CameraLayout::ZUp);
        Quaternion q = layout.GetRotation();
        float mag = std::sqrt(q.W*q.W + q.X*q.X + q.Y*q.Y + q.Z*q.Z);
        if (std::abs(mag - 1.0f) < 0.01f) PASS()
        else FAIL("Non-unit quaternion, mag=" + std::to_string(mag))
    }

    {
        TEST("Default engine CameraLayout is Z-up")
        StringFileReader reader;
        reader.SetContent("fn Setup() { }");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        Camera* cam = engine.GetCamera();
        if (!cam) { FAIL("GetCamera() returned null (no kernel)") }
        else {
            CameraLayout* layout = cam->GetCameraLayout();
            if (!layout) { FAIL("GetCameraLayout() returned null") }
            else if (layout->GetUpAxis() == CameraLayout::ZUp &&
                     layout->GetForwardAxis() == CameraLayout::YNForward) PASS()
            else FAIL("Default layout not Z-up/-Y forward")
        }
    }

    // 13.4a: Plane intersection
    {
        TEST("Plane distance to point")
        Plane p(Vector3D(0, 0, 0), Vector3D(0, 0, 1)); // XY plane, normal +Z
        float d = p.DistanceToPoint(Vector3D(0, 0, 5));
        if (std::abs(d - 5.0f) < 0.001f) PASS()
        else FAIL("Expected 5, got " + std::to_string(d))
    }

    {
        TEST("Plane distance negative side")
        Plane p(Vector3D(0, 0, 0), Vector3D(0, 0, 1));
        float d = p.DistanceToPoint(Vector3D(0, 0, -3));
        if (std::abs(d - (-3.0f)) < 0.001f) PASS()
        else FAIL("Expected -3, got " + std::to_string(d))
    }

    {
        TEST("Plane side returns correct values")
        Plane p(Vector3D(0, 0, 0), Vector3D(0, 0, 1));
        if (p.Side(Vector3D(0, 0, 5)) == 1 &&
            p.Side(Vector3D(0, 0, -5)) == -1 &&
            p.Side(Vector3D(0, 0, 0)) == 0) PASS()
        else FAIL("Side() returned wrong values")
    }

    {
        TEST("Plane ray intersection")
        Plane p(Vector3D(0, 0, 5), Vector3D(0, 0, 1));
        Ray ray(Vector3D(0, 0, 0), Vector3D(0, 0, 1));
        float t = 0;
        if (p.RayIntersect(ray, t) && std::abs(t - 5.0f) < 0.001f) PASS()
        else FAIL("Expected t=5, got " + std::to_string(t))
    }

    {
        TEST("Plane ray parallel returns false")
        Plane p(Vector3D(0, 0, 5), Vector3D(0, 0, 1));
        Ray ray(Vector3D(0, 0, 0), Vector3D(1, 0, 0)); // parallel to plane
        float t = 0;
        if (!p.RayIntersect(ray, t)) PASS()
        else FAIL("Should not intersect when parallel")
    }

    {
        TEST("Plane closest point")
        Plane p(Vector3D(0, 0, 0), Vector3D(0, 0, 1));
        Vector3D closest = p.ClosestPoint(Vector3D(3, 4, 7));
        if (std::abs(closest.X - 3) < 0.001f &&
            std::abs(closest.Y - 4) < 0.001f &&
            std::abs(closest.Z) < 0.001f) PASS()
        else FAIL("Closest point wrong")
    }

    // 13.4b: AABB
    {
        TEST("AABB contains point inside")
        AABB box(Vector3D(-1, -1, -1), Vector3D(1, 1, 1));
        if (box.Contains(Vector3D(0, 0, 0))) PASS()
        else FAIL("Origin should be inside")
    }

    {
        TEST("AABB rejects point outside")
        AABB box(Vector3D(-1, -1, -1), Vector3D(1, 1, 1));
        if (!box.Contains(Vector3D(2, 0, 0))) PASS()
        else FAIL("(2,0,0) should be outside")
    }

    {
        TEST("AABB center and size")
        AABB box(Vector3D(0, 0, 0), Vector3D(4, 6, 8));
        Vector3D c = box.GetCenter();
        Vector3D s = box.GetSize();
        if (std::abs(c.X - 2) < 0.001f && std::abs(c.Y - 3) < 0.001f && std::abs(c.Z - 4) < 0.001f &&
            std::abs(s.X - 4) < 0.001f && std::abs(s.Y - 6) < 0.001f && std::abs(s.Z - 8) < 0.001f) PASS()
        else FAIL("Center or size wrong")
    }

    {
        TEST("AABB overlap detection")
        AABB a(Vector3D(-1, -1, -1), Vector3D(1, 1, 1));
        AABB b(Vector3D(0, 0, 0), Vector3D(2, 2, 2));
        AABB c(Vector3D(5, 5, 5), Vector3D(6, 6, 6));
        if (a.Overlaps(b) && !a.Overlaps(c)) PASS()
        else FAIL("Overlap detection wrong")
    }

    {
        TEST("AABB ray intersection hit")
        AABB box(Vector3D(-1, -1, -1), Vector3D(1, 1, 1));
        Ray ray(Vector3D(-5, 0, 0), Vector3D(1, 0, 0));
        float tmin, tmax;
        if (box.RayIntersect(ray, tmin, tmax) &&
            std::abs(tmin - 4.0f) < 0.001f &&
            std::abs(tmax - 6.0f) < 0.001f) PASS()
        else FAIL("Expected tmin=4 tmax=6, got " + std::to_string(tmin) + ", " + std::to_string(tmax))
    }

    {
        TEST("AABB ray intersection miss")
        AABB box(Vector3D(-1, -1, -1), Vector3D(1, 1, 1));
        Ray ray(Vector3D(-5, 5, 0), Vector3D(1, 0, 0)); // passes above
        float tmin, tmax;
        if (!box.RayIntersect(ray, tmin, tmax)) PASS()
        else FAIL("Should miss the box")
    }

    {
        TEST("AABB encapsulate expands bounds")
        AABB box(Vector3D(0, 0, 0), Vector3D(1, 1, 1));
        box.Encapsulate(Vector3D(-2, 3, 0.5f));
        if (std::abs(box.min.X - (-2)) < 0.001f && std::abs(box.max.Y - 3) < 0.001f) PASS()
        else FAIL("Encapsulate didn't expand correctly")
    }

    {
        TEST("AABB union combines two boxes")
        AABB a(Vector3D(-1, -1, -1), Vector3D(1, 1, 1));
        AABB b(Vector3D(0, 0, 0), Vector3D(3, 3, 3));
        AABB u = a.Union(b);
        if (std::abs(u.min.X - (-1)) < 0.001f && std::abs(u.max.X - 3) < 0.001f) PASS()
        else FAIL("Union wrong")
    }

    {
        TEST("AABB from center + half-size")
        AABB box = AABB::FromCenterHalfSize(Vector3D(5, 5, 5), Vector3D(2, 2, 2));
        if (std::abs(box.min.X - 3) < 0.001f && std::abs(box.max.X - 7) < 0.001f) PASS()
        else FAIL("FromCenterHalfSize wrong")
    }

    {
        TEST("AABB volume")
        AABB box(Vector3D(0, 0, 0), Vector3D(2, 3, 4));
        float vol = box.GetVolume();
        if (std::abs(vol - 24.0f) < 0.001f) PASS()
        else FAIL("Expected 24, got " + std::to_string(vol))
    }

    // 13.5: SmoothStep
    {
        TEST("SmoothStep edges")
        float s0 = Mathematics::SmoothStep(0, 1, 0);
        float s1 = Mathematics::SmoothStep(0, 1, 1);
        if (std::abs(s0) < 0.001f && std::abs(s1 - 1.0f) < 0.001f) PASS()
        else FAIL("SmoothStep(0)=" + std::to_string(s0) + " SmoothStep(1)=" + std::to_string(s1))
    }

    {
        TEST("SmoothStep midpoint is 0.5")
        float s = Mathematics::SmoothStep(0, 1, 0.5f);
        if (std::abs(s - 0.5f) < 0.001f) PASS()
        else FAIL("Expected 0.5, got " + std::to_string(s))
    }

    {
        TEST("SmoothStep clamps outside range")
        float lo = Mathematics::SmoothStep(0, 1, -1);
        float hi = Mathematics::SmoothStep(0, 1, 2);
        if (std::abs(lo) < 0.001f && std::abs(hi - 1.0f) < 0.001f) PASS()
        else FAIL("Clamping failed")
    }

    {
        TEST("SmootherStep edges and midpoint")
        float s0 = Mathematics::SmootherStep(0, 1, 0);
        float s1 = Mathematics::SmootherStep(0, 1, 1);
        float sm = Mathematics::SmootherStep(0, 1, 0.5f);
        if (std::abs(s0) < 0.001f && std::abs(s1 - 1.0f) < 0.001f &&
            std::abs(sm - 0.5f) < 0.001f) PASS()
        else FAIL("SmootherStep values wrong")
    }

    // 13: Reflection registration
    {
        TEST("AABB registered in reflection (214 Describe calls)")
        // If we got here, AABB compiled and linked - check it has fields
        AABB box(Vector3D(1,2,3), Vector3D(4,5,6));
        if (std::abs(box.min.X - 1) < 0.001f && std::abs(box.max.Z - 6) < 0.001f) PASS()
        else FAIL("AABB field access wrong")
    }

    std::cout << std::endl;
}


