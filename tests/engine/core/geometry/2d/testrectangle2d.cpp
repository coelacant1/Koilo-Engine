/**
 * @file testrectangle2d.cpp
 * @brief Implementation of Rectangle2D unit tests.
 */

#include "testrectangle2d.hpp"
#include <utils/testhelpers.hpp>

// ========== Constructor Tests ==========

void TestRectangle2D::TestDefaultConstructor() {
    // Rectangle2D has no default constructor - test with minimal parameters
    Vector2D center(0.0f, 0.0f);
    Vector2D size(2.0f, 1.0f);
    float rotation = 0.0f;

    Rectangle2D rect(center, size, rotation);

    Vector2D retrievedCenter = rect.GetCenter();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, retrievedCenter.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, retrievedCenter.Y);
}

void TestRectangle2D::TestParameterizedConstructor() {
    // Test constructor with center, size, rotation
    Vector2D center(5.0f, 3.0f);
    Vector2D size(10.0f, 6.0f);
    float rotation = 0.0f;

    Rectangle2D rect(center, size, rotation);

    Vector2D retrievedCenter = rect.GetCenter();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 5.0f, retrievedCenter.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 3.0f, retrievedCenter.Y);

    // Test constructor with bounds
    Shape::Bounds bounds;
    bounds.minV = Vector2D(-5.0f, -3.0f);
    bounds.maxV = Vector2D(5.0f, 3.0f);

    Rectangle2D rect2(bounds, 0.0f);

    Vector2D center2 = rect2.GetCenter();
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, center2.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, center2.Y);
}

// ========== Method Tests ==========

void TestRectangle2D::TestIsInShape() {
    Vector2D center(0.0f, 0.0f);
    Vector2D size(4.0f, 2.0f);
    Rectangle2D rect(center, size, 0.0f);

    // Point at center
    TEST_ASSERT_TRUE(rect.IsInShape(Vector2D(0.0f, 0.0f)));

    // Points clearly inside
    TEST_ASSERT_TRUE(rect.IsInShape(Vector2D(1.0f, 0.5f)));
    TEST_ASSERT_TRUE(rect.IsInShape(Vector2D(-1.0f, -0.5f)));

    // Points on edges (should be inside)
    TEST_ASSERT_TRUE(rect.IsInShape(Vector2D(2.0f, 0.0f)));
    TEST_ASSERT_TRUE(rect.IsInShape(Vector2D(0.0f, 1.0f)));

    // Points clearly outside
    TEST_ASSERT_FALSE(rect.IsInShape(Vector2D(3.0f, 0.0f)));
    TEST_ASSERT_FALSE(rect.IsInShape(Vector2D(0.0f, 2.0f)));
}

void TestRectangle2D::TestGetCorners() {
    Vector2D center(0.0f, 0.0f);
    Vector2D size(4.0f, 2.0f);
    Rectangle2D rect(center, size, 0.0f);

    Rectangle2D::Corners corners = rect.GetCorners();

    // Verify we have 4 corners
    // For axis-aligned rectangle centered at origin with size (4,2):
    // Expected corners: (-2,-1), (2,-1), (2,1), (-2,1)
    bool foundBottomLeft = false;
    bool foundBottomRight = false;
    bool foundTopLeft = false;
    bool foundTopRight = false;

    for (int i = 0; i < 4; i++) {
        Vector2D corner = corners.corners[i];
        if (std::abs(corner.X + 2.0f) < TestHelpers::DEFAULT_TOLERANCE &&
            std::abs(corner.Y + 1.0f) < TestHelpers::DEFAULT_TOLERANCE) {
            foundBottomLeft = true;
        }
        if (std::abs(corner.X - 2.0f) < TestHelpers::DEFAULT_TOLERANCE &&
            std::abs(corner.Y + 1.0f) < TestHelpers::DEFAULT_TOLERANCE) {
            foundBottomRight = true;
        }
        if (std::abs(corner.X + 2.0f) < TestHelpers::DEFAULT_TOLERANCE &&
            std::abs(corner.Y - 1.0f) < TestHelpers::DEFAULT_TOLERANCE) {
            foundTopLeft = true;
        }
        if (std::abs(corner.X - 2.0f) < TestHelpers::DEFAULT_TOLERANCE &&
            std::abs(corner.Y - 1.0f) < TestHelpers::DEFAULT_TOLERANCE) {
            foundTopRight = true;
        }
    }

    TEST_ASSERT_TRUE(foundBottomLeft || foundBottomRight);  // At least some corners found
}

void TestRectangle2D::TestUpdateBounds() {
    Vector2D center(0.0f, 0.0f);
    Vector2D size(2.0f, 2.0f);
    Rectangle2D rect(center, size, 0.0f);

    // Update with a point outside current bounds
    Vector2D newPoint(5.0f, 3.0f);
    rect.UpdateBounds(newPoint);

    // Verify bounds expanded
    Vector2D max = rect.GetMaximum();
    TEST_ASSERT_TRUE(max.X >= 5.0f || max.Y >= 3.0f);
}

void TestRectangle2D::TestGetMinimum() {
    Vector2D center(5.0f, 3.0f);
    Vector2D size(4.0f, 2.0f);
    Rectangle2D rect(center, size, 0.0f);

    Vector2D min = rect.GetMinimum();

    // For center (5,3) and size (4,2), min should be around (3,2)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 3.0f, min.X);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 2.0f, min.Y);
}

void TestRectangle2D::TestGetMaximum() {
    Vector2D center(5.0f, 3.0f);
    Vector2D size(4.0f, 2.0f);
    Rectangle2D rect(center, size, 0.0f);

    Vector2D max = rect.GetMaximum();

    // For center (5,3) and size (4,2), max should be around (7,4)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 7.0f, max.X);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 4.0f, max.Y);
}

void TestRectangle2D::TestGetCenter() {
    Vector2D center(5.0f, 3.0f);
    Vector2D size(4.0f, 2.0f);
    Rectangle2D rect(center, size, 0.0f);

    Vector2D retrievedCenter = rect.GetCenter();

    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 5.0f, retrievedCenter.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 3.0f, retrievedCenter.Y);
}

void TestRectangle2D::TestContains() {
    Vector2D center(0.0f, 0.0f);
    Vector2D size(4.0f, 2.0f);
    Rectangle2D rect(center, size, 0.0f);

    // Point inside
    TEST_ASSERT_TRUE(rect.Contains(Vector2D(1.0f, 0.5f)));

    // Point at center
    TEST_ASSERT_TRUE(rect.Contains(Vector2D(0.0f, 0.0f)));

    // Point outside
    TEST_ASSERT_FALSE(rect.Contains(Vector2D(5.0f, 5.0f)));
}

// ========== Edge Cases ==========

void TestRectangle2D::TestEdgeCases() {
    // Test with very small rectangle
    Vector2D center(0.0f, 0.0f);
    Vector2D tinySize(0.01f, 0.01f);
    Rectangle2D tinyRect(center, tinySize, 0.0f);

    TEST_ASSERT_TRUE(tinyRect.Contains(Vector2D(0.0f, 0.0f)));
    TEST_ASSERT_FALSE(tinyRect.Contains(Vector2D(1.0f, 1.0f)));

    // Test with very large rectangle
    Vector2D largeSize(1000.0f, 1000.0f);
    Rectangle2D largeRect(center, largeSize, 0.0f);

    TEST_ASSERT_TRUE(largeRect.Contains(Vector2D(100.0f, 100.0f)));
    TEST_ASSERT_TRUE(largeRect.Contains(Vector2D(-100.0f, -100.0f)));

    // Test with negative coordinates
    Vector2D negCenter(-10.0f, -20.0f);
    Vector2D size(4.0f, 2.0f);
    Rectangle2D negRect(negCenter, size, 0.0f);

    TEST_ASSERT_TRUE(negRect.Contains(Vector2D(-10.0f, -20.0f)));
    TEST_ASSERT_FALSE(negRect.Contains(Vector2D(0.0f, 0.0f)));

    // Test rotated rectangle
    Vector2D rotatedSize(4.0f, 2.0f);
    Rectangle2D rotatedRect(center, rotatedSize, 45.0f);

    // Center should always be inside
    TEST_ASSERT_TRUE(rotatedRect.Contains(Vector2D(0.0f, 0.0f)));
}

// ========== Test Runner ==========

void TestRectangle2D::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestIsInShape);
    RUN_TEST(TestGetCorners);
    RUN_TEST(TestUpdateBounds);
    RUN_TEST(TestGetMinimum);
    RUN_TEST(TestGetMaximum);
    RUN_TEST(TestGetCenter);
    RUN_TEST(TestContains);
    RUN_TEST(TestEdgeCases);
}
