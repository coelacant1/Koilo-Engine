/**
 * @file testplane.cpp
 * @brief Implementation of Plane unit tests.
 */

#include "testplane.hpp"

// ========== Constructor Tests ==========

void TestPlane::TestDefaultConstructor() {
    Plane plane;

    // Default constructor should initialize fields
    // Verify the object was created successfully
    // Fields should have finite values (even if zero-initialized)
    TEST_ASSERT_TRUE(std::isfinite(plane.Centroid.X));
    TEST_ASSERT_TRUE(std::isfinite(plane.Centroid.Y));
    TEST_ASSERT_TRUE(std::isfinite(plane.Centroid.Z));
    TEST_ASSERT_TRUE(std::isfinite(plane.Normal.X));
    TEST_ASSERT_TRUE(std::isfinite(plane.Normal.Y));
    TEST_ASSERT_TRUE(std::isfinite(plane.Normal.Z));
}

void TestPlane::TestParameterizedConstructor() {
    Vector3D centroid(10.0f, 20.0f, 30.0f);
    Vector3D normal(0.0f, 1.0f, 0.0f);
    Plane plane(centroid, normal);

    // Verify centroid is set correctly
    TEST_ASSERT_VECTOR3D_EQUAL(centroid, plane.Centroid);

    // Verify normal is set correctly
    TEST_ASSERT_VECTOR3D_EQUAL(normal, plane.Normal);
}

// ========== Field Access Tests ==========

// ========== Method Tests ==========

void TestPlane::TestToString() {
    Vector3D centroid(1.0f, 2.0f, 3.0f);
    Vector3D normal(0.0f, 1.0f, 0.0f);
    Plane plane(centroid, normal);

    ptx::UString result = plane.ToString();

    // Verify that the string is not empty
    TEST_ASSERT_FALSE(result.IsEmpty());

    // Verify we can get the C string
    const char* str = result.CStr();
    TEST_ASSERT_TRUE(str != nullptr);

    // Verify the string contains expected content (manually check for brackets)
    bool hasOpenBracket = false;
    for (const char* p = str; *p != '\0'; ++p) {
        if (*p == '[') {
            hasOpenBracket = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasOpenBracket);
}

// ========== Edge Case Tests ==========

// ========== Test Runner ==========

void TestPlane::TestEdgeCases() {
    // Test with non-normalized normal vector
    Vector3D centroid(0.0f, 0.0f, 0.0f);
    Vector3D unnormalizedNormal(1.0f, 1.0f, 1.0f);
    Plane plane1(centroid, unnormalizedNormal);

    // Plane should accept any normal vector
    TEST_ASSERT_VECTOR3D_EQUAL(unnormalizedNormal, plane1.Normal);

    // Test with negative coordinates
    Vector3D negCentroid(-10.0f, -20.0f, -30.0f);
    Vector3D negNormal(0.0f, -1.0f, 0.0f);
    Plane plane2(negCentroid, negNormal);

    TEST_ASSERT_VECTOR3D_EQUAL(negCentroid, plane2.Centroid);
    TEST_ASSERT_VECTOR3D_EQUAL(negNormal, plane2.Normal);

    // Test with very small values
    Vector3D tinyC(0.001f, 0.001f, 0.001f);
    Vector3D tinyN(0.0f, 0.001f, 0.0f);
    Plane plane3(tinyC, tinyN);

    TEST_ASSERT_TRUE(std::isfinite(plane3.Centroid.X));
    TEST_ASSERT_TRUE(std::isfinite(plane3.Normal.Y));

    // Test with very large values
    Vector3D largeC(1000.0f, 2000.0f, 3000.0f);
    Vector3D largeN(0.0f, 1.0f, 0.0f);
    Plane plane4(largeC, largeN);

    TEST_ASSERT_VECTOR3D_EQUAL(largeC, plane4.Centroid);
    TEST_ASSERT_VECTOR3D_EQUAL(largeN, plane4.Normal);
}

void TestPlane::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);

    RUN_TEST(TestToString);

    RUN_TEST(TestEdgeCases);
}
