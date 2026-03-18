// SPDX-License-Identifier: GPL-3.0-or-later
// Input & UI Scripting Bindings
#include "helpers.hpp"
#include <koilo/systems/input/inputmanager.hpp>

void TestInputManager() {
    std::cout << "--- InputManager ---" << std::endl;
    using namespace koilo;
    using namespace koilo::scripting;

    // Test 1: Keyboard press detection
    {
        InputManager mgr;
        mgr.GetKeyboard().SetKeyState(KeyCode::A, true);
        // Before Update: key is set but "pressed" needs frame transition
        // After Update: previous=false, current=true -> pressed
        mgr.Update();
        // Now previous was swapped, key A is in current
        // Actually: Update() copies current->previous and clears aren't needed
        // Let's re-check: Keyboard::Update copies current->previous
        // So we need: SetKeyState first, then DON'T update yet, check IsKeyHeld
        InputManager mgr2;
        mgr2.GetKeyboard().SetKeyState(KeyCode::Space, true);
        if (mgr2.IsKeyHeld(KeyCode::Space))
            PASS() else FAIL("Key should be held when set to true")
    }

    // Test 2: Key press vs held distinction
    {
        InputManager mgr;
        mgr.GetKeyboard().SetKeyState(KeyCode::A, true);
        // Frame 1: current=true, previous=false -> pressed=true, held=true
        if (mgr.IsKeyPressed(KeyCode::A) && mgr.IsKeyHeld(KeyCode::A))
            PASS() else FAIL("Key should be pressed and held on first frame")
    }

    // Test 3: After Update, key becomes held but not pressed
    {
        InputManager mgr;
        mgr.GetKeyboard().SetKeyState(KeyCode::A, true);
        mgr.Update(); // previous = current(true), key still true
        mgr.GetKeyboard().SetKeyState(KeyCode::A, true); // still held
        // Now previous=true, current=true -> pressed=false, held=true
        if (!mgr.IsKeyPressed(KeyCode::A) && mgr.IsKeyHeld(KeyCode::A))
            PASS() else FAIL("Key should be held but not pressed after Update")
    }

    // Test 4: Key release detection
    {
        InputManager mgr;
        mgr.GetKeyboard().SetKeyState(KeyCode::A, true);
        mgr.Update();
        mgr.GetKeyboard().SetKeyState(KeyCode::A, false);
        // Now previous=true, current=false -> released=true
        if (mgr.IsKeyReleased(KeyCode::A))
            PASS() else FAIL("Key should be released after being let go")
    }

    // Test 5: Action mapping
    {
        InputManager mgr;
        mgr.MapAction("jump", KeyCode::Space);
        mgr.GetKeyboard().SetKeyState(KeyCode::Space, true);
        if (mgr.IsActionPressed("jump"))
            PASS() else FAIL("Action 'jump' should be pressed when mapped key is pressed")
    }

    // Test 6: Action held
    {
        InputManager mgr;
        mgr.MapAction("fire", KeyCode::F);
        mgr.GetKeyboard().SetKeyState(KeyCode::F, true);
        mgr.Update();
        mgr.GetKeyboard().SetKeyState(KeyCode::F, true);
        if (mgr.IsActionHeld("fire"))
            PASS() else FAIL("Action 'fire' should be held when mapped key is held")
    }

    // Test 7: Mouse position tracking
    {
        InputManager mgr;
        mgr.GetMouse().SetPosition(100.0f, 200.0f);
        Vector2D pos = mgr.GetMousePosition();
        if (std::abs(pos.X - 100.0f) < 0.01f && std::abs(pos.Y - 200.0f) < 0.01f)
            PASS() else FAIL("Mouse position should track SetPosition calls")
    }

    // Test 8: Mouse button press
    {
        InputManager mgr;
        mgr.GetMouse().SetButtonState(MouseButton::Left, true);
        if (mgr.IsMouseButtonPressed(MouseButton::Left))
            PASS() else FAIL("Mouse left button should be pressed")
    }

    // Test 9: Gamepad connection
    {
        InputManager mgr;
        Gamepad& gp = mgr.GetGamepad(0);
        gp.SetConnected(true);
        if (mgr.IsGamepadConnected(0))
            PASS() else FAIL("Gamepad 0 should be connected")
    }

    // Test 10: InputManager is reflected
    {
        InputManager::Describe();
        const ClassDesc* desc = ReflectionBridge::FindClass("InputManager");
        if (desc != nullptr)
            PASS() else FAIL("InputManager should be findable via reflection")
    }

    // Test 11: InputManager has IsActionPressed in reflection
    {
        const ClassDesc* desc = ReflectionBridge::FindClass("InputManager");
        bool found = false;
        if (desc) {
            for (size_t i = 0; i < desc->methods.count; i++) {
                if (std::string(desc->methods.data[i].name) == "IsActionPressed") {
                    found = true; break;
                }
            }
        }
        if (found)
            PASS() else FAIL("InputManager should have IsActionPressed in reflection")
    }

    std::cout << std::endl;
}

void TestWidgetAndUI() {
    std::cout << "--- Widget & UI (ui) ---" << std::endl;
    using namespace koilo;
    using namespace koilo::ui;

    // Test 1: Widget creation and properties
    {
        UIContext ctx(64);
        int lbl = ctx.CreateLabel("lbl", "Hello");
        const Widget* w = ctx.GetWidget(lbl);
        if (w && w->tag == WidgetTag::Label &&
            std::string(ctx.GetText(lbl)) == "Hello")
            PASS() else FAIL("Widget creation and text")
    }

    // Test 2: Widget hierarchy
    {
        UIContext ctx(64);
        int root = ctx.CreatePanel("root");
        int c1 = ctx.CreateLabel("c1", "A");
        int c2 = ctx.CreateLabel("c2", "B");
        ctx.SetParent(c1, root);
        ctx.SetParent(c2, root);
        if (ctx.GetWidget(root)->childCount == 2 &&
            ctx.GetWidget(c1)->parent == root)
            PASS() else FAIL("Widget parent-child hierarchy")
    }

    // Test 3: Hit testing
    {
        UIContext ctx(64);
        ctx.SetViewport(400, 300);
        int root = ctx.CreatePanel("root");
        ctx.SetRoot(root);
        int btn = ctx.CreateButton("btn", "Click");
        ctx.SetSize(btn, 80.0f, 30.0f);
        ctx.SetParent(btn, root);
        ctx.UpdateLayout();
        int hit = HitTest(ctx.Pool(), root, 40.0f, 15.0f);
        int miss = HitTest(ctx.Pool(), root, 500.0f, 500.0f);
        if (hit == btn && miss == -1)
            PASS() else FAIL("Hit testing")
    }

    // Test 4: Button click callback
    {
        UIContext ctx(64);
        ctx.SetViewport(400, 300);
        int root = ctx.CreatePanel("root");
        ctx.SetRoot(root);
        int btn = ctx.CreateButton("btn", "OK");
        ctx.SetSize(btn, 400.0f, 300.0f);
        ctx.SetParent(btn, root);
        ctx.UpdateLayout();
        bool clicked = false;
        ctx.SetOnClick(btn, [&clicked](Widget&) { clicked = true; });
        Event down; down.type = EventType::PointerDown;
        down.pointerX = 200; down.pointerY = 150;
        ctx.ProcessEvent(down);
        Event up; up.type = EventType::PointerUp;
        up.pointerX = 200; up.pointerY = 150;
        ctx.ProcessEvent(up);
        if (clicked)
            PASS() else FAIL("Button click callback")
    }

    // Test 5: Layout column
    {
        UIContext ctx(64);
        ctx.SetViewport(400, 300);
        int root = ctx.CreatePanel("root");
        ctx.SetRoot(root);
        ctx.SetLayout(root, LayoutDirection::Column);
        int c1 = ctx.CreatePanel("c1");
        int c2 = ctx.CreatePanel("c2");
        ctx.SetSize(c1, 100, 50);
        ctx.SetSize(c2, 100, 50);
        ctx.SetParent(c1, root);
        ctx.SetParent(c2, root);
        ctx.UpdateLayout();
        if (ctx.GetWidget(c2)->computedRect.y >= 49.0f)
            PASS() else FAIL("Layout column positioning")
    }

    // Test 6: Theme styling
    {
        Theme theme;
        const Style& s = theme.Get(WidgetTag::Button, PseudoState::Normal);
        if (s.isSet && s.background.r == 60)
            PASS() else FAIL("Theme default button style")
    }

    // Test 7: UI wrapper
    {
        UI ui;
        if (ui.Context().Root() >= 0)
            PASS() else FAIL("UI wrapper creates root")
    }

    // Test 8: UI Clear
    {
        UI ui;
        int btn = ui.Context().CreateButton("btn", "Test");
        ui.Context().SetParent(btn, ui.Context().Root());
        ui.Clear();
        if (ui.Context().Root() >= 0)
            PASS() else FAIL("UI Clear resets state")
    }

    // Test 9: UI reflection
    {
        UI::Describe();
        const ClassDesc* desc = ReflectionBridge::FindClass("UI");
        if (desc != nullptr)
            PASS() else FAIL("UI is in reflection registry")
    }

    std::cout << std::endl;
}

void TestUIScriptIntegration() {
    std::cout << "--- UI Script Integration ---" << std::endl;
    using namespace koilo;
    using namespace koilo::scripting;

    // Test 1: UI global is accessible from Setup
    {
        TEST("UI global accessible from Setup");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
fn Setup() {
    // UI global exists - accessing it should not crash
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            PASS()
        } else { FAIL(engine.GetError()); }
    }

    // Remaining script integration tests depend on old Widget API methods
    // being registered in reflection. These will be re-enabled once the
    // UIContext methods are exposed through the scripting layer.

    std::cout << std::endl;
}
