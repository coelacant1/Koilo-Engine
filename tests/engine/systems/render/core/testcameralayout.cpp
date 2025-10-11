/**
 * @file testcameralayout.cpp
 * @brief Implementation of CameraLayout unit tests.
 */

#include "testcameralayout.hpp"

// ========== Constructor Tests ==========

void TestCameraLayout::TestDefaultConstructor() {
    // CameraLayout requires parameters - test with basic forward/up axes
    CameraLayout layout(CameraLayout::ZForward, CameraLayout::YUp);

    // Verify axes were set correctly
    TEST_ASSERT_EQUAL(CameraLayout::ZForward, layout.GetForwardAxis());
    TEST_ASSERT_EQUAL(CameraLayout::YUp, layout.GetUpAxis());
}

// ========== Method Tests ==========
void TestCameraLayout::TestGetForwardAxis() {
    CameraLayout layout(CameraLayout::XForward, CameraLayout::ZUp);

    CameraLayout::ForwardAxis axis = layout.GetForwardAxis();
    TEST_ASSERT_EQUAL(CameraLayout::XForward, axis);
}

void TestCameraLayout::TestGetUpAxis() {
    CameraLayout layout(CameraLayout::ZForward, CameraLayout::YUp);

    CameraLayout::UpAxis axis = layout.GetUpAxis();
    TEST_ASSERT_EQUAL(CameraLayout::YUp, axis);
}

void TestCameraLayout::TestGetForwardVector() {
    // Test positive Z forward
    CameraLayout layoutZ(CameraLayout::ZForward, CameraLayout::YUp);
    Vector3D forwardZ = layoutZ.GetForwardVector();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, forwardZ.X);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, forwardZ.Y);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, forwardZ.Z);

    // Test negative X forward
    CameraLayout layoutXN(CameraLayout::XNForward, CameraLayout::YUp);
    Vector3D forwardXN = layoutXN.GetForwardVector();
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, forwardXN.X);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, forwardXN.Y);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, forwardXN.Z);
}

void TestCameraLayout::TestGetUpVector() {
    // Test positive Y up
    CameraLayout layoutY(CameraLayout::ZForward, CameraLayout::YUp);
    Vector3D upY = layoutY.GetUpVector();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, upY.X);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, upY.Y);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, upY.Z);

    // Test negative Z up
    CameraLayout layoutZN(CameraLayout::XForward, CameraLayout::ZNUp);
    Vector3D upZN = layoutZN.GetUpVector();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, upZN.X);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, upZN.Y);
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, upZN.Z);
}

void TestCameraLayout::TestGetRotation() {
    CameraLayout layout(CameraLayout::ZForward, CameraLayout::YUp);

    Quaternion rotation = layout.GetRotation();
    // Verify rotation is a valid quaternion (magnitude ~= 1 for unit quaternion)
    float magnitude = rotation.Magnitude();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, magnitude);
}

// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestCameraLayout::TestParameterizedConstructor() {
    // Test various axis combinations
    CameraLayout layout1(CameraLayout::XForward, CameraLayout::YUp);
    TEST_ASSERT_EQUAL(CameraLayout::XForward, layout1.GetForwardAxis());
    TEST_ASSERT_EQUAL(CameraLayout::YUp, layout1.GetUpAxis());

    CameraLayout layout2(CameraLayout::YNForward, CameraLayout::ZUp);
    TEST_ASSERT_EQUAL(CameraLayout::YNForward, layout2.GetForwardAxis());
    TEST_ASSERT_EQUAL(CameraLayout::ZUp, layout2.GetUpAxis());
}

void TestCameraLayout::TestEdgeCases() {
    // Test all forward axis variants
    CameraLayout layoutXF(CameraLayout::XForward, CameraLayout::YUp);
    Vector3D forwardXF = layoutXF.GetForwardVector();
    TEST_ASSERT_EQUAL_FLOAT(1.0f, forwardXF.X);

    CameraLayout layoutYF(CameraLayout::YForward, CameraLayout::ZUp);
    Vector3D forwardYF = layoutYF.GetForwardVector();
    TEST_ASSERT_EQUAL_FLOAT(1.0f, forwardYF.Y);

    CameraLayout layoutZF(CameraLayout::ZForward, CameraLayout::XUp);
    Vector3D forwardZF = layoutZF.GetForwardVector();
    TEST_ASSERT_EQUAL_FLOAT(1.0f, forwardZF.Z);

    // Test negative axes
    CameraLayout layoutXNF(CameraLayout::XNForward, CameraLayout::YUp);
    Vector3D forwardXNF = layoutXNF.GetForwardVector();
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, forwardXNF.X);

    CameraLayout layoutYNF(CameraLayout::YNForward, CameraLayout::ZUp);
    Vector3D forwardYNF = layoutYNF.GetForwardVector();
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, forwardYNF.Y);

    CameraLayout layoutZNF(CameraLayout::ZNForward, CameraLayout::YUp);
    Vector3D forwardZNF = layoutZNF.GetForwardVector();
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, forwardZNF.Z);
}

void TestCameraLayout::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetForwardAxis);
    RUN_TEST(TestGetUpAxis);
    RUN_TEST(TestGetForwardVector);
    RUN_TEST(TestGetUpVector);
    RUN_TEST(TestGetRotation);
    RUN_TEST(TestEdgeCases);
}
