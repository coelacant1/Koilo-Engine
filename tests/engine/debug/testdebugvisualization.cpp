// SPDX-License-Identifier: GPL-3.0-or-later
#include "testdebugvisualization.hpp"
#include <koilo/debug/debug_visualization.hpp>
#include <koilo/debug/debugdraw.hpp>
#include <koilo/kernel/cvar/cvar_system.hpp>

using namespace koilo;

namespace TestDebugVisualization {

void TestDrawWireBoxEmitsLines() {
    auto& dd = DebugDraw::GetInstance();
    dd.Clear();
    dd.Enable();

    // A box has 12 edges
    DebugVisualization::DrawWireBox(
        Vector3D(0, 0, 0), Vector3D(1, 1, 1), Color::Green);

    const auto& lines = dd.GetLines();
    TEST_ASSERT_EQUAL_INT(12, (int)lines.size());

    dd.Clear();
}

void TestDrawWireSphereEmitsLines() {
    auto& dd = DebugDraw::GetInstance();
    dd.Clear();
    dd.Enable();

    // 3 rings of 16 segments each = 48 lines
    DebugVisualization::DrawWireSphere(
        Vector3D(0, 0, 0), 1.0f, Color::Cyan, 16);

    const auto& lines = dd.GetLines();
    TEST_ASSERT_EQUAL_INT(48, (int)lines.size());

    dd.Clear();
}

void TestDrawSceneBoundsNullSafe() {
    // Should not crash with null
    DebugVisualization::DrawSceneBounds(nullptr);
}

void TestDrawPhysicsCollidersNullSafe() {
    // Should not crash with null
    DebugVisualization::DrawPhysicsColliders(nullptr);
}

void TestDrawCameraFrustumNullSafe() {
    // Should not crash with null
    DebugVisualization::DrawCameraFrustum(nullptr, 1.0f);
}

void TestDrawCrossEmitsSixVertices() {
    auto& dd = DebugDraw::GetInstance();
    dd.Clear();
    dd.Enable();

    // DrawCross emits 3 lines (one per axis)
    DebugVisualization::DrawCross(Vector3D(0, 0, 0), 0.5f, Color::Red);

    const auto& lines = dd.GetLines();
    TEST_ASSERT_EQUAL_INT(3, (int)lines.size());

    dd.Clear();
}

void TestCVarDefaultsOff() {
    auto& sys = CVarSystem::Get();

    auto* physDbg = sys.Find("physics.debug");
    TEST_ASSERT_NOT_NULL(physDbg);
    TEST_ASSERT_FALSE(physDbg->boolVal);

    auto* rBounds = sys.Find("r.bounds");
    TEST_ASSERT_NOT_NULL(rBounds);
    TEST_ASSERT_FALSE(rBounds->boolVal);
}

void RunAllTests() {
    RUN_TEST(TestDrawWireBoxEmitsLines);
    RUN_TEST(TestDrawWireSphereEmitsLines);
    RUN_TEST(TestDrawSceneBoundsNullSafe);
    RUN_TEST(TestDrawPhysicsCollidersNullSafe);
    RUN_TEST(TestDrawCameraFrustumNullSafe);
    RUN_TEST(TestDrawCrossEmitsSixVertices);
    RUN_TEST(TestCVarDefaultsOff);
}

} // namespace TestDebugVisualization
