/**
 * @file testcorners.cpp
 * @brief Implementation of Corners unit tests.
 */

#include "testcorners.hpp"

// ========== Constructor Tests ==========

void TestCorners::TestDefaultConstructor() {
    // Corners is a simple struct with an array of 4 Vector2D objects
    Rectangle2D::Corners corners;

    // Verify the struct can be instantiated
    // Default-constructed corners array should have 4 elements
    TEST_ASSERT_TRUE(sizeof(corners.corners) / sizeof(Vector2D) == 4);
}

void TestCorners::TestParameterizedConstructor() {
    // Test by getting corners from a Rectangle2D
    Vector2D center(0.0f, 0.0f);
    Vector2D size(4.0f, 2.0f);
    Rectangle2D rect(center, size, 0.0f);

    Rectangle2D::Corners corners = rect.GetCorners();

    // Verify we got 4 corners
    int validCorners = 0;
    for (int i = 0; i < 4; i++) {
        if (std::isfinite(corners.corners[i].X) && std::isfinite(corners.corners[i].Y)) {
            validCorners++;
        }
    }

    TEST_ASSERT_EQUAL(4, validCorners);

    // Verify at least one corner is at expected distance from center
    bool foundCornerAtCorrectDistance = false;
    for (int i = 0; i < 4; i++) {
        Vector2D corner = corners.corners[i];
        float distX = std::abs(corner.X);
        float distY = std::abs(corner.Y);

        // Corners should be at halfSize distance from center
        // For size (4,2), corners should be at (±2, ±1)
        if ((std::abs(distX - 2.0f) < 0.1f) && (std::abs(distY - 1.0f) < 0.1f)) {
            foundCornerAtCorrectDistance = true;
            break;
        }
    }

    TEST_ASSERT_TRUE(foundCornerAtCorrectDistance);
}

// ========== Edge Cases ==========

void TestCorners::TestEdgeCases() {
    // Test with rotated rectangle
    Vector2D center(5.0f, 5.0f);
    Vector2D size(6.0f, 4.0f);
    Rectangle2D rotatedRect(center, size, 45.0f);

    Rectangle2D::Corners rotatedCorners = rotatedRect.GetCorners();

    // All corners should be valid (finite values)
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_TRUE(std::isfinite(rotatedCorners.corners[i].X));
        TEST_ASSERT_TRUE(std::isfinite(rotatedCorners.corners[i].Y));
    }

    // Test with very small rectangle
    Vector2D tinySize(0.1f, 0.1f);
    Rectangle2D tinyRect(center, tinySize, 0.0f);

    Rectangle2D::Corners tinyCorners = tinyRect.GetCorners();

    // All corners should still be valid
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_TRUE(std::isfinite(tinyCorners.corners[i].X));
        TEST_ASSERT_TRUE(std::isfinite(tinyCorners.corners[i].Y));
    }

    // Test with very large rectangle
    Vector2D largeSize(1000.0f, 500.0f);
    Rectangle2D largeRect(center, largeSize, 0.0f);

    Rectangle2D::Corners largeCorners = largeRect.GetCorners();

    // All corners should still be valid
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_TRUE(std::isfinite(largeCorners.corners[i].X));
        TEST_ASSERT_TRUE(std::isfinite(largeCorners.corners[i].Y));
    }
}

// ========== Test Runner ==========

void TestCorners::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
