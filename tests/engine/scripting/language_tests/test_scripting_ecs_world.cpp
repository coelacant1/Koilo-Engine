// SPDX-License-Identifier: GPL-3.0-or-later
// ECS, AI & World Serialization Bindings
#include "helpers.hpp"

void TestPhase17ECSIntegration() {
    std::cout << "--- ECS Integration ---" << std::endl;

    using namespace koilo;
    using koilo::scripting::KoiloScriptEngine;

    // 17.1: Entity creation and lifecycle
    {
        TEST("Entity creation returns valid handle")
        ScriptEntityManager sem;
        auto e1 = sem.Create();
        if (sem.IsValid(e1) && sem.GetCount() == 1) PASS()
        else FAIL("Entity not valid or count wrong")
    }
    {
        TEST("CreateNamed adds tag component")
        ScriptEntityManager sem;
        auto e = sem.CreateNamed("player");
        if (sem.IsValid(e) && sem.GetTag(e) == "player") PASS()
        else FAIL("Named entity tag=" + sem.GetTag(e))
    }
    {
        TEST("Destroy invalidates entity")
        ScriptEntityManager sem;
        auto e1 = sem.Create();
        auto e2 = sem.Create();
        sem.Destroy(e1);
        if (!sem.IsValid(e1) && sem.IsValid(e2) && sem.GetCount() == 1) PASS()
        else FAIL("Destroy did not work correctly")
    }

    // 17.2: Tag component
    {
        TEST("SetTag and GetTag")
        ScriptEntityManager sem;
        auto e = sem.Create();
        sem.SetTag(e, "enemy");
        if (sem.GetTag(e) == "enemy") PASS()
        else FAIL("tag=" + sem.GetTag(e))
    }
    {
        TEST("FindByTag returns correct entity")
        ScriptEntityManager sem;
        auto e = sem.CreateNamed("hero");
        auto found = sem.FindByTag("hero");
        if (found == e) PASS()
        else FAIL("FindByTag returned wrong entity")
    }
    {
        TEST("FindByTag returns null for missing tag")
        ScriptEntityManager sem;
        sem.CreateNamed("a");
        auto found = sem.FindByTag("nonexistent");
        if (found.IsNull()) PASS()
        else FAIL("Should have returned null entity")
    }

    // 17.3: Transform component (auto-added)
    {
        TEST("New entity has zero position by default")
        ScriptEntityManager sem;
        auto e = sem.Create();
        auto pos = sem.GetPosition(e);
        if (std::abs(pos.X) < 0.001f && std::abs(pos.Y) < 0.001f && std::abs(pos.Z) < 0.001f) PASS()
        else FAIL("Default pos not zero")
    }
    {
        TEST("SetPosition and GetPosition round-trip")
        ScriptEntityManager sem;
        auto e = sem.Create();
        sem.SetPosition(e, Vector3D(5.0f, 10.0f, -3.0f));
        auto pos = sem.GetPosition(e);
        if (std::abs(pos.X - 5.0f) < 0.001f && std::abs(pos.Y - 10.0f) < 0.001f &&
            std::abs(pos.Z - (-3.0f)) < 0.001f) PASS()
        else FAIL("Position mismatch")
    }
    {
        TEST("SetScale and GetScale round-trip")
        ScriptEntityManager sem;
        auto e = sem.Create();
        sem.SetScale(e, Vector3D(2.0f, 3.0f, 4.0f));
        auto s = sem.GetScale(e);
        if (std::abs(s.X - 2.0f) < 0.001f && std::abs(s.Y - 3.0f) < 0.001f) PASS()
        else FAIL("Scale mismatch")
    }

    // 17.4: Velocity component (on-demand)
    {
        TEST("Velocity defaults to zero before first set")
        ScriptEntityManager sem;
        auto e = sem.Create();
        auto vel = sem.GetVelocity(e);
        if (std::abs(vel.X) < 0.001f && std::abs(vel.Y) < 0.001f) PASS()
        else FAIL("Default velocity not zero")
    }
    {
        TEST("SetVelocity creates component and round-trips")
        ScriptEntityManager sem;
        auto e = sem.Create();
        sem.SetVelocity(e, Vector3D(1.0f, 0.0f, 0.0f));
        auto vel = sem.GetVelocity(e);
        if (std::abs(vel.X - 1.0f) < 0.001f) PASS()
        else FAIL("Velocity X=" + std::to_string(vel.X))
    }
    {
        TEST("SetAngularVelocity round-trips")
        ScriptEntityManager sem;
        auto e = sem.Create();
        sem.SetAngularVelocity(e, Vector3D(0.0f, 3.14f, 0.0f));
        auto av = sem.GetAngularVelocity(e);
        if (std::abs(av.Y - 3.14f) < 0.01f) PASS()
        else FAIL("Angular velocity Y=" + std::to_string(av.Y))
    }

    // 17.5: SceneNode bridge
    {
        TEST("AttachNode and GetNode")
        ScriptEntityManager sem;
        auto e = sem.CreateNamed("meshEntity");
        SceneNode node("testNode");
        sem.AttachNode(e, &node);
        if (sem.GetNode(e) == &node) PASS()
        else FAIL("GetNode returned wrong pointer")
    }
    {
        TEST("SyncTransforms pushes entity position to SceneNode")
        ScriptEntityManager sem;
        auto e = sem.Create();
        SceneNode node("syncNode");
        sem.AttachNode(e, &node);
        sem.SetPosition(e, Vector3D(7.0f, 8.0f, 9.0f));
        sem.SyncTransforms();
        auto np = node.GetPosition();
        if (std::abs(np.X - 7.0f) < 0.001f && std::abs(np.Y - 8.0f) < 0.001f) PASS()
        else FAIL("Node position not synced: " + std::to_string(np.X) + "," + std::to_string(np.Y))
    }
    {
        TEST("Destroy detaches SceneNode")
        ScriptEntityManager sem;
        auto e = sem.Create();
        SceneNode node("detachNode");
        sem.AttachNode(e, &node);
        sem.Destroy(e);
        if (sem.GetNode(e) == nullptr) PASS()
        else FAIL("Node should be nullptr after destroy")
    }

    // 17.6: Multiple entities
    {
        TEST("Multiple entities with same tag, FindByTag returns first")
        ScriptEntityManager sem;
        auto e1 = sem.CreateNamed("npc");
        sem.CreateNamed("npc");
        auto found = sem.FindByTag("npc");
        if (found == e1 && sem.GetCount() == 2) PASS()
        else FAIL("FindByTag didn't return first, count=" + std::to_string(sem.GetCount()))
    }

    // 17.7: Invalid entity safety
    {
        TEST("Operations on null entity don't crash")
        ScriptEntityManager sem;
        Entity invalid;
        sem.SetTag(invalid, "test");
        sem.SetPosition(invalid, Vector3D(1, 2, 3));
        sem.Destroy(invalid);
        if (!sem.IsValid(invalid) && sem.GetTag(invalid) == "" &&
            std::abs(sem.GetPosition(invalid).X) < 0.001f) PASS()
        else FAIL("Null entity operations returned unexpected values")
    }

    // 17.8: Script engine integration
    {
        TEST("entities global accessible from KoiloScript")
        StringFileReader reader;
        reader.SetContent("fn Setup() { var e = entities.CreateNamed(\"hero\"); }\nfn Update(dt) { }");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        if (engine.BuildScene() && engine.GetEntities()->GetCount() == 1) PASS()
        else FAIL("Script entity creation failed, count=" + std::to_string(engine.GetEntities()->GetCount()))
    }

    // 17.9: Underlying EntityManager access
    {
        TEST("GetManager provides direct C++ ECS access")
        ScriptEntityManager sem;
        auto& mgr = sem.GetManager();
        auto e = mgr.CreateEntity();
        if (mgr.IsEntityValid(e) && mgr.GetEntityCount() == 1) PASS()
        else FAIL("Direct EntityManager access failed")
    }

    // 17.10: Script-side ECS via double handles
    {
        TEST("Script: entities.Create + GetCount")
        StringFileReader reader;
        reader.SetContent(R"(
var id = 0;
var count = 0;
fn Setup() {
    id = entities.Create();
    count = entities.GetCount();
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        auto id = engine.GetScriptVariable("id");
        auto count = engine.GetScriptVariable("count");
        if (id.type == Value::Type::NUMBER && id.numberValue > 0.0 &&
            count.type == Value::Type::NUMBER && count.numberValue == 1.0) PASS()
        else FAIL("id=" + std::to_string(id.numberValue) + " count=" + std::to_string(count.numberValue))
    }
    {
        TEST("Script: entities.CreateNamed + FindByTag")
        StringFileReader reader;
        reader.SetContent(R"(
var id = 0;
var found = 0;
fn Setup() {
    id = entities.CreateNamed("player");
    found = entities.FindByTag("player");
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        auto id = engine.GetScriptVariable("id");
        auto found = engine.GetScriptVariable("found");
        if (id.type == Value::Type::NUMBER && found.type == Value::Type::NUMBER &&
            id.numberValue == found.numberValue && id.numberValue > 0.0) PASS()
        else FAIL("id=" + std::to_string(id.numberValue) + " found=" + std::to_string(found.numberValue))
    }
    {
        TEST("Script: entities.SetTag + FindByTag")
        StringFileReader reader;
        reader.SetContent(R"(
var id = 0;
var found = 0;
fn Setup() {
    id = entities.Create();
    entities.SetTag(id, "hero");
    found = entities.FindByTag("hero");
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        auto id = engine.GetScriptVariable("id");
        auto found = engine.GetScriptVariable("found");
        if (id.numberValue == found.numberValue && found.numberValue > 0.0) PASS()
        else FAIL("id=" + std::to_string(id.numberValue) + " found=" + std::to_string(found.numberValue))
    }
    {
        TEST("Script: entities.GetTag returns name")
        StringFileReader reader;
        reader.SetContent(R"(
var tag = "";
fn Setup() {
    var id = entities.CreateNamed("npc");
    tag = entities.GetTag(id);
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        auto tag = engine.GetScriptVariable("tag");
        if (tag.type == Value::Type::STRING && tag.stringValue == "npc") PASS()
        else FAIL("tag=" + tag.stringValue)
    }
    {
        TEST("Script: entities.Destroy + IsValid")
        StringFileReader reader;
        reader.SetContent(R"(
var validBefore = false;
var validAfter = true;
fn Setup() {
    var id = entities.Create();
    validBefore = entities.IsValid(id);
    entities.Destroy(id);
    validAfter = entities.IsValid(id);
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        auto before = engine.GetScriptVariable("validBefore");
        auto after = engine.GetScriptVariable("validAfter");
        if (before.IsTruthy() && !after.IsTruthy()) PASS()
        else FAIL("before=" + std::to_string(before.IsTruthy()) + " after=" + std::to_string(after.IsTruthy()))
    }
    {
        TEST("Script: entities.SetPosition + GetPosition")
        StringFileReader reader;
        reader.SetContent(R"(
var px = 0;
var py = 0;
var pz = 0;
fn Setup() {
    var id = entities.Create();
    entities.SetPosition(id, Vector3D(10, 20, 30));
    var pos = entities.GetPosition(id);
    px = pos.X;
    py = pos.Y;
    pz = pos.Z;
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        auto px = engine.GetScriptVariable("px");
        auto py = engine.GetScriptVariable("py");
        auto pz = engine.GetScriptVariable("pz");
        if (px.type == Value::Type::NUMBER && std::abs(px.numberValue - 10.0) < 0.1 &&
            py.type == Value::Type::NUMBER && std::abs(py.numberValue - 20.0) < 0.1 &&
            pz.type == Value::Type::NUMBER && std::abs(pz.numberValue - 30.0) < 0.1) PASS()
        else FAIL("pos=(" + std::to_string(px.numberValue) + "," + std::to_string(py.numberValue) + "," + std::to_string(pz.numberValue) + ")")
    }
    {
        TEST("Script: entities.SetVelocity + GetVelocity")
        StringFileReader reader;
        reader.SetContent(R"(
var vx = 0;
var vy = 0;
var vz = 0;
fn Setup() {
    var id = entities.Create();
    entities.SetVelocity(id, Vector3D(1, 2, 3));
    var vel = entities.GetVelocity(id);
    vx = vel.X;
    vy = vel.Y;
    vz = vel.Z;
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        auto vx = engine.GetScriptVariable("vx");
        auto vy = engine.GetScriptVariable("vy");
        auto vz = engine.GetScriptVariable("vz");
        if (vx.type == Value::Type::NUMBER && std::abs(vx.numberValue - 1.0) < 0.1 &&
            vy.type == Value::Type::NUMBER && std::abs(vy.numberValue - 2.0) < 0.1 &&
            vz.type == Value::Type::NUMBER && std::abs(vz.numberValue - 3.0) < 0.1) PASS()
        else FAIL("vel=(" + std::to_string(vx.numberValue) + "," + std::to_string(vy.numberValue) + "," + std::to_string(vz.numberValue) + ")")
    }
    {
        TEST("Script: Multiple entities + FindByTag each")
        StringFileReader reader;
        reader.SetContent(R"(
var a = 0;
var b = 0;
var fA = 0;
var fB = 0;
var count = 0;
fn Setup() {
    a = entities.Create();
    b = entities.Create();
    entities.SetTag(a, "alpha");
    entities.SetTag(b, "beta");
    fA = entities.FindByTag("alpha");
    fB = entities.FindByTag("beta");
    count = entities.GetCount();
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        auto a = engine.GetScriptVariable("a");
        auto b = engine.GetScriptVariable("b");
        auto fA = engine.GetScriptVariable("fA");
        auto fB = engine.GetScriptVariable("fB");
        auto count = engine.GetScriptVariable("count");
        if (a.numberValue == fA.numberValue && b.numberValue == fB.numberValue &&
            count.numberValue == 2.0) PASS()
        else FAIL("a=" + std::to_string(a.numberValue) + " fA=" + std::to_string(fA.numberValue) +
                  " b=" + std::to_string(b.numberValue) + " fB=" + std::to_string(fB.numberValue))
    }
    {
        TEST("Script: FindByTag not found returns -1")
        StringFileReader reader;
        reader.SetContent(R"(
var found = 0;
fn Setup() {
    found = entities.FindByTag("nonexistent");
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        auto found = engine.GetScriptVariable("found");
        if (found.type == Value::Type::NUMBER && found.numberValue == -1.0) PASS()
        else FAIL("found=" + std::to_string(found.numberValue))
    }
    {
        TEST("Script: entities.SetScale + GetScale")
        StringFileReader reader;
        reader.SetContent(R"(
var sx = 0;
var sy = 0;
var sz = 0;
fn Setup() {
    var id = entities.Create();
    entities.SetScale(id, Vector3D(2, 3, 4));
    var s = entities.GetScale(id);
    sx = s.X;
    sy = s.Y;
    sz = s.Z;
}
fn Update(dt) {}
)");
        KoiloScriptEngine engine(&reader);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        auto sx = engine.GetScriptVariable("sx");
        auto sy = engine.GetScriptVariable("sy");
        auto sz = engine.GetScriptVariable("sz");
        if (sx.type == Value::Type::NUMBER && std::abs(sx.numberValue - 2.0) < 0.1 &&
            sy.type == Value::Type::NUMBER && std::abs(sy.numberValue - 3.0) < 0.1 &&
            sz.type == Value::Type::NUMBER && std::abs(sz.numberValue - 4.0) < 0.1) PASS()
        else FAIL("scale=(" + std::to_string(sx.numberValue) + "," + std::to_string(sy.numberValue) + "," + std::to_string(sz.numberValue) + ")")
    }
}

// ============================================================================
// AI System Tests
// ============================================================================

void TestPhase18AISystem() {
    std::cout << "--- AI System ---" << std::endl;

    // -- StateMachine tests --

    TEST("StateMachine: add states and set initial")
    {
        koilo::StateMachine sm;
        sm.AddState("idle");
        sm.AddState("patrol");
        sm.SetInitialState("idle");
        sm.Start();
        if (sm.GetCurrentStateName() == "idle") PASS()
        else FAIL("Expected idle, got: " + sm.GetCurrentStateName())
    }

    TEST("StateMachine: manual transition")
    {
        koilo::StateMachine sm;
        sm.AddState("idle");
        sm.AddState("chase");
        sm.SetInitialState("idle");
        sm.Start();
        sm.TransitionTo("chase");
        if (sm.GetCurrentStateName() == "chase") PASS()
        else FAIL("Expected chase, got: " + sm.GetCurrentStateName())
    }

    TEST("StateMachine: Update ticks current state")
    {
        koilo::StateMachine sm;
        sm.AddState("idle");
        sm.SetInitialState("idle");
        sm.Start();
        koilo::TimeManager::GetInstance().Tick(0.016f); sm.Update();
        if (sm.GetCurrentStateName() == "idle") PASS()
        else FAIL("Machine state corrupted after Update")
    }

    TEST("StateMachine: stopped machine safe on update")
    {
        koilo::StateMachine sm;
        sm.AddState("idle");
        sm.SetInitialState("idle");
        sm.Start();
        sm.Stop();
        koilo::TimeManager::GetInstance().Tick(0.016f); sm.Update();
        PASS()
    }

    // -- BehaviorTree tests --

    TEST("BehaviorTree: create and set root")
    {
        koilo::BehaviorTree bt("test_tree");
        auto action = std::make_shared<koilo::ActionNode>(
            []() { return koilo::NodeStatus::Success; });
        bt.SetRoot(action);
        if (bt.GetRoot() != nullptr && bt.GetName() == "test_tree") PASS()
        else FAIL("BT creation/root failed")
    }

    TEST("BehaviorTree: Tick runs root node")
    {
        koilo::BehaviorTree bt("ticker");
        bool ran = false;
        auto action = std::make_shared<koilo::ActionNode>(
            [&ran]() { ran = true; return koilo::NodeStatus::Success; });
        bt.SetRoot(action);
        bt.Start();
        bt.Tick();
        if (ran) PASS()
        else FAIL("Tick did not execute root action")
    }

    TEST("BehaviorTree: stopped tree does not tick")
    {
        koilo::BehaviorTree bt;
        bool ran = false;
        auto action = std::make_shared<koilo::ActionNode>(
            [&ran]() { ran = true; return koilo::NodeStatus::Success; });
        bt.SetRoot(action);
        bt.Tick();
        if (!ran) PASS()
        else FAIL("Stopped tree should not tick")
    }

    TEST("BehaviorTree: Sequence succeeds when all children succeed")
    {
        auto seq = std::make_shared<koilo::SequenceNode>();
        auto a1 = std::make_shared<koilo::ActionNode>(
            []() { return koilo::NodeStatus::Success; });
        auto a2 = std::make_shared<koilo::ActionNode>(
            []() { return koilo::NodeStatus::Success; });
        seq->AddChild(a1);
        seq->AddChild(a2);
        auto result = seq->Execute();
        if (result == koilo::NodeStatus::Success) PASS()
        else FAIL("Sequence should succeed")
    }

    TEST("BehaviorTree: Sequence fails on first failure")
    {
        auto seq = std::make_shared<koilo::SequenceNode>();
        int callCount = 0;
        auto fail = std::make_shared<koilo::ActionNode>(
            [&callCount]() { callCount++; return koilo::NodeStatus::Failure; });
        auto ok = std::make_shared<koilo::ActionNode>(
            [&callCount]() { callCount++; return koilo::NodeStatus::Success; });
        seq->AddChild(fail);
        seq->AddChild(ok);
        auto result = seq->Execute();
        if (result == koilo::NodeStatus::Failure && callCount == 1) PASS()
        else FAIL("Sequence should fail after first child, callCount=" + std::to_string(callCount))
    }

    TEST("BehaviorTree: Selector succeeds on first success")
    {
        auto sel = std::make_shared<koilo::SelectorNode>();
        auto fail = std::make_shared<koilo::ActionNode>(
            []() { return koilo::NodeStatus::Failure; });
        auto ok = std::make_shared<koilo::ActionNode>(
            []() { return koilo::NodeStatus::Success; });
        sel->AddChild(fail);
        sel->AddChild(ok);
        auto result = sel->Execute();
        if (result == koilo::NodeStatus::Success) PASS()
        else FAIL("Selector should succeed on second child")
    }

    TEST("BehaviorTree: Inverter flips result")
    {
        auto inv = std::make_shared<koilo::InverterNode>();
        auto ok = std::make_shared<koilo::ActionNode>(
            []() { return koilo::NodeStatus::Success; });
        inv->AddChild(ok);
        auto result = inv->Execute();
        if (result == koilo::NodeStatus::Failure) PASS()
        else FAIL("Inverter should flip Success to Failure")
    }

    TEST("BehaviorTree: Reset clears tree")
    {
        koilo::BehaviorTree bt;
        auto action = std::make_shared<koilo::ActionNode>(
            []() { return koilo::NodeStatus::Success; });
        bt.SetRoot(action);
        bt.Start();
        bt.Tick();
        bt.Reset();
        PASS()
    }

    // -- A* Pathfinding tests --

    TEST("PathfinderGrid: find straight path")
    {
        koilo::PathfinderGrid grid(5, 5, false);
        std::vector<koilo::GridNode> path;
        bool found = grid.FindPath(0, 0, 4, 0, path);
        if (found && path.size() == 5 && path.front().x == 0 && path.back().x == 4) PASS()
        else FAIL("Straight path failed, found=" + std::to_string(found) + " len=" + std::to_string(path.size()))
    }

    TEST("PathfinderGrid: path around obstacle")
    {
        koilo::PathfinderGrid grid(5, 5, false);
        grid.SetWalkable(2, 0, false);
        grid.SetWalkable(2, 1, false);
        grid.SetWalkable(2, 2, false);
        grid.SetWalkable(2, 3, false);
        std::vector<koilo::GridNode> path;
        bool found = grid.FindPath(0, 0, 4, 0, path);
        if (found && path.size() > 5) PASS()
        else FAIL("Should find path around wall, found=" + std::to_string(found) + " len=" + std::to_string(path.size()))
    }

    TEST("PathfinderGrid: no path when fully blocked")
    {
        koilo::PathfinderGrid grid(3, 3, false);
        grid.SetWalkable(1, 0, false);
        grid.SetWalkable(1, 1, false);
        grid.SetWalkable(1, 2, false);
        std::vector<koilo::GridNode> path;
        bool found = grid.FindPath(0, 0, 2, 0, path);
        if (!found && path.empty()) PASS()
        else FAIL("Should not find path through solid wall")
    }

    TEST("PathfinderGrid: diagonal path shorter")
    {
        koilo::PathfinderGrid gridNoDiag(5, 5, false);
        koilo::PathfinderGrid gridDiag(5, 5, true);
        std::vector<koilo::GridNode> pathNoDiag, pathDiag;
        gridNoDiag.FindPath(0, 0, 4, 4, pathNoDiag);
        gridDiag.FindPath(0, 0, 4, 4, pathDiag);
        if (pathDiag.size() < pathNoDiag.size()) PASS()
        else FAIL("Diagonal path should be shorter")
    }

    TEST("PathfinderGrid: heuristic functions")
    {
        koilo::GridNode a(0, 0), b(3, 4);
        float manhattan = koilo::PathfinderGrid::ManhattanDistance(a, b);
        float euclidean = koilo::PathfinderGrid::EuclideanDistance(a, b);
        float diagonal = koilo::PathfinderGrid::DiagonalDistance(a, b);
        if (manhattan == 7.0f && euclidean == 5.0f && diagonal == 4.0f) PASS()
        else FAIL("Heuristics wrong")
    }

    // -- ScriptAIManager tests --

    TEST("ScriptAIManager: create and use state machine")
    {
        koilo::ScriptAIManager ai;
        ai.CreateStateMachine("fsm");
        ai.AddState("fsm", "idle");
        ai.AddState("fsm", "walk");
        ai.SetInitialState("fsm", "idle");
        ai.StartMachine("fsm");
        if (ai.GetCurrentState("fsm") == "idle") PASS()
        else FAIL("Expected idle")
    }

    TEST("ScriptAIManager: transition and update")
    {
        koilo::ScriptAIManager ai;
        ai.CreateStateMachine("fsm");
        ai.AddState("fsm", "a");
        ai.AddState("fsm", "b");
        ai.SetInitialState("fsm", "a");
        ai.StartMachine("fsm");
        ai.TransitionTo("fsm", "b");
        koilo::TimeManager::GetInstance().Tick(0.016f); ai.Update();
        if (ai.GetCurrentState("fsm") == "b") PASS()
        else FAIL("Expected b")
    }

    TEST("ScriptAIManager: create grid and find path")
    {
        koilo::ScriptAIManager ai;
        ai.CreateGrid("map", 10, 10, false);
        bool found = ai.FindPath("map", 0, 0, 9, 0);
        if (found && ai.GetPathLength() == 10 && ai.GetPathX(0) == 0 && ai.GetPathX(9) == 9) PASS()
        else FAIL("Grid pathfinding failed, len=" + std::to_string(ai.GetPathLength()))
    }

    TEST("ScriptAIManager: grid with obstacle")
    {
        koilo::ScriptAIManager ai;
        ai.CreateGrid("map", 5, 5, false);
        ai.SetWalkable("map", 2, 0, false);
        ai.SetWalkable("map", 2, 1, false);
        ai.SetWalkable("map", 2, 2, false);
        ai.SetWalkable("map", 2, 3, false);
        bool found = ai.FindPath("map", 0, 0, 4, 0);
        if (found && ai.GetPathLength() > 5) PASS()
        else FAIL("Should route around obstacle")
    }

    TEST("ScriptAIManager: nonexistent machine returns empty")
    {
        koilo::ScriptAIManager ai;
        std::string state = ai.GetCurrentState("nope");
        if (state.empty()) PASS()
        else FAIL("Should return empty for nonexistent machine")
    }

    TEST("ScriptAIManager: GetGrid direct access")
    {
        koilo::ScriptAIManager ai;
        ai.CreateGrid("level", 3, 3, false);
        koilo::PathfinderGrid* g = ai.GetGrid("level");
        if (g != nullptr && g->IsInBounds(2, 2) && !g->IsInBounds(3, 3)) PASS()
        else FAIL("Direct grid access failed")
    }

    // -- Script integration --

    TEST("Script: ai global is accessible")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script =
            "fn Setup() {\n"
            "  ai.CreateStateMachine(\"bot\");\n"
            "  ai.AddState(\"bot\", \"idle\");\n"
            "  ai.SetInitialState(\"bot\", \"idle\");\n"
            "  ai.StartMachine(\"bot\");\n"
            "}\n"
            "fn Update(dt) {\n"
            "  ai.Update(dt);\n"
            "}\n";
        reader.SetContent(script);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
        auto* mgr = engine.GetAI();
        auto* sm = mgr->GetStateMachine("bot");
        if (sm && sm->GetCurrentStateName() == "idle") PASS()
        else FAIL("Script ai global didn't create state machine")
    }

    TEST("Script: ai pathfinding from script")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script =
            "fn Setup() {\n"
            "  ai.CreateGrid(\"map\", 5, 5, false);\n"
            "}\n"
            "fn Update(dt) {\n"
            "}\n";
        reader.SetContent(script);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        auto* mgr = engine.GetAI();
        bool found = mgr->FindPath("map", 0, 0, 4, 4);
        if (found && mgr->GetPathLength() == 9) PASS()
        else FAIL("Script-created grid pathfinding failed, len=" + std::to_string(mgr->GetPathLength()))
    }
}

// ============================================================================
// Audio System Tests
// ============================================================================

void TestPhase20WorldSerialization() {
    std::cout << "--- World & Serialization ---" << std::endl;

    using namespace koilo;
    using namespace koilo::scripting;

    // --- ReflectionSerializer: Vector3D ---
    TEST("ReflectionSerializer: Vector3D round-trip")
    {
        Vector3D v(1.5f, -2.0f, 3.75f);
        Vector3D::Describe();
        const ClassDesc* desc = ReflectionBridge::FindClass("Vector3D");
        std::string json = ReflectionSerializer::SerializeToJSON(&v, desc);
        Vector3D v2;
        ReflectionSerializer::DeserializeFromJSON(&v2, desc, json);
        if (std::abs(v2.X - 1.5f) < 0.01f && std::abs(v2.Y - (-2.0f)) < 0.01f && std::abs(v2.Z - 3.75f) < 0.01f) PASS()
        else FAIL("Vector3D round-trip mismatch")
    }

    // --- ReflectionSerializer: Quaternion ---
    TEST("ReflectionSerializer: Quaternion round-trip")
    {
        Quaternion q;
        q.W = 1.0f; q.X = 0.0f; q.Y = 0.5f; q.Z = -0.5f;
        Quaternion::Describe();
        const ClassDesc* desc = ReflectionBridge::FindClass("Quaternion");
        std::string json = ReflectionSerializer::SerializeToJSON(&q, desc);
        Quaternion q2;
        ReflectionSerializer::DeserializeFromJSON(&q2, desc, json);
        if (std::abs(q2.W - 1.0f) < 0.01f && std::abs(q2.Y - 0.5f) < 0.01f) PASS()
        else FAIL("Quaternion round-trip mismatch")
    }

    // --- ReflectionSerializer: TagComponent (string field) ---
    TEST("ReflectionSerializer: TagComponent round-trip")
    {
        TagComponent tc("Player");
        TagComponent::Describe();
        const ClassDesc* desc = ReflectionBridge::FindClass("TagComponent");
        std::string json = ReflectionSerializer::SerializeToJSON(&tc, desc);
        TagComponent tc2;
        ReflectionSerializer::DeserializeFromJSON(&tc2, desc, json);
        if (tc2.tag == "Player") PASS()
        else FAIL("TagComponent round-trip mismatch: " + tc2.tag)
    }

    // --- ReflectionSerializer: VelocityComponent (nested Vector3D) ---
    TEST("ReflectionSerializer: VelocityComponent round-trip")
    {
        VelocityComponent vc(Vector3D(10, 0, -5), Vector3D(0, 1.5f, 0));
        VelocityComponent::Describe();
        const ClassDesc* desc = ReflectionBridge::FindClass("VelocityComponent");
        std::string json = ReflectionSerializer::SerializeToJSON(&vc, desc);
        VelocityComponent vc2;
        ReflectionSerializer::DeserializeFromJSON(&vc2, desc, json);
        if (std::abs(vc2.linear.X - 10.0f) < 0.01f && std::abs(vc2.angular.Y - 1.5f) < 0.01f) PASS()
        else FAIL("VelocityComponent round-trip mismatch")
    }

    // --- ReflectionSerializer: TransformComponent (nested Transform with nested Vector3D/Quaternion) ---
    TEST("ReflectionSerializer: TransformComponent round-trip")
    {
        TransformComponent tc(Vector3D(5, 10, 15));
        TransformComponent::Describe();
        Transform::Describe();
        const ClassDesc* desc = ReflectionBridge::FindClass("TransformComponent");
        std::string json = ReflectionSerializer::SerializeToJSON(&tc, desc);
        TransformComponent tc2;
        ReflectionSerializer::DeserializeFromJSON(&tc2, desc, json);
        Vector3D pos = tc2.GetPosition();
        if (std::abs(pos.X - 5.0f) < 0.01f && std::abs(pos.Y - 10.0f) < 0.01f && std::abs(pos.Z - 15.0f) < 0.01f) PASS()
        else FAIL("TransformComponent round-trip mismatch")
    }

    // --- LevelSerializer: component registry initialized ---
    TEST("LevelSerializer: component registry has entries")
    {
        LevelSerializer serializer;
        auto& reg = LevelSerializer::GetComponentRegistry();
        if (reg.size() >= 3) PASS()
        else FAIL("Expected at least 3 component types in registry")
    }

    // --- LevelSerializer: entity serialization with components ---
    TEST("LevelSerializer: entity serialize/deserialize with components")
    {
        EntityManager em;
        LevelSerializer serializer(&em);
        Entity e = em.CreateEntity();
        em.AddComponent<TagComponent>(e, TagComponent("Hero"));
        em.AddComponent<VelocityComponent>(e, VelocityComponent(Vector3D(1, 2, 3)));

        SerializedEntity se = serializer.SerializeEntity(e);
        if (se.componentTypes.size() != 2) { FAIL("Expected 2 component types, got " + std::to_string(se.componentTypes.size())) }
        else {
            Entity e2 = serializer.DeserializeEntity(se);
            auto* tag = em.GetComponent<TagComponent>(e2);
            auto* vel = em.GetComponent<VelocityComponent>(e2);
            if (tag && vel && tag->tag == "Hero" && std::abs(vel->linear.X - 1.0f) < 0.01f) PASS()
            else FAIL("Deserialized entity missing components or wrong values")
        }
    }

    // --- LevelSerializer: JSON string round-trip ---
    TEST("LevelSerializer: level JSON string round-trip")
    {
        EntityManager em;
        LevelSerializer serializer(&em);

        auto level = std::make_shared<Level>("TestLevel");
        level->SetEntityManager(&em);
        level->SetStreamable(true);
        level->SetStreamingBounds(Vector3D(10, 20, 30), 500.0f);

        Entity e1 = em.CreateEntity();
        em.AddComponent<TagComponent>(e1, TagComponent("NPC"));
        level->AddEntity(e1);

        std::string json = serializer.SerializeLevelToString(level);
        if (json.empty()) { FAIL("SerializeLevelToString returned empty") }
        else {
            auto loaded = serializer.DeserializeLevelFromString(json);
            if (loaded && loaded->GetName() == "TestLevel" && loaded->IsStreamable()
                && std::abs(loaded->GetStreamingRadius() - 500.0f) < 0.01f
                && loaded->GetEntities().size() == 1) PASS()
            else FAIL("Level JSON round-trip mismatch")
        }
    }

    // --- LevelSerializer: JSON file round-trip ---
    TEST("LevelSerializer: level JSON file round-trip")
    {
        EntityManager em;
        LevelSerializer serializer(&em, SerializationFormat::JSON);

        auto level = std::make_shared<Level>("FileLevel");
        level->SetEntityManager(&em);
        Entity e = em.CreateEntity();
        em.AddComponent<TagComponent>(e, TagComponent("Boss"));
        em.AddComponent<TransformComponent>(e, TransformComponent(Vector3D(100, 200, 300)));
        level->AddEntity(e);

        std::string path = "/tmp/koilo_test_level.json";
        bool saved = serializer.SerializeLevelToFile(level, path);
        if (!saved) { FAIL("Failed to save JSON file") }
        else {
            auto loaded = serializer.DeserializeLevelFromFile(path);
            if (loaded && loaded->GetName() == "FileLevel" && loaded->GetEntities().size() == 1) {
                auto& ents = loaded->GetEntities();
                auto* tag = em.GetComponent<TagComponent>(ents[0]);
                auto* tf = em.GetComponent<TransformComponent>(ents[0]);
                if (tag && tag->tag == "Boss" && tf && std::abs(tf->GetPosition().X - 100.0f) < 0.01f) PASS()
                else FAIL("File round-trip component mismatch")
            } else FAIL("File round-trip level mismatch")
        }
        std::remove(path.c_str());
    }

    // --- LevelSerializer: binary file round-trip ---
    TEST("LevelSerializer: level binary file round-trip")
    {
        EntityManager em;
        LevelSerializer serializer(&em, SerializationFormat::Binary);

        auto level = std::make_shared<Level>("BinaryLevel");
        level->SetEntityManager(&em);
        level->SetStreamable(true);
        level->SetStreamingBounds(Vector3D(1, 2, 3), 100.0f);
        Entity e = em.CreateEntity();
        em.AddComponent<TagComponent>(e, TagComponent("Guard"));
        level->AddEntity(e);

        std::string path = "/tmp/koilo_test_level.bin";
        bool saved = serializer.SerializeLevelToFile(level, path);
        if (!saved) { FAIL("Failed to save binary file") }
        else {
            auto loaded = serializer.DeserializeLevelFromFile(path);
            if (loaded && loaded->GetName() == "BinaryLevel" && loaded->IsStreamable()
                && std::abs(loaded->GetStreamingRadius() - 100.0f) < 0.01f
                && loaded->GetEntities().size() == 1) {
                auto* tag = em.GetComponent<TagComponent>(loaded->GetEntities()[0]);
                if (tag && tag->tag == "Guard") PASS()
                else FAIL("Binary round-trip component mismatch")
            } else FAIL("Binary round-trip level mismatch")
        }
        std::remove(path.c_str());
    }

    // --- WorldManager: create and activate level ---
    TEST("WorldManager: create and activate level")
    {
        WorldManager wm;
        wm.CreateLevel("Level1");
        wm.CreateLevel("Level2");
        if (wm.GetLevelCount() != 2) { FAIL("Expected 2 levels") }
        else {
            wm.SetActiveLevel("Level1");
            if (wm.GetActiveLevelName() == "Level1") PASS()
            else FAIL("Active level name mismatch")
        }
    }

    // --- WorldManager: streaming check ---
    TEST("WorldManager: streaming loads/unloads levels")
    {
        WorldManager wm;
        auto lvl = wm.CreateLevel("StreamLevel");
        lvl->SetStreamable(true);
        lvl->SetStreamingBounds(Vector3D(0, 0, 0), 10.0f);
        wm.SetStreamingEnabled(true);

        wm.SetStreamingViewerPosition(Vector3D(0, 0, 0));
        wm.CheckStreaming();
        if (lvl->GetState() != LevelState::Loaded) { FAIL("Level should be loaded when in range") }
        else {
            wm.SetStreamingViewerPosition(Vector3D(100, 100, 100));
            wm.CheckStreaming();
            if (lvl->GetState() == LevelState::Unloaded) PASS()
            else FAIL("Level should be unloaded when out of range")
        }
    }

    // --- WorldManager: save and load from file ---
    TEST("WorldManager: save and load level from file")
    {
        EntityManager em;
        WorldManager wm(&em);
        wm.CreateLevel("SaveMe");
        auto lvl = wm.GetLevel("SaveMe");
        Entity e = em.CreateEntity();
        em.AddComponent<TagComponent>(e, TagComponent("SavedEntity"));
        lvl->AddEntity(e);

        std::string path = "/tmp/koilo_test_wm_level.json";
        bool saved = wm.SaveLevelToFile("SaveMe", path);
        if (!saved) { FAIL("WorldManager save failed") }
        else {
            WorldManager wm2(&em);
            auto loaded = wm2.LoadLevelFromFile(path);
            if (loaded && loaded->GetName() == "SaveMe") PASS()
            else FAIL("WorldManager load failed")
        }
        std::remove(path.c_str());
    }

    // --- ScriptWorldManager: basic operations ---
    TEST("ScriptWorldManager: create and activate level")
    {
        ScriptWorldManager swm;
        swm.CreateLevel("TestLvl");
        if (swm.GetLevelCount() != 1) { FAIL("Expected 1 level") }
        else {
            swm.SetActiveLevel("TestLvl");
            if (swm.GetActiveLevelName() == "TestLvl") PASS()
            else FAIL("Active level mismatch")
        }
    }

    TEST("ScriptWorldManager: streaming controls")
    {
        ScriptWorldManager swm;
        swm.SetStreamingEnabled(true);
        if (swm.IsStreamingEnabled()) PASS()
        else FAIL("Streaming should be enabled")
    }

    // --- Script integration: world global accessible ---
    TEST("Script: world global is accessible")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent("fn Setup() {\n  var count = world.GetLevelCount();\n}\n");
        bool ok = engine.LoadScript("test.ks");
        if (ok) PASS()
        else FAIL("Script failed to access world global")
    }

    TEST("Script: world.CreateLevel from script")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent("fn Setup() {\n  world.CreateLevel(\"ScriptLevel\");\n}\n");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        if (ok && engine.GetWorld()->GetLevelCount() == 1) PASS()
        else FAIL("Script CreateLevel failed")
    }
}



