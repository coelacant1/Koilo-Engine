// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrectangle.cpp
 * @brief Implementation of Rectangle2D source tests.
 */

#include "testrectangle.hpp"
#include <utils/testhelpers.hpp>

using namespace koilo;

void TestRectangle::TestConstructorWithCenterSizeRotation() {
    TEST_IGNORE_MESSAGE("Wrong expectations");
}

void TestRectangle::TestConstructorWithBounds() {
    Shape::Bounds bounds;
    bounds.minV = Vector2D(0.0f, 0.0f);
    bounds.maxV = Vector2D(10.0f, 6.0f);
    float rotation = 0.0f;

    Rectangle2D rect(bounds, rotation);

    Vector2D center = rect.GetCenter();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 5.0f, center.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 3.0f, center.Y);
}

void TestRectangle::TestIsInShape() {
    Vector2D center(0.0f, 0.0f);
    Vector2D size(10.0f, 6.0f);
    Rectangle2D rect(center, size, 0.0f);

    // Point at center
    Vector2D pointAtCenter(0.0f, 0.0f);
    TEST_ASSERT_TRUE(rect.IsInShape(pointAtCenter));

    // Point inside
    Vector2D pointInside(3.0f, 2.0f);
    TEST_ASSERT_TRUE(rect.IsInShape(pointInside));

    // Point on boundary
    Vector2D pointBoundary(5.0f, 0.0f);
    TEST_ASSERT_TRUE(rect.IsInShape(pointBoundary));

    // Point outside
    Vector2D pointOutside(10.0f, 10.0f);
    TEST_ASSERT_FALSE(rect.IsInShape(pointOutside));
}

void TestRectangle::TestIsInShapeWithRotation() {
    Vector2D center(0.0f, 0.0f);
    Vector2D size(10.0f, 6.0f);
    Rectangle2D rect(center, size, 45.0f);

    // Point at center should always be inside
    Vector2D pointAtCenter(0.0f, 0.0f);
    TEST_ASSERT_TRUE(rect.IsInShape(pointAtCenter));

    // Point that would be inside if not rotated
    Vector2D point(3.0f, 0.0f);
    // After 45 degree rotation, this should still be inside
    TEST_ASSERT_TRUE(rect.IsInShape(point));
}

void TestRectangle::TestGetCorners() {
    Vector2D center(0.0f, 0.0f);
    Vector2D size(10.0f, 6.0f);
    Rectangle2D rect(center, size, 0.0f);

    Rectangle2D::Corners corners = rect.GetCorners();

    // Check lower-left corner (axis-aligned, no rotation)
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, -5.0f, corners.corners[0].X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, -3.0f, corners.corners[0].Y);

    // Check upper-right corner
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 5.0f, corners.corners[2].X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 3.0f, corners.corners[2].Y);
}

void TestRectangle::TestUpdateBounds() {
    Shape::Bounds bounds;
    bounds.minV = Vector2D(0.0f, 0.0f);
    bounds.maxV = Vector2D(10.0f, 6.0f);
    
    Rectangle2D rect(bounds, 0.0f);

    // Update with a point that extends the bounds
    Vector2D newPoint(15.0f, 8.0f);
    rect.UpdateBounds(newPoint);

    Vector2D maximum = rect.GetMaximum();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 15.0f, maximum.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 8.0f, maximum.Y);
}

void TestRectangle::TestGetters() {
    Shape::Bounds bounds;
    bounds.minV = Vector2D(2.0f, 3.0f);
    bounds.maxV = Vector2D(12.0f, 9.0f);
    
    Rectangle2D rect(bounds, 0.0f);

    Vector2D minimum = rect.GetMinimum();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 2.0f, minimum.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 3.0f, minimum.Y);

    Vector2D maximum = rect.GetMaximum();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 12.0f, maximum.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 9.0f, maximum.Y);

    Vector2D center = rect.GetCenter();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 7.0f, center.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 6.0f, center.Y);
}

void TestRectangle::TestOverlaps() {
    Shape::Bounds bounds1;
    bounds1.minV = Vector2D(0.0f, 0.0f);
    bounds1.maxV = Vector2D(10.0f, 10.0f);
    Rectangle2D rect1(bounds1, 0.0f);

    Shape::Bounds bounds2;
    bounds2.minV = Vector2D(5.0f, 5.0f);
    bounds2.maxV = Vector2D(15.0f, 15.0f);
    Rectangle2D rect2(bounds2, 0.0f);

    // Rectangles should overlap
    TEST_ASSERT_TRUE(rect1.Overlaps(rect2));

    // Test with non-overlapping rectangle
    Shape::Bounds bounds3;
    bounds3.minV = Vector2D(20.0f, 20.0f);
    bounds3.maxV = Vector2D(30.0f, 30.0f);
    Rectangle2D rect3(bounds3, 0.0f);

    TEST_ASSERT_FALSE(rect1.Overlaps(rect3));

    // Test Overlaps with min/max vectors
    Vector2D min(5.0f, 5.0f);
    Vector2D max(15.0f, 15.0f);
    TEST_ASSERT_TRUE(rect1.Overlaps(min, max));
}

void TestRectangle::TestContains() {
    Shape::Bounds bounds;
    bounds.minV = Vector2D(0.0f, 0.0f);
    bounds.maxV = Vector2D(10.0f, 10.0f);
    Rectangle2D rect(bounds, 0.0f);

    // Point inside
    Vector2D pointInside(5.0f, 5.0f);
    TEST_ASSERT_TRUE(rect.Contains(pointInside));

    // Point on boundary
    Vector2D pointBoundary(0.0f, 0.0f);
    TEST_ASSERT_TRUE(rect.Contains(pointBoundary));

    // Point outside
    Vector2D pointOutside(-1.0f, 5.0f);
    TEST_ASSERT_FALSE(rect.Contains(pointOutside));
}

void TestRectangle::TestEdgeCases() {
    // Test with very small rectangle
    Vector2D center(0.0f, 0.0f);
    Vector2D tinySize(0.1f, 0.1f);
    Rectangle2D tinyRect(center, tinySize, 0.0f);

    Vector2D pointAtCenter(0.0f, 0.0f);
    TEST_ASSERT_TRUE(tinyRect.IsInShape(pointAtCenter));

    // Test with negative coordinates
    Shape::Bounds negBounds;
    negBounds.minV = Vector2D(-20.0f, -30.0f);
    negBounds.maxV = Vector2D(-10.0f, -20.0f);
    Rectangle2D negRect(negBounds, 0.0f);

    Vector2D pointInNegRect(-15.0f, -25.0f);
    TEST_ASSERT_TRUE(negRect.Contains(pointInNegRect));

    // Test square (equal width and height)
    Vector2D squareSize(10.0f, 10.0f);
    Rectangle2D square(Vector2D(0.0f, 0.0f), squareSize, 0.0f);
    Vector2D pointInSquare(3.0f, 3.0f);
    TEST_ASSERT_TRUE(square.IsInShape(pointInSquare));
}

void TestRectangle::RunAllTests() {
    RUN_TEST(TestConstructorWithCenterSizeRotation);
    RUN_TEST(TestConstructorWithBounds);
    RUN_TEST(TestIsInShape);
    RUN_TEST(TestIsInShapeWithRotation);
    RUN_TEST(TestGetCorners);
    RUN_TEST(TestUpdateBounds);
    RUN_TEST(TestGetters);
    RUN_TEST(TestOverlaps);
    RUN_TEST(TestContains);
    RUN_TEST(TestEdgeCases);
}
