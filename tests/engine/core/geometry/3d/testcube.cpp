// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcube.cpp
 * @brief Implementation of Cube geometry unit tests.
 */

#include "testcube.hpp"

using namespace koilo;

void TestCube::TestParameterizedConstructor() {
    Vector3D center(10.0f, 20.0f, 30.0f);
    Vector3D size(6.0f, 8.0f, 10.0f);
    Cube cube(center, size);

    TEST_ASSERT_VECTOR3D_EQUAL(center, cube.GetPosition());
    TEST_ASSERT_VECTOR3D_EQUAL(size, cube.GetSize());

    Vector3D expectedMin = center - size / 2.0f;
    Vector3D expectedMax = center + size / 2.0f;
    TEST_ASSERT_VECTOR3D_EQUAL(expectedMin, cube.GetMinimum());
    TEST_ASSERT_VECTOR3D_EQUAL(expectedMax, cube.GetMaximum());
}

void TestCube::TestGetPosition() {
    Cube cube(Vector3D(100.0f, 200.0f, 300.0f), Vector3D(10.0f, 20.0f, 30.0f));
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(100.0f, 200.0f, 300.0f), cube.GetPosition());
}

void TestCube::TestGetSize() {
    Cube cube(Vector3D(0, 0, 0), Vector3D(50.0f, 60.0f, 70.0f));
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(50.0f, 60.0f, 70.0f), cube.GetSize());
}

void TestCube::TestGetMaximum() {
    Cube cube(Vector3D(10, 10, 10), Vector3D(20, 20, 20));
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(20, 20, 20), cube.GetMaximum());
}

void TestCube::TestGetMinimum() {
    Cube cube(Vector3D(10, 10, 10), Vector3D(20, 20, 20));
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(0, 0, 0), cube.GetMinimum());
}

void TestCube::TestDefaultConstructor() {
    Cube cube(Vector3D(0, 0, 0), Vector3D(1, 1, 1));
    Vector3D pos = cube.GetPosition();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pos.X);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pos.Y);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pos.Z);
}

void TestCube::TestEdgeCases() {
    // Zero-sized cube
    Cube cube1(Vector3D(5, 5, 5), Vector3D(0, 0, 0));
    TEST_ASSERT_VECTOR3D_EQUAL(cube1.GetMinimum(), cube1.GetMaximum());

    // Negative-sized cube
    Cube cube2(Vector3D(5, 5, 5), Vector3D(-1, -1, -1));
    Vector3D size2 = cube2.GetSize();
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(-1, -1, -1), size2);
}

void TestCube::RunAllTests() {
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetPosition);
    RUN_TEST(TestGetSize);
    RUN_TEST(TestGetMaximum);
    RUN_TEST(TestGetMinimum);
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
}
