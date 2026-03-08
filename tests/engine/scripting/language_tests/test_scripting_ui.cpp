// SPDX-License-Identifier: GPL-3.0-or-later
// Input & UI Scripting Bindings
#include "helpers.hpp"

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
    std::cout << "--- Widget & UI ---" << std::endl;
    using namespace koilo;

    // Test 1: Widget creation and properties
    {
        Widget w;
        w.SetPosition(10.0f, 20.0f);
        w.SetSize(100.0f, 50.0f);
        w.SetText("Hello");
        if (w.x == 10.0f && w.y == 20.0f && w.width == 100.0f && w.GetText() == "Hello")
            PASS() else FAIL("Widget property setters/getters")
    }

    // Test 2: Widget hierarchy
    {
        Widget parent;
        Widget child1, child2;
        parent.AddChild(&child1);
        parent.AddChild(&child2);
        if (parent.GetChildCount() == 2 && child1.parent == &parent && child2.parent == &parent)
            PASS() else FAIL("Widget parent-child hierarchy")
    }

    // Test 3: Hit testing with absolute coords
    {
        Widget parent;
        parent.SetPosition(10.0f, 10.0f);
        parent.SetSize(100.0f, 100.0f);
        Widget child;
        child.SetPosition(20.0f, 20.0f);
        child.SetSize(30.0f, 30.0f);
        parent.AddChild(&child);
        // child absolute: (30, 30) to (60, 60)
        if (child.Contains(35.0f, 35.0f) && !child.Contains(5.0f, 5.0f))
            PASS() else FAIL("Widget hit testing with absolute coords")
    }

    // Test 4: Factory functions
    {
        Widget* label = CreateLabel("Test", 0, 0);
        Widget* panel = CreatePanel(0, 0, 50, 50);
        Widget* btn = CreateButton("OK", 0, 0, 80, 30);
        if (label->type == WidgetType::Label &&
            panel->type == WidgetType::Panel &&
            btn->type == WidgetType::Button && btn->focusable)
            PASS() else FAIL("Factory functions create correct types")
        delete label; delete panel; delete btn;
    }

    // Test 5: UI focus traversal
    {
        UI ui;
        Widget* btn1 = ui.CreateButton("A", 0, 0, 80, 30);
        Widget* btn2 = ui.CreateButton("B", 0, 40, 80, 30);
        Widget* btn3 = ui.CreateButton("C", 0, 80, 80, 30);
        btn1->SetOnActivate("onA");
        btn2->SetOnActivate("onB");
        btn3->SetOnActivate("onC");
        ui.RebuildFocusList();
        if (ui.GetFocusCount() == 3 && ui.GetFocusedWidget() == btn1)
            PASS() else FAIL("UI focus list builds correctly")
    }

    // Test 6: NextFocus / PrevFocus
    {
        UI ui;
        Widget* btn1 = ui.CreateButton("A", 0, 0, 80, 30);
        Widget* btn2 = ui.CreateButton("B", 0, 40, 80, 30);
        ui.RebuildFocusList();
        ui.NextFocus();
        if (ui.GetFocusedWidget() == btn2 && btn2->IsFocused() && !btn1->IsFocused())
            PASS() else FAIL("NextFocus moves to next widget")
    }

    // Test 7: Focus wraps around
    {
        UI ui;
        ui.CreateButton("A", 0, 0, 80, 30);
        ui.CreateButton("B", 0, 40, 80, 30);
        ui.RebuildFocusList();
        ui.NextFocus();
        ui.NextFocus(); // wraps to first
        Widget* focused = ui.GetFocusedWidget();
        if (focused && focused->GetText() == "A")
            PASS() else FAIL("Focus wraps around to first")
    }

    // Test 8: PrevFocus
    {
        UI ui;
        Widget* btn1 = ui.CreateButton("A", 0, 0, 80, 30);
        ui.CreateButton("B", 0, 40, 80, 30);
        ui.RebuildFocusList();
        ui.PrevFocus(); // wraps to last
        ui.PrevFocus(); // back to first
        if (ui.GetFocusedWidget() == btn1)
            PASS() else FAIL("PrevFocus wraps correctly")
    }

    // Test 9: ActivateFocus returns callback
    {
        UI ui;
        Widget* btn = ui.CreateButton("OK", 0, 0, 80, 30);
        btn->SetOnActivate("onOK");
        ui.RebuildFocusList();
        std::string result = ui.ActivateFocus();
        if (result == "onOK")
            PASS() else FAIL("ActivateFocus returns callback name")
    }

    // Test 10: Hit test
    {
        UI ui;
        Widget* btn = ui.CreateButton("Click", 10, 10, 80, 30);
        (void)btn;
        Widget* hit = ui.HitTest(15.0f, 15.0f);
        Widget* miss = ui.HitTest(200.0f, 200.0f);
        if (hit != nullptr && miss == nullptr)
            PASS() else FAIL("UI hit testing finds correct widget")
    }

    // Test 11: FocusAtPoint
    {
        UI ui;
        Widget* btn1 = ui.CreateButton("A", 0, 0, 80, 30);
        Widget* btn2 = ui.CreateButton("B", 0, 40, 80, 30);
        ui.RebuildFocusList();
        Widget* focused = ui.FocusAtPoint(5.0f, 45.0f); // inside btn2
        if (focused == btn2 && btn2->IsFocused() && !btn1->IsFocused())
            PASS() else FAIL("FocusAtPoint focuses correct widget")
    }

    // Test 12: Panel rendering
    {
        const int W = 16, H = 16;
        Color888 buf[W * H];
        for (int i = 0; i < W * H; i++) buf[i] = Color888(0, 0, 0);

        Widget panel;
        panel.type = WidgetType::Panel;
        panel.SetPosition(2, 2);
        panel.SetSize(4, 4);
        panel.SetBackgroundColor(255, 0, 0);
        panel.visible = true;
        panel.Render(buf, W, H);

        // Check center pixel
        Color888& c = buf[4 * W + 4]; // row=4, col=4 -> inside panel
        if (c.r == 255 && c.g == 0 && c.b == 0)
            PASS() else FAIL("Panel renders background color")
    }

    // Test 13: Label text rendering
    {
        const int W = 32, H = 16;
        Color888 buf[W * H];
        for (int i = 0; i < W * H; i++) buf[i] = Color888(0, 0, 0);

        Widget label;
        label.type = WidgetType::Label;
        label.SetPosition(0, 0);
        label.SetSize(32, 8);
        label.SetText("A");
        label.SetTextColor(255, 255, 255);
        label.visible = true;
        label.Render(buf, W, H);

        // At least one pixel should be white (text rendered)
        bool hasWhite = false;
        for (int i = 0; i < W * H; i++) {
            if (buf[i].r == 255 && buf[i].g == 255 && buf[i].b == 255) {
                hasWhite = true; break;
            }
        }
        if (hasWhite)
            PASS() else FAIL("Label renders text pixels")
    }

    // Test 14: UI RenderToBuffer with multiple widgets
    {
        const int W = 64, H = 32;
        Color888 buf[W * H];
        for (int i = 0; i < W * H; i++) buf[i] = Color888(0, 0, 0);

        UI ui;
        Widget* panel = ui.CreatePanel(0, 0, 64, 32);
        panel->SetBackgroundColor(50, 50, 50);
        ui.CreateLabel("HI", 4, 4);
        ui.RenderToBuffer(buf, W, H);

        // Panel should have colored the background
        if (buf[16 * W + 32].r == 50 && buf[16 * W + 32].g == 50)
            PASS() else FAIL("UI RenderToBuffer renders widgets")
    }

    // Test 15: Widget reflection
    {
        Widget::Describe();
        const ClassDesc* desc = ReflectionBridge::FindClass("Widget");
        if (desc != nullptr)
            PASS() else FAIL("Widget is in reflection registry")
    }

    // Test 16: UI reflection
    {
        UI::Describe();
        const ClassDesc* desc = ReflectionBridge::FindClass("UI");
        if (desc != nullptr)
            PASS() else FAIL("UI is in reflection registry")
    }

    // Test 17: Disabled widget not in focus list
    {
        UI ui;
        Widget* btn1 = ui.CreateButton("A", 0, 0, 80, 30);
        Widget* btn2 = ui.CreateButton("B", 0, 40, 80, 30);
        btn1->SetEnabled(false);
        ui.RebuildFocusList();
        if (ui.GetFocusCount() == 1 && ui.GetFocusedWidget() == btn2)
            PASS() else FAIL("Disabled widget excluded from focus list")
    }

    // Test 18: Clear removes all widgets
    {
        UI ui;
        ui.CreateButton("A", 0, 0, 80, 30);
        ui.CreateButton("B", 0, 40, 80, 30);
        ui.Clear();
        if (ui.GetWidgetCount() == 0 && ui.GetFocusCount() == 0)
            PASS() else FAIL("UI Clear removes all widgets")
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
var count = 0;

fn Setup() {
    count = ui.GetWidgetCount();
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            Value count = engine.GetGlobal("count");
            if (count.type == Value::Type::NUMBER && count.numberValue == 0.0)
                PASS() else FAIL("ui.GetWidgetCount should return 0 initially")
        } else { FAIL(engine.GetError()); }
    }

    // Test 2: Create button from script and check widget count
    {
        TEST("Create button via ui global");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
var count = 0;

fn Setup() {
    var btn = ui.CreateButton("Start", 10, 10, 80, 30);
    count = ui.GetWidgetCount();
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            Value count = engine.GetGlobal("count");
            if (count.type == Value::Type::NUMBER && count.numberValue == 1.0)
                PASS() else FAIL("ui should have 1 widget after CreateButton")
        } else { FAIL(engine.GetError()); }
    }

    // Test 3: UI overlay renders into PixelGroup
    {
        TEST("UI overlay renders to PixelGroup buffer");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
display.SetWidth(32);
display.SetHeight(16);
display.SetPixelWidth(32);
display.SetPixelHeight(16);

fn Setup() {
    var panel = ui.CreatePanel(0, 0, 32, 16);
    panel.SetBackgroundColor(100, 50, 25);
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            PixelGroup* pg = engine.GetPixelGroup();
            if (pg) {
                Color888* colors = pg->GetColors();
                int w = 32, h = 16;
                for (int i = 0; i < w * h; i++) colors[i] = Color888(0, 0, 0);
                engine.GetUI()->RenderToBuffer(colors, w, h);
                Color888& c = colors[8 * w + 16];
                if (c.r == 100 && c.g == 50 && c.b == 25)
                    PASS() else FAIL("UI panel should render to pixel buffer")
            } else { FAIL("No PixelGroup"); }
        } else { FAIL(engine.GetError()); }
    }

    // Test 4: Focus traversal via script
    {
        TEST("Focus traversal works from script");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
display.SetWidth(64);
display.SetHeight(64);

fn Setup() {
    ui.CreateButton("A", 0, 0, 60, 20);
    ui.CreateButton("B", 0, 25, 60, 20);
    ui.RebuildFocusList();
    ui.NextFocus();
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            Widget* focused = engine.GetUI()->GetFocusedWidget();
            if (focused && focused->GetText() == "B")
                PASS() else FAIL("NextFocus should move to second button")
        } else { FAIL(engine.GetError()); }
    }

    // Test 5: Activate returns callback name
    {
        TEST("ActivateFocus returns onActivate callback");
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
display.SetWidth(64);
display.SetHeight(64);

fn Setup() {
    var btn = ui.CreateButton("OK", 0, 0, 60, 20);
    btn.SetOnActivate("onOKPressed");
    ui.RebuildFocusList();
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            std::string cb = engine.GetUI()->ActivateFocus();
            if (cb == "onOKPressed")
                PASS() else FAIL("ActivateFocus should return 'onOKPressed'")
        } else { FAIL(engine.GetError()); }
    }

    std::cout << std::endl;
}



