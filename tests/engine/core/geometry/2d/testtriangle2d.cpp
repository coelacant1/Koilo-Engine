/**
 * @file testtriangle2d.cpp
 * @brief Implementation of Triangle2D unit tests.
 */

#include "testtriangle2d.hpp"
#include <utils/testhelpers.hpp>

// ========== Constructor Tests ==========

void TestTriangle2D::TestDefaultConstructor() {
    Triangle2D triangle;

    // Default triangle should exist at origin
    // Verify vertices are accessible
    TEST_ASSERT_TRUE(std::isfinite(triangle.p1.X));
    TEST_ASSERT_TRUE(std::isfinite(triangle.p1.Y));
    TEST_ASSERT_TRUE(std::isfinite(triangle.p2.X));
    TEST_ASSERT_TRUE(std::isfinite(triangle.p2.Y));
    TEST_ASSERT_TRUE(std::isfinite(triangle.p3.X));
    TEST_ASSERT_TRUE(std::isfinite(triangle.p3.Y));
}

void TestTriangle2D::TestParameterizedConstructor() {
    // Create a simple right triangle
    Vector2D p1(0.0f, 0.0f);
    Vector2D p2(3.0f, 0.0f);
    Vector2D p3(0.0f, 4.0f);

    Triangle2D triangle(p1, p2, p3);

    // Verify vertices are set correctly
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, triangle.p1.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, triangle.p1.Y);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 3.0f, triangle.p2.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, triangle.p2.Y);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, triangle.p3.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 4.0f, triangle.p3.Y);
}

// ========== Method Tests ==========

void TestTriangle2D::TestGetArea() {
    // Right triangle with base=3, height=4, area should be 6
    Vector2D p1(0.0f, 0.0f);
    Vector2D p2(3.0f, 0.0f);
    Vector2D p3(0.0f, 4.0f);

    Triangle2D triangle(p1, p2, p3);

    float area = triangle.GetArea();

    // Area of right triangle = 0.5 * base * height = 0.5 * 3 * 4 = 6
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 6.0f, area);

    // Test equilateral triangle
    // Side length = 2, area = (sqrt(3)/4) * side^2 = sqrt(3) ≈ 1.732
    Vector2D e1(0.0f, 0.0f);
    Vector2D e2(2.0f, 0.0f);
    Vector2D e3(1.0f, 1.732f);

    Triangle2D equilateral(e1, e2, e3);
    float areaEq = equilateral.GetArea();

    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.732f, areaEq);
}

void TestTriangle2D::TestGetCentroid() {
    // Triangle with vertices at (0,0), (3,0), (0,4)
    Vector2D p1(0.0f, 0.0f);
    Vector2D p2(3.0f, 0.0f);
    Vector2D p3(0.0f, 4.0f);

    Triangle2D triangle(p1, p2, p3);

    Vector2D centroid = triangle.GetCentroid();

    // Centroid = (p1 + p2 + p3) / 3 = (3/3, 4/3) = (1, 1.333...)
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, centroid.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.333f, centroid.Y);

    // Test with equilateral triangle centered at origin
    Vector2D eq1(-1.0f, 0.0f);
    Vector2D eq2(1.0f, 0.0f);
    Vector2D eq3(0.0f, 1.732f);

    Triangle2D equilateral(eq1, eq2, eq3);
    Vector2D centroidEq = equilateral.GetCentroid();

    // Centroid should be near (0, 0.577)
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, centroidEq.X);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.577f, centroidEq.Y);
}

void TestTriangle2D::TestIsInShape() {
    // Create a triangle
    Vector2D p1(0.0f, 0.0f);
    Vector2D p2(4.0f, 0.0f);
    Vector2D p3(2.0f, 3.0f);

    Triangle2D triangle(p1, p2, p3);

    // Point at centroid - should be inside
    Vector2D centroid = triangle.GetCentroid();
    TEST_ASSERT_TRUE(triangle.IsInShape(centroid));

    // Point at vertex - should be inside (on boundary)
    TEST_ASSERT_TRUE(triangle.IsInShape(p1));
    TEST_ASSERT_TRUE(triangle.IsInShape(p2));
    TEST_ASSERT_TRUE(triangle.IsInShape(p3));

    // Point clearly inside
    Vector2D inside(2.0f, 1.0f);
    TEST_ASSERT_TRUE(triangle.IsInShape(inside));

    // Point clearly outside
    Vector2D outside(10.0f, 10.0f);
    TEST_ASSERT_FALSE(triangle.IsInShape(outside));

    Vector2D outside2(-1.0f, 0.0f);
    TEST_ASSERT_FALSE(triangle.IsInShape(outside2));
}

// ========== Edge Cases ==========

void TestTriangle2D::TestEdgeCases() {
    // Test degenerate triangle (collinear points)
    Vector2D col1(0.0f, 0.0f);
    Vector2D col2(1.0f, 0.0f);
    Vector2D col3(2.0f, 0.0f);

    Triangle2D degenerateTriangle(col1, col2, col3);

    float area = degenerateTriangle.GetArea();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, area);

    // Test with very small triangle
    Vector2D tiny1(0.0f, 0.0f);
    Vector2D tiny2(0.001f, 0.0f);
    Vector2D tiny3(0.0f, 0.001f);

    Triangle2D tinyTriangle(tiny1, tiny2, tiny3);

    float tinyArea = tinyTriangle.GetArea();
    TEST_ASSERT_TRUE(tinyArea > 0.0f);
    TEST_ASSERT_TRUE(tinyArea < 0.001f);

    // Test with negative coordinates
    Vector2D neg1(-5.0f, -5.0f);
    Vector2D neg2(-2.0f, -5.0f);
    Vector2D neg3(-3.5f, -2.0f);

    Triangle2D negTriangle(neg1, neg2, neg3);

    Vector2D negCentroid = negTriangle.GetCentroid();
    TEST_ASSERT_TRUE(negCentroid.X < 0.0f);
    TEST_ASSERT_TRUE(negCentroid.Y < 0.0f);

    // Test large triangle
    Vector2D large1(0.0f, 0.0f);
    Vector2D large2(1000.0f, 0.0f);
    Vector2D large3(500.0f, 866.0f);

    Triangle2D largeTriangle(large1, large2, large3);

    float largeArea = largeTriangle.GetArea();
    TEST_ASSERT_TRUE(largeArea > 100000.0f);
}

// ========== Test Runner ==========

void TestTriangle2D::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetArea);
    RUN_TEST(TestGetCentroid);
    RUN_TEST(TestIsInShape);
    RUN_TEST(TestEdgeCases);
}
