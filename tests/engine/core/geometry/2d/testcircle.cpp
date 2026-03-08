// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcircle.cpp
 * @brief Implementation of Circle2D source tests.
 */

#include "testcircle.hpp"
#include <utils/testhelpers.hpp>

using namespace koilo;

void TestCircle::TestConstructor() {
    Vector2D center(5.0f, 3.0f);
    float radius = 10.0f;

    Circle2D circle(center, radius);

    Vector2D retrievedCenter = circle.GetCenter();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 5.0f, retrievedCenter.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 3.0f, retrievedCenter.Y);
}

void TestCircle::TestIsInShape() {
    Vector2D center(0.0f, 0.0f);
    float radius = 5.0f;
    Circle2D circle(center, radius);

    // Point inside
    Vector2D pointInside(3.0f, 0.0f);
    TEST_ASSERT_TRUE(circle.IsInShape(pointInside));

    // Point outside (boundary is exclusive in implementation: x*x + y*y < powRad)
    Vector2D pointOutside(6.0f, 0.0f);
    TEST_ASSERT_FALSE(circle.IsInShape(pointOutside));

    // Point exactly on boundary (distance = radius)
    Vector2D pointBoundary(5.0f, 0.0f);
    // Note: IsInShape uses < not <=, so boundary is outside
    TEST_ASSERT_FALSE(circle.IsInShape(pointBoundary));
}

void TestCircle::TestEdgeCases() {
    // Test with very small radius
    Vector2D center(0.0f, 0.0f);
    float tinyRadius = 0.001f;
    Circle2D tinyCircle(center, tinyRadius);

    Vector2D pointAtCenter(0.0f, 0.0f);
    TEST_ASSERT_TRUE(tinyCircle.IsInShape(pointAtCenter));

    // Test with negative coordinates
    Vector2D negCenter(-10.0f, -20.0f);
    Circle2D negCircle(negCenter, 5.0f);

    Vector2D pointNearNegCenter(-12.0f, -20.0f);
    TEST_ASSERT_TRUE(negCircle.IsInShape(pointNearNegCenter));
}

void TestCircle::RunAllTests() {
    RUN_TEST(TestConstructor);
    RUN_TEST(TestIsInShape);
    RUN_TEST(TestEdgeCases);
}
