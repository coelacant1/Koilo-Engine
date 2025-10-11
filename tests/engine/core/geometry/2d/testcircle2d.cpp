/**
 * @file testcircle2d.cpp
 * @brief Implementation of Circle2D unit tests.
 */

#include "testcircle2d.hpp"
#include <utils/testhelpers.hpp>

// ========== Constructor Tests ==========

void TestCircle2D::TestDefaultConstructor() {
    // Circle2D has no default constructor - test with minimal parameters
    Vector2D center(0.0f, 0.0f);
    float radius = 1.0f;

    Circle2D circle(center, radius);

    // Verify the circle was constructed successfully
    Vector2D retrievedCenter = circle.GetCenter();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, retrievedCenter.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, retrievedCenter.Y);
}

void TestCircle2D::TestParameterizedConstructor() {
    // Test with specific center and radius
    Vector2D center(5.0f, 3.0f);
    float radius = 10.0f;

    Circle2D circle(center, radius);

    // Verify center is set correctly
    Vector2D retrievedCenter = circle.GetCenter();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 5.0f, retrievedCenter.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 3.0f, retrievedCenter.Y);

    // Test with different parameters
    Vector2D center2(-2.0f, -4.0f);
    float radius2 = 0.5f;
    Circle2D circle2(center2, radius2);

    Vector2D retrievedCenter2 = circle2.GetCenter();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, -2.0f, retrievedCenter2.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, -4.0f, retrievedCenter2.Y);
}

// ========== Method Tests ==========

void TestCircle2D::TestIsInShape() {
    Vector2D center(0.0f, 0.0f);
    float radius = 5.0f;
    Circle2D circle(center, radius);

    // Test point at center - should be inside
    Vector2D pointAtCenter(0.0f, 0.0f);
    TEST_ASSERT_TRUE(circle.IsInShape(pointAtCenter));

    // Test point clearly inside (distance = 3, radius = 5)
    Vector2D pointInside(3.0f, 0.0f);
    TEST_ASSERT_TRUE(circle.IsInShape(pointInside));

    // Test point on the boundary (distance = 5, radius = 5)
    Vector2D pointOnBoundary(5.0f, 0.0f);
    TEST_ASSERT_TRUE(circle.IsInShape(pointOnBoundary));

    // Test point clearly outside (distance = 7, radius = 5)
    Vector2D pointOutside(7.0f, 0.0f);
    TEST_ASSERT_FALSE(circle.IsInShape(pointOutside));

    // Test point at diagonal inside (distance = sqrt(3^2 + 4^2) = 5)
    Vector2D pointDiagonal(3.0f, 4.0f);
    TEST_ASSERT_TRUE(circle.IsInShape(pointDiagonal));

    // Test point at diagonal outside
    Vector2D pointDiagonalOut(4.0f, 4.0f);  // distance = sqrt(32) ≈ 5.66
    TEST_ASSERT_FALSE(circle.IsInShape(pointDiagonalOut));
}

// ========== Edge Cases ==========

void TestCircle2D::TestEdgeCases() {
    // Test with very small radius
    Vector2D center(0.0f, 0.0f);
    float tinyRadius = 0.001f;
    Circle2D tinyCircle(center, tinyRadius);

    Vector2D pointAtCenter(0.0f, 0.0f);
    TEST_ASSERT_TRUE(tinyCircle.IsInShape(pointAtCenter));

    Vector2D pointFarAway(1.0f, 1.0f);
    TEST_ASSERT_FALSE(tinyCircle.IsInShape(pointFarAway));

    // Test with large radius
    float largeRadius = 1000.0f;
    Circle2D largeCircle(center, largeRadius);

    Vector2D pointSomewhatFar(100.0f, 100.0f);  // distance ≈ 141
    TEST_ASSERT_TRUE(largeCircle.IsInShape(pointSomewhatFar));

    // Test with negative coordinates
    Vector2D negCenter(-10.0f, -20.0f);
    float radius = 5.0f;
    Circle2D negCircle(negCenter, radius);

    Vector2D pointNearNegCenter(-12.0f, -20.0f);  // distance = 2
    TEST_ASSERT_TRUE(negCircle.IsInShape(pointNearNegCenter));

    Vector2D pointFarFromNegCenter(-20.0f, -30.0f);  // distance > 5
    TEST_ASSERT_FALSE(negCircle.IsInShape(pointFarFromNegCenter));
}

// ========== Test Runner ==========

void TestCircle2D::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestIsInShape);
    RUN_TEST(TestEdgeCases);
}
