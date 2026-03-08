// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testtriangle3d.cpp
 * @brief Implementation of Triangle3D unit tests.
 */

#include "testtriangle3d.hpp"
#include <utils/testhelpers.hpp>

using namespace koilo;
// ========== Constructor Tests ==========

void TestTriangle3D::TestDefaultConstructor() {
    Triangle3D triangle;
    // Default constructor leaves pointers null
    TEST_ASSERT_NULL(triangle.p1);
    TEST_ASSERT_NULL(triangle.p2);
    TEST_ASSERT_NULL(triangle.p3);
}

void TestTriangle3D::TestParameterizedConstructor() {
    Vector3D v1(0.0f, 0.0f, 0.0f);
    Vector3D v2(1.0f, 0.0f, 0.0f);
    Vector3D v3(0.0f, 1.0f, 0.0f);

    Triangle3D triangle(&v1, &v2, &v3);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, triangle.p1->X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, triangle.p2->X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, triangle.p3->X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, triangle.p3->Y);
}

// ========== Method Tests ==========

void TestTriangle3D::TestGetArea() {
    Vector3D v1(0.0f, 0.0f, 0.0f);
    Vector3D v2(3.0f, 0.0f, 0.0f);
    Vector3D v3(0.0f, 4.0f, 0.0f);
    Triangle3D triangle(&v1, &v2, &v3);

    // Area = 0.5 * 3 * 4 = 6
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 6.0f, triangle.GetArea());

    // Equilateral triangle
    float side = 2.0f;
    Vector3D e1(0.0f, 0.0f, 0.0f);
    Vector3D e2(side, 0.0f, 0.0f);
    Vector3D e3(side/2.0f, side * 0.866f, 0.0f);
    Triangle3D equilateral(&e1, &e2, &e3);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.732f, equilateral.GetArea());
}

void TestTriangle3D::TestGetNormal() {
    Vector3D v1(0.0f, 0.0f, 0.0f);
    Vector3D v2(1.0f, 0.0f, 0.0f);
    Vector3D v3(0.0f, 1.0f, 0.0f);
    Triangle3D triangle(&v1, &v2, &v3);

    Vector3D normal = triangle.GetNormal();
    float length = normal.Magnitude();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, length);
    TEST_ASSERT_TRUE(std::abs(normal.Z) > 0.9f);
}

void TestTriangle3D::TestGetCentroid() {
    Vector3D v1(0.0f, 0.0f, 0.0f);
    Vector3D v2(3.0f, 0.0f, 0.0f);
    Vector3D v3(0.0f, 4.0f, 0.0f);
    Triangle3D triangle(&v1, &v2, &v3);

    Vector3D centroid = triangle.GetCentroid();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, centroid.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.333f, centroid.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, centroid.Z);

    Vector3D v4(1.0f, 2.0f, 3.0f);
    Vector3D v5(4.0f, 5.0f, 6.0f);
    Vector3D v6(7.0f, 8.0f, 9.0f);
    Triangle3D triangle2(&v4, &v5, &v6);
    Vector3D centroid2 = triangle2.GetCentroid();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.0f, centroid2.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, centroid2.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 6.0f, centroid2.Z);
}

void TestTriangle3D::TestClosestPoint() {
    Vector3D v1(0.0f, 0.0f, 0.0f);
    Vector3D v2(4.0f, 0.0f, 0.0f);
    Vector3D v3(2.0f, 3.0f, 0.0f);
    Triangle3D triangle(&v1, &v2, &v3);

    Vector3D centroid = triangle.GetCentroid();
    Vector3D closest1 = triangle.ClosestPoint(centroid);
    Vector3D diff1 = closest1 - centroid;
    TEST_ASSERT_TRUE(diff1.Magnitude() < 0.1f);

    Vector3D above(2.0f, 1.0f, 5.0f);
    Vector3D closest2 = triangle.ClosestPoint(above);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, closest2.Z);
}

// ========== Edge Cases ==========

void TestTriangle3D::TestEdgeCases() {
    // Degenerate triangle (collinear points)
    Vector3D col1(0.0f, 0.0f, 0.0f);
    Vector3D col2(1.0f, 0.0f, 0.0f);
    Vector3D col3(2.0f, 0.0f, 0.0f);
    Triangle3D degen(&col1, &col2, &col3);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, degen.GetArea());

    // Very small triangle
    Vector3D tiny1(0.0f, 0.0f, 0.0f);
    Vector3D tiny2(0.001f, 0.0f, 0.0f);
    Vector3D tiny3(0.0f, 0.001f, 0.0f);
    Triangle3D tinyTri(&tiny1, &tiny2, &tiny3);
    float tinyArea = tinyTri.GetArea();
    TEST_ASSERT_TRUE(tinyArea > 0.0f);
    TEST_ASSERT_TRUE(tinyArea < 0.001f);

    // Negative coordinates
    Vector3D neg1(-5.0f, -5.0f, -5.0f);
    Vector3D neg2(-2.0f, -5.0f, -5.0f);
    Vector3D neg3(-3.5f, -2.0f, -5.0f);
    Triangle3D negTri(&neg1, &neg2, &neg3);
    Vector3D negCentroid = negTri.GetCentroid();
    TEST_ASSERT_TRUE(negCentroid.X < 0.0f);
    TEST_ASSERT_TRUE(negCentroid.Y < 0.0f);
    TEST_ASSERT_TRUE(negCentroid.Z < 0.0f);

    // Large triangle
    Vector3D large1(0.0f, 0.0f, 0.0f);
    Vector3D large2(1000.0f, 0.0f, 0.0f);
    Vector3D large3(500.0f, 866.0f, 0.0f);
    Triangle3D largeTri(&large1, &large2, &large3);
    TEST_ASSERT_TRUE(largeTri.GetArea() > 100000.0f);
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
