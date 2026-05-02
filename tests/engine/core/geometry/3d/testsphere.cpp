// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsphere.cpp
 * @brief Implementation of Sphere geometry unit tests.
 */

#include "testsphere.hpp"

using namespace koilo;

void TestSphere::TestParameterizedConstructor() {
    Vector3D position(10.0f, 20.0f, 30.0f);
    float radius = 5.0f;
    Sphere sphere(position, radius);

    TEST_ASSERT_VECTOR3D_EQUAL(position, sphere.position);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, radius, sphere.GetRadius());
}

void TestSphere::TestGetRadius() {
    Sphere sphere(Vector3D(0, 0, 0), 42.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 42.5f, sphere.GetRadius());
}

void TestSphere::TestDefaultConstructor() {
    Sphere sphere(Vector3D(0, 0, 0), 1.0f);
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(0, 0, 0), sphere.position);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, sphere.GetRadius());
}

void TestSphere::TestEdgeCases() {
    Sphere tinySphere(Vector3D(0, 0, 0), 0.001f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.001f, tinySphere.GetRadius());

    Sphere largeSphere(Vector3D(0, 0, 0), 1000.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1000.0f, largeSphere.GetRadius());

    Sphere negSphere(Vector3D(-10.0f, -20.0f, -30.0f), 5.0f);
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(-10.0f, -20.0f, -30.0f), negSphere.position);
}

void TestSphere::TestCollide() {
    // TODO: Implement test for Collide()
    Sphere obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestSphere::TestIsIntersecting() {
    // TODO: Implement test for IsIntersecting()
    Sphere obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestSphere::TestUpdate() {
    // TODO: Implement test for Update()
    Sphere obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestSphere::RunAllTests() {
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetRadius);
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestCollide);
    RUN_TEST(TestIsIntersecting);
    RUN_TEST(TestUpdate);
}
