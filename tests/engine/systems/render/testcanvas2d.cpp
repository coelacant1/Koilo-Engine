// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcanvas2d.cpp
 * @brief Implementation of Canvas2D unit tests.
 */

#include "testcanvas2d.hpp"

using namespace koilo;

// ========== Constructor Tests ==========

void TestCanvas2D::TestDefaultConstructor() {
    Canvas2D obj;
    TEST_ASSERT_EQUAL(0, obj.GetWidth());
    TEST_ASSERT_EQUAL(0, obj.GetHeight());
}

void TestCanvas2D::TestParameterizedConstructor() {
    // Canvas2D only has default ctor; resize is done via Resize()
    Canvas2D obj;
    obj.Resize(32, 24);
    TEST_ASSERT_EQUAL(32, obj.GetWidth());
    TEST_ASSERT_EQUAL(24, obj.GetHeight());
}

// ========== Method Tests ==========

// ========== Edge Cases ==========

void TestCanvas2D::TestEdgeCases() {
    Canvas2D::DetachAll();
    Canvas2D obj;
    // Drawing on uninitialized canvas should not crash
    obj.SetPixel(0, 0, 255, 0, 0);
    // Out-of-bounds should not crash
    obj.Resize(4, 4);
    obj.SetPixel(-1, -1, 255, 0, 0);
    obj.SetPixel(100, 100, 255, 0, 0);
    TEST_ASSERT_EQUAL(4, obj.GetWidth());

    // Attach/Detach
    TEST_ASSERT_FALSE(obj.IsAttached());
    obj.Attach();
    TEST_ASSERT_TRUE(obj.IsAttached());
    TEST_ASSERT_EQUAL(1u, Canvas2D::ActiveList().size());
    obj.Detach();
    TEST_ASSERT_FALSE(obj.IsAttached());
    TEST_ASSERT_EQUAL(0u, Canvas2D::ActiveList().size());

    // Multiple canvases
    Canvas2D c1, c2;
    c1.Resize(4, 4); c1.Attach();
    c2.Resize(4, 4); c2.Attach();
    TEST_ASSERT_EQUAL(2u, Canvas2D::ActiveList().size());
    Canvas2D::DetachAll();
    TEST_ASSERT_EQUAL(0u, Canvas2D::ActiveList().size());
}

// ========== Test Runner ==========

void TestCanvas2D::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);

    RUN_TEST(TestEdgeCases);

}
