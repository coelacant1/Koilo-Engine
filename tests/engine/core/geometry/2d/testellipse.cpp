// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testellipse.cpp
 * @brief Implementation of Ellipse2D source tests.
 */

#include "testellipse.hpp"
#include <utils/testhelpers.hpp>

using namespace koilo;

void TestEllipse::TestConstructorWithCenterSizeRotation() {
    Vector2D center(5.0f, 3.0f);
    Vector2D size(10.0f, 6.0f);
    float rotation = 45.0f;

    Ellipse2D ellipse(center, size, rotation);

    Vector2D retrievedCenter = ellipse.GetCenter();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 5.0f, retrievedCenter.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 3.0f, retrievedCenter.Y);
}

void TestEllipse::TestConstructorWithBounds() {
    Shape::Bounds bounds;
    bounds.minV = Vector2D(0.0f, 0.0f);
    bounds.maxV = Vector2D(10.0f, 6.0f);
    float rotation = 0.0f;

    Ellipse2D ellipse(bounds, rotation);

    // Constructor should set center from bounds
    Vector2D center = ellipse.GetCenter();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 5.0f, center.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 3.0f, center.Y);
}

void TestEllipse::TestIsInShape() {
    // Axis-aligned ellipse (no rotation)
    Vector2D center(0.0f, 0.0f);
    Vector2D size(10.0f, 6.0f);  // width=10, height=6 (radii are size/2)
    Ellipse2D ellipse(center, size, 0.0f);

    // Point at center
    Vector2D pointAtCenter(0.0f, 0.0f);
    TEST_ASSERT_TRUE(ellipse.IsInShape(pointAtCenter));

    // Point inside along X axis
    Vector2D pointInside(3.0f, 0.0f);  // well within radius_x = 5
    TEST_ASSERT_TRUE(ellipse.IsInShape(pointInside));

    // Point outside
    Vector2D pointOutside(10.0f, 5.0f);
    TEST_ASSERT_FALSE(ellipse.IsInShape(pointOutside));
}

void TestEllipse::TestIsInShapeWithRotation() {
    // Create ellipse with 45 degree rotation
    Vector2D center(0.0f, 0.0f);
    Vector2D size(10.0f, 6.0f);
    Ellipse2D ellipse(center, size, 45.0f);

    // Point at center should always be inside
    Vector2D pointAtCenter(0.0f, 0.0f);
    TEST_ASSERT_TRUE(ellipse.IsInShape(pointAtCenter));

    // With rotation, the ellipse is rotated 45 degrees
    // Testing rotated boundary behavior
    Vector2D pointClose(1.0f, 1.0f);
    TEST_ASSERT_TRUE(ellipse.IsInShape(pointClose));
}

void TestEllipse::TestEdgeCases() {
    // Test with very small ellipse
    Vector2D center(0.0f, 0.0f);
    Vector2D tinySize(0.1f, 0.05f);
    Ellipse2D tinyEllipse(center, tinySize, 0.0f);

    Vector2D pointAtCenter(0.0f, 0.0f);
    TEST_ASSERT_TRUE(tinyEllipse.IsInShape(pointAtCenter));

    // Test with negative coordinates
    Vector2D negCenter(-10.0f, -20.0f);
    Vector2D normalSize(8.0f, 4.0f);
    Ellipse2D negEllipse(negCenter, normalSize, 0.0f);

    Vector2D pointNearNegCenter(-11.0f, -20.0f);
    TEST_ASSERT_TRUE(negEllipse.IsInShape(pointNearNegCenter));

    // Test circle-like ellipse (equal width and height)
    Vector2D circleSize(8.0f, 8.0f);
    Ellipse2D circleEllipse(Vector2D(0.0f, 0.0f), circleSize, 0.0f);
    Vector2D pointInCircle(2.0f, 2.0f);
    TEST_ASSERT_TRUE(circleEllipse.IsInShape(pointInCircle));
}

void TestEllipse::RunAllTests() {
    RUN_TEST(TestConstructorWithCenterSizeRotation);
    RUN_TEST(TestConstructorWithBounds);
    RUN_TEST(TestIsInShape);
    RUN_TEST(TestIsInShapeWithRotation);
    RUN_TEST(TestEdgeCases);
}
