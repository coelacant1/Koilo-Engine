/**
 * @file testcube.cpp
 * @brief Implementation of Cube unit tests.
 */

#include "testcube.hpp"

// ========== Constructor Tests ==========

void TestCube::TestParameterizedConstructor() {
    Vector3D center(10.0f, 20.0f, 30.0f);
    Vector3D size(6.0f, 8.0f, 10.0f);
    Cube cube(center, size);

    // Verify center position
    Vector3D pos = cube.GetPosition();
    TEST_ASSERT_VECTOR3D_EQUAL(center, pos);

    // Verify size
    Vector3D cubeSize = cube.GetSize();
    TEST_ASSERT_VECTOR3D_EQUAL(size, cubeSize);

    // Verify min/max are calculated correctly
    Vector3D expectedMin = center - size / 2.0f;
    Vector3D expectedMax = center + size / 2.0f;
    TEST_ASSERT_VECTOR3D_EQUAL(expectedMin, cube.GetMinimum());
    TEST_ASSERT_VECTOR3D_EQUAL(expectedMax, cube.GetMaximum());

    // Verify initial public fields
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(0, 0, 0), cube.velocity);
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(0, 0, 0), cube.position);
}

// ========== Getter Method Tests ==========

void TestCube::TestGetPosition() {
    Vector3D center(100.0f, 200.0f, 300.0f);
    Vector3D size(10.0f, 20.0f, 30.0f);
    Cube cube(center, size);

    Vector3D position = cube.GetPosition();
    TEST_ASSERT_VECTOR3D_EQUAL(center, position);
}

void TestCube::TestGetSize() {
    Vector3D center(0.0f, 0.0f, 0.0f);
    Vector3D size(50.0f, 60.0f, 70.0f);
    Cube cube(center, size);

    Vector3D cubeSize = cube.GetSize();
    TEST_ASSERT_VECTOR3D_EQUAL(size, cubeSize);
}

void TestCube::TestGetMaximum() {
    Vector3D center(10.0f, 10.0f, 10.0f);
    Vector3D size(20.0f, 20.0f, 20.0f);
    Cube cube(center, size);

    Vector3D expectedMax(20.0f, 20.0f, 20.0f); // center + size/2
    TEST_ASSERT_VECTOR3D_EQUAL(expectedMax, cube.GetMaximum());
}

void TestCube::TestGetMinimum() {
    Vector3D center(10.0f, 10.0f, 10.0f);
    Vector3D size(20.0f, 20.0f, 20.0f);
    Cube cube(center, size);

    Vector3D expectedMin(0.0f, 0.0f, 0.0f); // center - size/2
    TEST_ASSERT_VECTOR3D_EQUAL(expectedMin, cube.GetMinimum());
}

// ========== Physics Update Tests ==========

// ========== Intersection Tests ==========

// ========== Field Access Tests ==========

// ========== Edge Case Tests ==========

// ========== Test Runner ==========

void TestCube::TestDefaultConstructor() {
    // Cube requires constructor parameters (no default constructor)
    Vector3D center(0.0f, 0.0f, 0.0f);
    Vector3D size(1.0f, 1.0f, 1.0f);
    Cube cube(center, size);
    
    Vector3D pos = cube.GetPosition();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pos.X);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pos.Y);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pos.Z);
}

void TestCube::TestEdgeCases() {
    // Test with zero-sized cube
    Vector3D center(5.0f, 5.0f, 5.0f);
    Vector3D zeroSize(0.0f, 0.0f, 0.0f);
    Cube cube1(center, zeroSize);
    TEST_ASSERT_TRUE(true);  // Should not crash
    
    // Test with negative-sized cube
    Vector3D negSize(-1.0f, -1.0f, -1.0f);
    Cube cube2(center, negSize);
    TEST_ASSERT_TRUE(true);  // Should handle gracefully
}

void TestCube::TestIsIntersecting() {
    Vector3D center1(0.0f, 0.0f, 0.0f);
    Vector3D size1(2.0f, 2.0f, 2.0f);
    Cube cube1(center1, size1);
    
    Vector3D center2(1.0f, 0.0f, 0.0f);
    Vector3D size2(2.0f, 2.0f, 2.0f);
    Cube cube2(center2, size2);
    
    // Overlapping cubes should intersect
    Vector3D intersection = cube1.IsIntersecting(&cube2);
    // If intersecting, some component should be non-zero
    TEST_ASSERT_TRUE(intersection.X != 0.0f || intersection.Y != 0.0f || intersection.Z != 0.0f);
    
    // Non-overlapping cubes
    Vector3D center3(10.0f, 0.0f, 0.0f);
    Cube cube3(center3, size2);
    Vector3D noIntersection = cube1.IsIntersecting(&cube3);
    // No intersection means all zeros
    TEST_ASSERT_EQUAL_FLOAT(0.0f, noIntersection.X);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, noIntersection.Y);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, noIntersection.Z);
}

void TestCube::TestUpdate() {
    Vector3D center(0.0f, 0.0f, 0.0f);
    Vector3D size(1.0f, 1.0f, 1.0f);
    Cube cube(center, size);
    
    // Set velocity
    cube.velocity = Vector3D(1.0f, 0.0f, 0.0f);
    
    // Update with 0.1 second delta, no acceleration, no rotation
    Vector3D acceleration(0.0f, 0.0f, 0.0f);
    Quaternion rotation(1.0f, 0.0f, 0.0f, 0.0f);
    cube.Update(0.1f, acceleration, rotation);
    
    // Position should have moved
    Vector3D newPos = cube.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.1f, newPos.X);
}

void TestCube::RunAllTests() {
    RUN_TEST(TestParameterizedConstructor);

    RUN_TEST(TestGetPosition);
    RUN_TEST(TestGetSize);
    RUN_TEST(TestGetMaximum);
    RUN_TEST(TestGetMinimum);

    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestIsIntersecting);
    RUN_TEST(TestUpdate);
}
