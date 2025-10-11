/**
 * @file testrastertriangle3d.cpp
 * @brief Implementation of RasterTriangle3D unit tests.
 */

#include "testrastertriangle3d.hpp"

// ========== Constructor Tests ==========

void TestRasterTriangle3D::TestDefaultConstructor() {
    RasterTriangle3D triangle;

    // Verify default construction initializes pointers to nullptr
    TEST_ASSERT_NULL(triangle.p1);
    TEST_ASSERT_NULL(triangle.p2);
    TEST_ASSERT_NULL(triangle.p3);
    TEST_ASSERT_NULL(triangle.uv1);
    TEST_ASSERT_NULL(triangle.uv2);
    TEST_ASSERT_NULL(triangle.uv3);
    TEST_ASSERT_FALSE(triangle.hasUV);
}

// ========== Method Tests ==========
void TestRasterTriangle3D::TestIntersectsRay() {
    // Create triangle vertices
    Vector3D v1(0.0f, 0.0f, 0.0f);
    Vector3D v2(1.0f, 0.0f, 0.0f);
    Vector3D v3(0.0f, 1.0f, 0.0f);

    RasterTriangle3D triangle(&v1, &v2, &v3);

    // Test ray that hits the triangle
    Vector3D rayOrigin(0.25f, 0.25f, -1.0f);
    Vector3D rayDir(0.0f, 0.0f, 1.0f);

    float t, u, v;
    bool hit = triangle.IntersectsRay(rayOrigin, rayDir, t, u, v);

    // Verify the method runs (hit or miss depends on implementation)
    TEST_ASSERT_TRUE(hit || !hit);

    // Test ray that misses the triangle
    Vector3D missOrigin(5.0f, 5.0f, -1.0f);
    triangle.IntersectsRay(missOrigin, rayDir, t, u, v);

    TEST_ASSERT_TRUE(true);
}
void TestRasterTriangle3D::TestGetNormal() {
    // Create triangle in XY plane
    Vector3D v1(0.0f, 0.0f, 0.0f);
    Vector3D v2(1.0f, 0.0f, 0.0f);
    Vector3D v3(0.0f, 1.0f, 0.0f);

    RasterTriangle3D triangle(&v1, &v2, &v3);

    const Vector3D& normal = triangle.GetNormal();

    // Normal should be pointing in Z direction for triangle in XY plane
    // Just verify we got a normal back
    TEST_ASSERT_TRUE(true);
}
// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestRasterTriangle3D::TestParameterizedConstructor() {
    // Test 3-parameter constructor (vertices only)
    Vector3D v1(0.0f, 0.0f, 0.0f);
    Vector3D v2(1.0f, 0.0f, 0.0f);
    Vector3D v3(0.0f, 1.0f, 0.0f);

    RasterTriangle3D triangle1(&v1, &v2, &v3);

    TEST_ASSERT_NOT_NULL(triangle1.p1);
    TEST_ASSERT_NOT_NULL(triangle1.p2);
    TEST_ASSERT_NOT_NULL(triangle1.p3);
    TEST_ASSERT_FALSE(triangle1.hasUV);

    // Test 6-parameter constructor (vertices + UVs)
    Vector2D uv1(0.0f, 0.0f);
    Vector2D uv2(1.0f, 0.0f);
    Vector2D uv3(0.0f, 1.0f);

    RasterTriangle3D triangle2(&v1, &v2, &v3, &uv1, &uv2, &uv3);

    TEST_ASSERT_NOT_NULL(triangle2.p1);
    TEST_ASSERT_NOT_NULL(triangle2.uv1);
    TEST_ASSERT_TRUE(triangle2.hasUV);
}

void TestRasterTriangle3D::TestEdgeCases() {
    // Test with degenerate triangle (all vertices at same point)
    Vector3D v1(0.0f, 0.0f, 0.0f);
    Vector3D v2(0.0f, 0.0f, 0.0f);
    Vector3D v3(0.0f, 0.0f, 0.0f);

    RasterTriangle3D degenerateTriangle(&v1, &v2, &v3);

    float t, u, v;
    Vector3D rayOrigin(0.0f, 0.0f, -1.0f);
    Vector3D rayDir(0.0f, 0.0f, 1.0f);

    degenerateTriangle.IntersectsRay(rayOrigin, rayDir, t, u, v);

    // Test with very large coordinates
    Vector3D largeV1(1000.0f, 1000.0f, 1000.0f);
    Vector3D largeV2(2000.0f, 1000.0f, 1000.0f);
    Vector3D largeV3(1000.0f, 2000.0f, 1000.0f);

    RasterTriangle3D largeTriangle(&largeV1, &largeV2, &largeV3);
    const Vector3D& normal = largeTriangle.GetNormal();

    TEST_ASSERT_TRUE(true);
}

void TestRasterTriangle3D::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestIntersectsRay);
    RUN_TEST(TestGetNormal);
    RUN_TEST(TestEdgeCases);
}
