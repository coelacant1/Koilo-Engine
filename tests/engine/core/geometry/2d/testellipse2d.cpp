/**
 * @file testellipse2d.cpp
 * @brief Implementation of Ellipse2D unit tests.
 */

#include "testellipse2d.hpp"
#include <utils/testhelpers.hpp>

// ========== Constructor Tests ==========

void TestEllipse2D::TestDefaultConstructor() {
    // Ellipse2D has no default constructor - test with minimal parameters
    Vector2D center(0.0f, 0.0f);
    Vector2D size(2.0f, 1.0f);  // width = 2, height = 1
    float rotation = 0.0f;

    Ellipse2D ellipse(center, size, rotation);

    // Verify the ellipse was constructed successfully
    Vector2D retrievedCenter = ellipse.GetCenter();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, retrievedCenter.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, retrievedCenter.Y);

    Vector2D retrievedSize = ellipse.GetSize();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 2.0f, retrievedSize.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 1.0f, retrievedSize.Y);
}

void TestEllipse2D::TestParameterizedConstructor() {
    // Test constructor with center, size, and rotation
    Vector2D center(5.0f, 3.0f);
    Vector2D size(10.0f, 6.0f);
    float rotation = 45.0f;

    Ellipse2D ellipse(center, size, rotation);

    Vector2D retrievedCenter = ellipse.GetCenter();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 5.0f, retrievedCenter.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 3.0f, retrievedCenter.Y);

    Vector2D retrievedSize = ellipse.GetSize();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 10.0f, retrievedSize.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 6.0f, retrievedSize.Y);

    float retrievedRotation = ellipse.GetRotation();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 45.0f, retrievedRotation);

    // Test constructor with bounds
    Shape::Bounds bounds;
    bounds.minV = Vector2D(-5.0f, -3.0f);
    bounds.maxV = Vector2D(5.0f, 3.0f);

    Ellipse2D ellipse2(bounds, 0.0f);

    Vector2D center2 = ellipse2.GetCenter();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, center2.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, center2.Y);
}

// ========== Method Tests ==========

void TestEllipse2D::TestIsInShape() {
    // Create an axis-aligned ellipse centered at origin
    Vector2D center(0.0f, 0.0f);
    Vector2D size(4.0f, 2.0f);  // semi-major axis = 2, semi-minor axis = 1
    float rotation = 0.0f;

    Ellipse2D ellipse(center, size, rotation);

    // Test point at center - should be inside
    Vector2D pointAtCenter(0.0f, 0.0f);
    TEST_ASSERT_TRUE(ellipse.IsInShape(pointAtCenter));

    // Test point along major axis (inside)
    Vector2D pointMajorInside(1.5f, 0.0f);
    TEST_ASSERT_TRUE(ellipse.IsInShape(pointMajorInside));

    // Test point along minor axis (inside)
    Vector2D pointMinorInside(0.0f, 0.8f);
    TEST_ASSERT_TRUE(ellipse.IsInShape(pointMinorInside));

    // Test point outside along major axis
    Vector2D pointMajorOutside(2.5f, 0.0f);
    TEST_ASSERT_FALSE(ellipse.IsInShape(pointMajorOutside));

    // Test point outside along minor axis
    Vector2D pointMinorOutside(0.0f, 1.5f);
    TEST_ASSERT_FALSE(ellipse.IsInShape(pointMinorOutside));

    // Test point in first quadrant (inside ellipse)
    Vector2D pointQuadrant(1.0f, 0.5f);
    TEST_ASSERT_TRUE(ellipse.IsInShape(pointQuadrant));
}

// ========== Edge Cases ==========

void TestEllipse2D::TestEdgeCases() {
    // Test very small ellipse
    Vector2D center(0.0f, 0.0f);
    Vector2D tinySize(0.01f, 0.01f);
    Ellipse2D tinyEllipse(center, tinySize, 0.0f);

    Vector2D pointAtCenter(0.0f, 0.0f);
    TEST_ASSERT_TRUE(tinyEllipse.IsInShape(pointAtCenter));

    Vector2D pointFarAway(1.0f, 1.0f);
    TEST_ASSERT_FALSE(tinyEllipse.IsInShape(pointFarAway));

    // Test very elongated ellipse
    Vector2D longSize(100.0f, 1.0f);
    Ellipse2D longEllipse(center, longSize, 0.0f);

    Vector2D pointAlongMajor(40.0f, 0.0f);
    TEST_ASSERT_TRUE(longEllipse.IsInShape(pointAlongMajor));

    Vector2D pointOffMinor(0.0f, 2.0f);
    TEST_ASSERT_FALSE(longEllipse.IsInShape(pointOffMinor));

    // Test with negative coordinates
    Vector2D negCenter(-10.0f, -20.0f);
    Vector2D size(6.0f, 4.0f);
    Ellipse2D negEllipse(negCenter, size, 0.0f);

    Vector2D pointNearNegCenter(-11.0f, -20.0f);
    TEST_ASSERT_TRUE(negEllipse.IsInShape(pointNearNegCenter));

    Vector2D pointFarFromNegCenter(-20.0f, -30.0f);
    TEST_ASSERT_FALSE(negEllipse.IsInShape(pointFarFromNegCenter));

    // Test rotated ellipse (45 degrees)
    Vector2D rotCenter(0.0f, 0.0f);
    Vector2D rotSize(4.0f, 2.0f);
    float rotation = 45.0f;
    Ellipse2D rotatedEllipse(rotCenter, rotSize, rotation);

    // Center should always be inside regardless of rotation
    TEST_ASSERT_TRUE(rotatedEllipse.IsInShape(pointAtCenter));
}

// ========== Test Runner ==========

void TestEllipse2D::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestIsInShape);
    RUN_TEST(TestEdgeCases);
}
