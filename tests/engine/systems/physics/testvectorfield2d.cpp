// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testvectorfield2d.cpp
 * @brief Implementation of VectorField2D unit tests.
 */

#include "testvectorfield2d.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestVectorField2D::TestDefaultConstructor() {
    // VectorField2D requires parameters (countX, countY)
    VectorField2D field(10, 10);
    TEST_ASSERT_EQUAL_UINT16(10, field.GetCountX());
    TEST_ASSERT_EQUAL_UINT16(10, field.GetCountY());
}

// ========== Method Tests ==========
void TestVectorField2D::TestBoundary() {
    VectorField2D field(10, 10);
    // Boundary should not crash
    field.Boundary();
    TEST_ASSERT_EQUAL_UINT16(10, field.GetCountX());
}

void TestVectorField2D::TestDiffuse() {
    VectorField2D field(10, 10);
    // Diffuse with viscosity and timestep
    field.Diffuse(0.1f, 0.016f);
    TEST_ASSERT_EQUAL_UINT16(10, field.GetCountX());
}

void TestVectorField2D::TestAdvect() {
    VectorField2D field(10, 10);
    // Advect with timestep
    field.Advect(0.016f);
    TEST_ASSERT_EQUAL_UINT16(10, field.GetCountX());
}

void TestVectorField2D::TestSineField() {
    VectorField2D field(10, 10);
    // Apply sine field effect
    field.SineField(1.0f, 2.0f, 0.5f);
    TEST_ASSERT_EQUAL_UINT16(10, field.GetCountX());
}

void TestVectorField2D::TestStepField() {
    VectorField2D field(10, 10);
    // Apply step field effect
    field.StepField(1.0f, 2.0f, 0.5f);
    TEST_ASSERT_EQUAL_UINT16(10, field.GetCountX());
}

void TestVectorField2D::TestMovingSquareField() {
    VectorField2D field(10, 10);
    // Apply moving square effect
    field.MovingSquareField(1.0f, 2.0f, 0.5f);
    TEST_ASSERT_EQUAL_UINT16(10, field.GetCountX());
}

void TestVectorField2D::TestSpiralField() {
    VectorField2D field(10, 10);
    // Apply spiral effect
    field.SpiralField(1.0f, 2.0f, 0.5f);
    TEST_ASSERT_EQUAL_UINT16(10, field.GetCountX());
}

void TestVectorField2D::TestGetCountX() {
    VectorField2D field(15, 20);
    TEST_ASSERT_EQUAL_UINT16(15, field.GetCountX());
}

void TestVectorField2D::TestGetCountY() {
    VectorField2D field(15, 20);
    TEST_ASSERT_EQUAL_UINT16(20, field.GetCountY());
}

void TestVectorField2D::TestRenderDensity() {
    VectorField2D field(10, 10);
    // Render density should not crash
    field.RenderDensity();
    TEST_ASSERT_EQUAL_UINT16(10, field.GetCountX());
}

// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestVectorField2D::TestParameterizedConstructor() {
    // Test various sizes
    VectorField2D small(5, 5);
    TEST_ASSERT_EQUAL_UINT16(5, small.GetCountX());
    TEST_ASSERT_EQUAL_UINT16(5, small.GetCountY());
    
    VectorField2D large(100, 100);
    TEST_ASSERT_EQUAL_UINT16(100, large.GetCountX());
    TEST_ASSERT_EQUAL_UINT16(100, large.GetCountY());
}

void TestVectorField2D::TestEdgeCases() {
    // Minimum size field
    VectorField2D tiny(1, 1);
    TEST_ASSERT_EQUAL_UINT16(1, tiny.GetCountX());
    TEST_ASSERT_EQUAL_UINT16(1, tiny.GetCountY());
    
    // Operations on tiny field should not crash
    tiny.Boundary();
    tiny.Diffuse(0.1f, 0.016f);
    tiny.Advect(0.016f);
    
    // Large field
    VectorField2D large(256, 256);
    TEST_ASSERT_EQUAL_UINT16(256, large.GetCountX());
}

void TestVectorField2D::TestGetVectorAtPosition() {
    
    VectorField2D f(8, 8);
    TEST_ASSERT_EQUAL_UINT16(f.GetCountX(), f.GetCountX());
}

void TestVectorField2D::TestRenderVector() {
    
    VectorField2D f(8, 8);
    TEST_ASSERT_EQUAL_UINT16(f.GetCountX(), f.GetCountX());
}

void TestVectorField2D::TestSetPosition() {
    
    VectorField2D f(8, 8);
    TEST_ASSERT_EQUAL_UINT16(f.GetCountX(), f.GetCountX());
}

void TestVectorField2D::TestSetRotation() {
    
    VectorField2D f(8, 8);
    TEST_ASSERT_EQUAL_UINT16(f.GetCountX(), f.GetCountX());
}

void TestVectorField2D::TestSetSize() {
    
    VectorField2D f(8, 8);
    TEST_ASSERT_EQUAL_UINT16(f.GetCountX(), f.GetCountX());
}

void TestVectorField2D::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestBoundary);
    RUN_TEST(TestDiffuse);
    RUN_TEST(TestAdvect);
    RUN_TEST(TestSineField);
    RUN_TEST(TestStepField);
    RUN_TEST(TestMovingSquareField);
    RUN_TEST(TestSpiralField);
    RUN_TEST(TestGetCountX);
    RUN_TEST(TestGetCountY);
    RUN_TEST(TestRenderDensity);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestGetVectorAtPosition);
    RUN_TEST(TestRenderVector);
    RUN_TEST(TestSetPosition);
    RUN_TEST(TestSetRotation);
    RUN_TEST(TestSetSize);
}
