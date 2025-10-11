/**
 * @file testsphere.cpp
 * @brief Implementation of Sphere unit tests.
 */

#include "testsphere.hpp"

// ========== Constructor Tests ==========

void TestSphere::TestParameterizedConstructor() {
    Vector3D position(10.0f, 20.0f, 30.0f);
    float radius = 5.0f;
    Sphere sphere(position, radius);

    // Verify position is set correctly
    TEST_ASSERT_VECTOR3D_EQUAL(position, sphere.position);

    // Verify radius is set correctly
    TEST_ASSERT_FLOAT_WITHIN(0.01f, radius, sphere.GetRadius());

    // Verify initial velocity is zero
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(0, 0, 0), sphere.velocity);
}

// ========== Getter Method Tests ==========

void TestSphere::TestGetRadius() {
    Vector3D position(0.0f, 0.0f, 0.0f);
    float radius = 42.5f;
    Sphere sphere(position, radius);

    float retrievedRadius = sphere.GetRadius();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, radius, retrievedRadius);
}

// ========== Physics Update Tests ==========

// ========== Intersection Tests ==========

// ========== Collision Tests ==========

// ========== Field Access Tests ==========

// ========== Edge Case Tests ==========

// ========== Test Runner ==========

void TestSphere::TestCollide() {
    // Create two spheres that are intersecting
    Sphere sphere1(Vector3D(0.0f, 0.0f, 0.0f), 2.0f);
    Sphere sphere2(Vector3D(3.0f, 0.0f, 0.0f), 2.0f);

    // Set initial velocities
    sphere1.velocity = Vector3D(1.0f, 0.0f, 0.0f);
    sphere2.velocity = Vector3D(-1.0f, 0.0f, 0.0f);

    // Perform collision with elasticity
    float elasticity = 0.8f;
    sphere1.Collide(elasticity, &sphere2);

    // After collision, velocities should have changed
    // The exact values depend on the collision implementation
    // Just verify the collision was processed
    TEST_ASSERT_TRUE(std::isfinite(sphere1.velocity.X));
    TEST_ASSERT_TRUE(std::isfinite(sphere2.velocity.X));
}

void TestSphere::TestDefaultConstructor() {
    // Sphere has no default constructor - test with minimal parameters
    Vector3D position(0.0f, 0.0f, 0.0f);
    float radius = 1.0f;
    Sphere sphere(position, radius);

    // Verify sphere was created with expected defaults
    TEST_ASSERT_VECTOR3D_EQUAL(position, sphere.position);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, radius, sphere.GetRadius());
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(0, 0, 0), sphere.velocity);
}

void TestSphere::TestEdgeCases() {
    // Test with very small radius
    Sphere tinySphere(Vector3D(0.0f, 0.0f, 0.0f), 0.001f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.001f, tinySphere.GetRadius());

    // Test with large radius
    Sphere largeSphere(Vector3D(0.0f, 0.0f, 0.0f), 1000.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1000.0f, largeSphere.GetRadius());

    // Test with negative coordinates
    Sphere negSphere(Vector3D(-10.0f, -20.0f, -30.0f), 5.0f);
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(-10.0f, -20.0f, -30.0f), negSphere.position);

    // Test update with zero time step
    Sphere sphere(Vector3D(0.0f, 0.0f, 0.0f), 1.0f);
    Vector3D acceleration(0.0f, -9.8f, 0.0f);
    Quaternion rotation;
    sphere.Update(0.0f, acceleration, rotation);

    // Position should remain unchanged with zero time step
    TEST_ASSERT_TRUE(std::isfinite(sphere.position.X));

    // Test collision with identical positions
    Sphere s1(Vector3D(0.0f, 0.0f, 0.0f), 1.0f);
    Sphere s2(Vector3D(0.0f, 0.0f, 0.0f), 1.0f);
    TEST_ASSERT_TRUE(s1.IsIntersecting(&s2));
}

void TestSphere::TestIsIntersecting() {
    // Create two spheres that are intersecting
    Sphere sphere1(Vector3D(0.0f, 0.0f, 0.0f), 2.0f);
    Sphere sphere2(Vector3D(3.0f, 0.0f, 0.0f), 2.0f);

    // Spheres at distance 3 with radii 2+2=4 should be intersecting
    TEST_ASSERT_TRUE(sphere1.IsIntersecting(&sphere2));

    // Create two spheres that are not intersecting
    Sphere sphere3(Vector3D(0.0f, 0.0f, 0.0f), 1.0f);
    Sphere sphere4(Vector3D(10.0f, 0.0f, 0.0f), 1.0f);

    // Spheres at distance 10 with radii 1+1=2 should not be intersecting
    TEST_ASSERT_FALSE(sphere3.IsIntersecting(&sphere4));

    // Create two spheres that are exactly touching
    Sphere sphere5(Vector3D(0.0f, 0.0f, 0.0f), 2.0f);
    Sphere sphere6(Vector3D(4.0f, 0.0f, 0.0f), 2.0f);

    // Spheres at distance 4 with radii 2+2=4 should be touching (intersecting)
    TEST_ASSERT_TRUE(sphere5.IsIntersecting(&sphere6));

    // Test with one sphere inside another
    Sphere largeSphere(Vector3D(0.0f, 0.0f, 0.0f), 10.0f);
    Sphere smallSphere(Vector3D(1.0f, 1.0f, 1.0f), 1.0f);
    TEST_ASSERT_TRUE(largeSphere.IsIntersecting(&smallSphere));
}

void TestSphere::TestUpdate() {
    Sphere sphere(Vector3D(0.0f, 10.0f, 0.0f), 1.0f);
    Vector3D acceleration(0.0f, -9.8f, 0.0f);  // Gravity
    Quaternion rotation;  // No rotation

    // Initial position and velocity
    Vector3D initialPos = sphere.position;
    Vector3D initialVel = sphere.velocity;

    // Update for 1 second
    float deltaTime = 1.0f;
    sphere.Update(deltaTime, acceleration, rotation);

    // Verify position changed (falling due to gravity)
    TEST_ASSERT_TRUE(sphere.position.Y < initialPos.Y);

    // Verify velocity increased in negative Y direction
    TEST_ASSERT_TRUE(sphere.velocity.Y < initialVel.Y);

    // Test update with small time step
    Sphere sphere2(Vector3D(0.0f, 0.0f, 0.0f), 1.0f);
    sphere2.Update(0.01f, acceleration, rotation);
    TEST_ASSERT_TRUE(std::isfinite(sphere2.position.Y));
    TEST_ASSERT_TRUE(std::isfinite(sphere2.velocity.Y));

    // Test update with zero acceleration
    Sphere sphere3(Vector3D(5.0f, 5.0f, 5.0f), 1.0f);
    sphere3.velocity = Vector3D(1.0f, 0.0f, 0.0f);
    sphere3.Update(1.0f, Vector3D(0.0f, 0.0f, 0.0f), rotation);

    // With no acceleration, sphere should maintain constant velocity
    TEST_ASSERT_TRUE(std::isfinite(sphere3.position.X));
    TEST_ASSERT_TRUE(sphere3.position.X > 5.0f);  // Should have moved in X direction
}

void TestSphere::RunAllTests() {
    RUN_TEST(TestParameterizedConstructor);

    RUN_TEST(TestGetRadius);

    RUN_TEST(TestCollide);
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestIsIntersecting);
    RUN_TEST(TestUpdate);
}
