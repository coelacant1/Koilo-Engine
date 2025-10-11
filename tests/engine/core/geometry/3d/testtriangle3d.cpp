/**
 * @file testtriangle3d.cpp
 * @brief Implementation of Triangle3D unit tests.
 */

#include "testtriangle3d.hpp"
#include <utils/testhelpers.hpp>

// ========== Constructor Tests ==========

void TestTriangle3D::TestDefaultConstructor() {
    Triangle3D triangle;

    // Default constructor creates degenerate triangle at origin
    // Verify vertices are accessible and have finite values
    TEST_ASSERT_TRUE(std::isfinite(triangle.p1.X));
    TEST_ASSERT_TRUE(std::isfinite(triangle.p1.Y));
    TEST_ASSERT_TRUE(std::isfinite(triangle.p1.Z));
    TEST_ASSERT_TRUE(std::isfinite(triangle.p2.X));
    TEST_ASSERT_TRUE(std::isfinite(triangle.p2.Y));
    TEST_ASSERT_TRUE(std::isfinite(triangle.p2.Z));
    TEST_ASSERT_TRUE(std::isfinite(triangle.p3.X));
    TEST_ASSERT_TRUE(std::isfinite(triangle.p3.Y));
    TEST_ASSERT_TRUE(std::isfinite(triangle.p3.Z));
}

void TestTriangle3D::TestParameterizedConstructor() {
    // Create a triangle in 3D space
    Vector3D v1(0.0f, 0.0f, 0.0f);
    Vector3D v2(1.0f, 0.0f, 0.0f);
    Vector3D v3(0.0f, 1.0f, 0.0f);

    Triangle3D triangle(v1, v2, v3);

    // Verify vertices are set correctly
    TEST_ASSERT_VECTOR3D_EQUAL(v1, triangle.p1);
    TEST_ASSERT_VECTOR3D_EQUAL(v2, triangle.p2);
    TEST_ASSERT_VECTOR3D_EQUAL(v3, triangle.p3);

    // Test with different vertices
    Vector3D v4(10.0f, 20.0f, 30.0f);
    Vector3D v5(15.0f, 25.0f, 35.0f);
    Vector3D v6(5.0f, 15.0f, 25.0f);

    Triangle3D triangle2(v4, v5, v6);

    TEST_ASSERT_VECTOR3D_EQUAL(v4, triangle2.p1);
    TEST_ASSERT_VECTOR3D_EQUAL(v5, triangle2.p2);
    TEST_ASSERT_VECTOR3D_EQUAL(v6, triangle2.p3);
}

// ========== Method Tests ==========

void TestTriangle3D::TestGetArea() {
    // Create a right triangle in XY plane
    Vector3D v1(0.0f, 0.0f, 0.0f);
    Vector3D v2(3.0f, 0.0f, 0.0f);
    Vector3D v3(0.0f, 4.0f, 0.0f);

    Triangle3D triangle(v1, v2, v3);

    float area = triangle.GetArea();

    // Area of right triangle = 0.5 * base * height = 0.5 * 3 * 4 = 6
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 6.0f, area);

    // Test equilateral triangle
    float side = 2.0f;
    Vector3D e1(0.0f, 0.0f, 0.0f);
    Vector3D e2(side, 0.0f, 0.0f);
    Vector3D e3(side/2.0f, side * 0.866f, 0.0f);  // sqrt(3)/2 ≈ 0.866

    Triangle3D equilateral(e1, e2, e3);
    float areaEq = equilateral.GetArea();

    // Area of equilateral triangle with side 2 = sqrt(3) ≈ 1.732
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.732f, areaEq);
}

void TestTriangle3D::TestGetNormal() {
    // Create a triangle in XY plane - normal should point in +Z direction
    Vector3D v1(0.0f, 0.0f, 0.0f);
    Vector3D v2(1.0f, 0.0f, 0.0f);
    Vector3D v3(0.0f, 1.0f, 0.0f);

    Triangle3D triangle(v1, v2, v3);

    Vector3D normal = triangle.GetNormal();

    // Normal should be normalized (length ≈ 1)
    float length = normal.Magnitude();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, length);

    // For counter-clockwise vertices in XY plane, normal should point in +Z
    // (or -Z depending on winding order)
    TEST_ASSERT_TRUE(std::abs(normal.Z) > 0.9f);  // Z component should dominate

    // Test triangle in XZ plane
    Vector3D v4(0.0f, 0.0f, 0.0f);
    Vector3D v5(1.0f, 0.0f, 0.0f);
    Vector3D v6(0.0f, 0.0f, 1.0f);

    Triangle3D triangle2(v4, v5, v6);
    Vector3D normal2 = triangle2.GetNormal();

    // Normal should still be unit length
    float length2 = normal2.Magnitude();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, length2);
}

void TestTriangle3D::TestGetCentroid() {
    // Triangle with vertices at (0,0,0), (3,0,0), (0,4,0)
    Vector3D v1(0.0f, 0.0f, 0.0f);
    Vector3D v2(3.0f, 0.0f, 0.0f);
    Vector3D v3(0.0f, 4.0f, 0.0f);

    Triangle3D triangle(v1, v2, v3);

    Vector3D centroid = triangle.GetCentroid();

    // Centroid = (v1 + v2 + v3) / 3 = (3/3, 4/3, 0/3) = (1, 1.333, 0)
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, centroid.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.333f, centroid.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, centroid.Z);

    // Test triangle in 3D space
    Vector3D v4(1.0f, 2.0f, 3.0f);
    Vector3D v5(4.0f, 5.0f, 6.0f);
    Vector3D v6(7.0f, 8.0f, 9.0f);

    Triangle3D triangle2(v4, v5, v6);
    Vector3D centroid2 = triangle2.GetCentroid();

    // Centroid = ((1+4+7)/3, (2+5+8)/3, (3+6+9)/3) = (4, 5, 6)
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.0f, centroid2.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, centroid2.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 6.0f, centroid2.Z);
}

void TestTriangle3D::TestClosestPoint() {
    // Create a triangle in XY plane
    Vector3D v1(0.0f, 0.0f, 0.0f);
    Vector3D v2(4.0f, 0.0f, 0.0f);
    Vector3D v3(2.0f, 3.0f, 0.0f);

    Triangle3D triangle(v1, v2, v3);

    // Point at centroid - should be closest to itself (on triangle)
    Vector3D centroid = triangle.GetCentroid();
    Vector3D closest1 = triangle.ClosestPoint(centroid);

    // Closest point to centroid should be very close to centroid
    Vector3D diff1 = closest1 - centroid;
    TEST_ASSERT_TRUE(diff1.Magnitude() < 0.1f);

    // Point directly above triangle center
    Vector3D above(2.0f, 1.0f, 5.0f);
    Vector3D closest2 = triangle.ClosestPoint(above);

    // Closest point should be on the triangle plane (Z ≈ 0)
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, closest2.Z);

    // Point far from triangle
    Vector3D farPoint(100.0f, 100.0f, 100.0f);
    Vector3D closest3 = triangle.ClosestPoint(farPoint);

    // Closest point should be one of the vertices or on an edge
    TEST_ASSERT_TRUE(std::isfinite(closest3.X));
    TEST_ASSERT_TRUE(std::isfinite(closest3.Y));
    TEST_ASSERT_TRUE(std::isfinite(closest3.Z));
}

// ========== Edge Cases ==========

void TestTriangle3D::TestEdgeCases() {
    // Test degenerate triangle (collinear points)
    Vector3D col1(0.0f, 0.0f, 0.0f);
    Vector3D col2(1.0f, 0.0f, 0.0f);
    Vector3D col3(2.0f, 0.0f, 0.0f);

    Triangle3D degenerateTriangle(col1, col2, col3);

    float area = degenerateTriangle.GetArea();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, area);

    // Test very small triangle
    Vector3D tiny1(0.0f, 0.0f, 0.0f);
    Vector3D tiny2(0.001f, 0.0f, 0.0f);
    Vector3D tiny3(0.0f, 0.001f, 0.0f);

    Triangle3D tinyTriangle(tiny1, tiny2, tiny3);

    float tinyArea = tinyTriangle.GetArea();
    TEST_ASSERT_TRUE(tinyArea > 0.0f);
    TEST_ASSERT_TRUE(tinyArea < 0.001f);

    // Test triangle with negative coordinates
    Vector3D neg1(-5.0f, -5.0f, -5.0f);
    Vector3D neg2(-2.0f, -5.0f, -5.0f);
    Vector3D neg3(-3.5f, -2.0f, -5.0f);

    Triangle3D negTriangle(neg1, neg2, neg3);

    Vector3D negCentroid = negTriangle.GetCentroid();
    TEST_ASSERT_TRUE(negCentroid.X < 0.0f);
    TEST_ASSERT_TRUE(negCentroid.Y < 0.0f);
    TEST_ASSERT_TRUE(negCentroid.Z < 0.0f);

    // Test large triangle
    Vector3D large1(0.0f, 0.0f, 0.0f);
    Vector3D large2(1000.0f, 0.0f, 0.0f);
    Vector3D large3(500.0f, 866.0f, 0.0f);

    Triangle3D largeTriangle(large1, large2, large3);

    float largeArea = largeTriangle.GetArea();
    TEST_ASSERT_TRUE(largeArea > 100000.0f);

    // Test normal of degenerate triangle (should still be finite)
    Vector3D degNormal = degenerateTriangle.GetNormal();
    TEST_ASSERT_TRUE(std::isfinite(degNormal.X));
    TEST_ASSERT_TRUE(std::isfinite(degNormal.Y));
    TEST_ASSERT_TRUE(std::isfinite(degNormal.Z));
}

// ========== Test Runner ==========

void TestTriangle3D::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetArea);
    RUN_TEST(TestGetNormal);
    RUN_TEST(TestGetCentroid);
    RUN_TEST(TestClosestPoint);
    RUN_TEST(TestEdgeCases);
}
