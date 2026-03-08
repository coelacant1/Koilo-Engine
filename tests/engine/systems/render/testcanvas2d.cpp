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

void TestCanvas2D::TestResize() {
    Canvas2D obj;
    obj.Resize(16, 16);
    TEST_ASSERT_EQUAL(16, obj.GetWidth());
    TEST_ASSERT_EQUAL(16, obj.GetHeight());
}

void TestCanvas2D::TestEnsureSize() {
    Canvas2D obj;
    obj.EnsureSize(8, 8);
    TEST_ASSERT_EQUAL(8, obj.GetWidth());
    // Calling again with same size should not resize
    obj.SetPixel(0, 0, 255, 0, 0);
    obj.EnsureSize(8, 8);
    TEST_ASSERT_EQUAL(8, obj.GetWidth());
}

void TestCanvas2D::TestClear() {
    Canvas2D obj;
    obj.Resize(4, 4);
    obj.SetPixel(0, 0, 255, 0, 0);
    std::vector<Color888> fb(16);
    obj.CompositeOnto(fb.data(), 4, 4);
    TEST_ASSERT_EQUAL(255, fb[0].R);
    obj.Clear();
    std::fill(fb.begin(), fb.end(), Color888(0, 0, 0));
    obj.CompositeOnto(fb.data(), 4, 4);
    TEST_ASSERT_EQUAL(0, fb[0].R); // cleared pixels not composited
}

void TestCanvas2D::TestClearWithColor() {
    Canvas2D obj;
    obj.Resize(4, 4);
    obj.ClearWithColor(64, 128, 192);
    std::vector<Color888> fb(16);
    obj.CompositeOnto(fb.data(), 4, 4);
    TEST_ASSERT_EQUAL(64, fb[0].R);
    TEST_ASSERT_EQUAL(128, fb[0].G);
    TEST_ASSERT_EQUAL(192, fb[0].B);
}

void TestCanvas2D::TestSetPixel() {
    Canvas2D obj;
    obj.Resize(8, 8);
    obj.SetPixel(3, 4, 255, 128, 64);
    std::vector<Color888> fb(64);
    obj.CompositeOnto(fb.data(), 8, 8);
    TEST_ASSERT_EQUAL(255, fb[4 * 8 + 3].R);
    TEST_ASSERT_EQUAL(128, fb[4 * 8 + 3].G);
    TEST_ASSERT_EQUAL(64,  fb[4 * 8 + 3].B);
}

void TestCanvas2D::TestFillRect() {
    Canvas2D obj;
    obj.Resize(8, 8);
    obj.FillRect(1, 1, 2, 2, 100, 200, 50);
    std::vector<Color888> fb(64);
    obj.CompositeOnto(fb.data(), 8, 8);
    TEST_ASSERT_EQUAL(100, fb[1 * 8 + 1].R);
    TEST_ASSERT_EQUAL(100, fb[2 * 8 + 2].R);
}

void TestCanvas2D::TestDrawRect() {
    Canvas2D obj;
    obj.Resize(8, 8);
    obj.DrawRect(0, 0, 4, 4, 200, 100, 50);
    std::vector<Color888> fb(64);
    obj.CompositeOnto(fb.data(), 8, 8);
    TEST_ASSERT_EQUAL(200, fb[0].R);     // top-left corner
    TEST_ASSERT_EQUAL(200, fb[3].R);     // top-right corner
    TEST_ASSERT_EQUAL(0, fb[1 * 8 + 1].R); // interior should be empty
}

void TestCanvas2D::TestDrawLine() {
    Canvas2D obj;
    obj.Resize(8, 8);
    obj.DrawLine(0, 0, 7, 0, 255, 0, 0); // horizontal line
    std::vector<Color888> fb(64);
    obj.CompositeOnto(fb.data(), 8, 8);
    TEST_ASSERT_EQUAL(255, fb[0].R);
    TEST_ASSERT_EQUAL(255, fb[7].R);
}

void TestCanvas2D::TestDrawCircle() {
    Canvas2D obj;
    obj.Resize(16, 16);
    obj.DrawCircle(8, 8, 3, 0, 255, 0);
    std::vector<Color888> fb(256);
    obj.CompositeOnto(fb.data(), 16, 16);
    // Top of circle at (8, 5) should be green
    TEST_ASSERT_EQUAL(255, fb[5 * 16 + 8].G);
}

void TestCanvas2D::TestFillCircle() {
    Canvas2D obj;
    obj.Resize(16, 16);
    obj.FillCircle(8, 8, 2, 0, 0, 255);
    std::vector<Color888> fb(256);
    obj.CompositeOnto(fb.data(), 16, 16);
    // Center should be blue
    TEST_ASSERT_EQUAL(255, fb[8 * 16 + 8].B);
}

void TestCanvas2D::TestGetWidth() {
    Canvas2D obj;
    obj.Resize(10, 20);
    TEST_ASSERT_EQUAL(10, obj.GetWidth());
}

void TestCanvas2D::TestGetHeight() {
    Canvas2D obj;
    obj.Resize(10, 20);
    TEST_ASSERT_EQUAL(20, obj.GetHeight());
}

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

void TestCanvas2D::TestAttach() {
    // TODO: Implement test for Attach()
    Canvas2D obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestCanvas2D::TestDetach() {
    // TODO: Implement test for Detach()
    Canvas2D obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestCanvas2D::TestDrawText() {
    // TODO: Implement test for DrawText()
    Canvas2D obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestCanvas2D::TestIsAttached() {
    // TODO: Implement test for IsAttached()
    Canvas2D obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestCanvas2D::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestResize);
    RUN_TEST(TestEnsureSize);
    RUN_TEST(TestClear);
    RUN_TEST(TestClearWithColor);
    RUN_TEST(TestSetPixel);
    RUN_TEST(TestFillRect);
    RUN_TEST(TestDrawRect);
    RUN_TEST(TestDrawLine);
    RUN_TEST(TestDrawCircle);
    RUN_TEST(TestFillCircle);
    RUN_TEST(TestGetWidth);
    RUN_TEST(TestGetHeight);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestAttach);
    RUN_TEST(TestDetach);
    RUN_TEST(TestDrawText);
    RUN_TEST(TestIsAttached);
}
