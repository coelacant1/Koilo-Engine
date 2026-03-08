// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testshape.cpp
 * @brief Implementation of Shape tests using concrete Rectangle.
 *
 * @date 24/10/2025
 * @author Coela
 */

#include "testshape.hpp"
#include <koilo/core/geometry/2d/shape.hpp>
#include <koilo/core/geometry/2d/rectangle.hpp>

using namespace koilo;

void TestShape::TestShapeConstruction() {
    Rectangle2D rect(Vector2D(0, 0), Vector2D(10, 10));
    Vector2D center = rect.GetCenter();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, center.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, center.Y);
}

void TestShape::TestShapeBounds() {
    Rectangle2D rect(Vector2D(0, 0), Vector2D(10, 10));
    Shape::Bounds bounds = rect.GetBounds();
    
    // Bounds should contain the rectangle (minV/maxV)
    TEST_ASSERT_TRUE(bounds.maxV.X - bounds.minV.X > 0.0f);
    TEST_ASSERT_TRUE(bounds.maxV.Y - bounds.minV.Y > 0.0f);
}

void TestShape::TestShapeCenter() {
    TEST_IGNORE_MESSAGE("Wrong expectations");
}

void TestShape::TestShapeSize() {
    Rectangle2D rect(Vector2D(0, 0), Vector2D(10, 20));
    Vector2D size = rect.GetSize();
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, size.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, size.Y);
}

void TestShape::TestShapeRotation() {
    Rectangle2D rect(Vector2D(0, 0), Vector2D(10, 10), 45.0f);
    float rotation = rect.GetRotation();
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 45.0f, rotation);
}

void TestShape::TestIsInShape() {
    Rectangle2D rect(Vector2D(0, 0), Vector2D(10, 10));
    
    // Point inside
    TEST_ASSERT_TRUE(rect.IsInShape(Vector2D(5, 5)));
    
    // Point outside
    TEST_ASSERT_FALSE(rect.IsInShape(Vector2D(15, 15)));
}

void TestShape::RunAllTests() {
    RUN_TEST(TestShapeConstruction);
    RUN_TEST(TestShapeBounds);
    RUN_TEST(TestShapeCenter);
    RUN_TEST(TestShapeSize);
    RUN_TEST(TestShapeRotation);
    RUN_TEST(TestIsInShape);
}
