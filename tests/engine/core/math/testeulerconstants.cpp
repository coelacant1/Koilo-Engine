// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testeulerconstants.cpp
 * @brief Implementation of EulerConstants source tests.
 */

#include "testeulerconstants.hpp"
#include <utils/testhelpers.hpp>

using namespace koilo;
using namespace koilo::EulerConstants;

void TestEulerConstants::TestStaticFrameConstants() {
    // Test EulerOrderXYZS
    const EulerOrder& xyzs = EulerOrderXYZS;
    TEST_ASSERT_EQUAL(EulerOrder::Axis::XYZ, xyzs.AxisOrder);
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Static, xyzs.FrameTaken);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, xyzs.Permutation.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 1.0f, xyzs.Permutation.Y);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 2.0f, xyzs.Permutation.Z);

    // Test EulerOrderXZYS
    const EulerOrder& xzys = EulerOrderXZYS;
    TEST_ASSERT_EQUAL(EulerOrder::Axis::XZY, xzys.AxisOrder);
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Static, xzys.FrameTaken);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, xzys.Permutation.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 2.0f, xzys.Permutation.Y);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 1.0f, xzys.Permutation.Z);

    // Test EulerOrderYXZS
    const EulerOrder& yxzs = EulerOrderYXZS;
    TEST_ASSERT_EQUAL(EulerOrder::Axis::YXZ, yxzs.AxisOrder);
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Static, yxzs.FrameTaken);

    // Test EulerOrderYZXS
    const EulerOrder& yzxs = EulerOrderYZXS;
    TEST_ASSERT_EQUAL(EulerOrder::Axis::YZX, yzxs.AxisOrder);
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Static, yzxs.FrameTaken);

    // Test EulerOrderZXYS
    const EulerOrder& zxys = EulerOrderZXYS;
    TEST_ASSERT_EQUAL(EulerOrder::Axis::ZXY, zxys.AxisOrder);
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Static, zxys.FrameTaken);

    // Test EulerOrderZYXS
    const EulerOrder& zyxs = EulerOrderZYXS;
    TEST_ASSERT_EQUAL(EulerOrder::Axis::ZYX, zyxs.AxisOrder);
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Static, zyxs.FrameTaken);
}

void TestEulerConstants::TestRotatingFrameConstants() {
    // Test EulerOrderZYXR
    const EulerOrder& zyxr = EulerOrderZYXR;
    TEST_ASSERT_EQUAL(EulerOrder::Axis::XYZ, zyxr.AxisOrder);
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Rotating, zyxr.FrameTaken);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 0.0f, zyxr.Permutation.X);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 1.0f, zyxr.Permutation.Y);
    TEST_ASSERT_FLOAT_WITHIN(TestHelpers::DEFAULT_TOLERANCE, 2.0f, zyxr.Permutation.Z);

    // Test EulerOrderYZXR
    const EulerOrder& yzxr = EulerOrderYZXR;
    TEST_ASSERT_EQUAL(EulerOrder::Axis::XZY, yzxr.AxisOrder);
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Rotating, yzxr.FrameTaken);

    // Test EulerOrderXZYR
    const EulerOrder& xzyr = EulerOrderXZYR;
    TEST_ASSERT_EQUAL(EulerOrder::Axis::YXZ, xzyr.AxisOrder);
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Rotating, xzyr.FrameTaken);

    // Test EulerOrderZXYR
    const EulerOrder& zxyr = EulerOrderZXYR;
    TEST_ASSERT_EQUAL(EulerOrder::Axis::YZX, zxyr.AxisOrder);
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Rotating, zxyr.FrameTaken);

    // Test EulerOrderYXZR
    const EulerOrder& yxzr = EulerOrderYXZR;
    TEST_ASSERT_EQUAL(EulerOrder::Axis::ZXY, yxzr.AxisOrder);
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Rotating, yxzr.FrameTaken);

    // Test EulerOrderXYZR
    const EulerOrder& xyzr = EulerOrderXYZR;
    TEST_ASSERT_EQUAL(EulerOrder::Axis::ZYX, xyzr.AxisOrder);
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Rotating, xyzr.FrameTaken);
}

void TestEulerConstants::TestEulerConstantsWrapper() {
    // Test wrapper getter methods
    EulerConstantsWrapper wrapper;

    // Static frame getters
    const EulerOrder& xyzs = EulerConstantsWrapper::GetXYZS();
    TEST_ASSERT_EQUAL(EulerOrder::Axis::XYZ, xyzs.AxisOrder);
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Static, xyzs.FrameTaken);

    const EulerOrder& xzys = EulerConstantsWrapper::GetXZYS();
    TEST_ASSERT_EQUAL(EulerOrder::Axis::XZY, xzys.AxisOrder);
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Static, xzys.FrameTaken);

    const EulerOrder& yzxs = EulerConstantsWrapper::GetYZXS();
    TEST_ASSERT_EQUAL(EulerOrder::Axis::YZX, yzxs.AxisOrder);

    const EulerOrder& yxzs = EulerConstantsWrapper::GetYXZS();
    TEST_ASSERT_EQUAL(EulerOrder::Axis::YXZ, yxzs.AxisOrder);

    const EulerOrder& zxys = EulerConstantsWrapper::GetZXYS();
    TEST_ASSERT_EQUAL(EulerOrder::Axis::ZXY, zxys.AxisOrder);

    const EulerOrder& zyxs = EulerConstantsWrapper::GetZYXS();
    TEST_ASSERT_EQUAL(EulerOrder::Axis::ZYX, zyxs.AxisOrder);

    // Rotating frame getters
    const EulerOrder& zyxr = EulerConstantsWrapper::GetZYXR();
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Rotating, zyxr.FrameTaken);

    const EulerOrder& yzxr = EulerConstantsWrapper::GetYZXR();
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Rotating, yzxr.FrameTaken);

    const EulerOrder& xzyr = EulerConstantsWrapper::GetXZYR();
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Rotating, xzyr.FrameTaken);

    const EulerOrder& zxyr = EulerConstantsWrapper::GetZXYR();
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Rotating, zxyr.FrameTaken);

    const EulerOrder& yxzr = EulerConstantsWrapper::GetYXZR();
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Rotating, yxzr.FrameTaken);

    const EulerOrder& xyzr = EulerConstantsWrapper::GetXYZR();
    TEST_ASSERT_EQUAL(EulerOrder::AxisFrame::Rotating, xyzr.FrameTaken);
}

void TestEulerConstants::TestEdgeCases() {
    // Test that all constants are distinct
    const EulerOrder& xyzs = EulerOrderXYZS;
    const EulerOrder& xzys = EulerOrderXZYS;
    
    // Different axis orders
    TEST_ASSERT_NOT_EQUAL(xyzs.AxisOrder, xzys.AxisOrder);

    // Test that static and rotating variants are different
    const EulerOrder& xyzsStatic = EulerOrderXYZS;
    const EulerOrder& xyzsRotating = EulerOrderZYXR;  // XYZ in rotating frame
    
    TEST_ASSERT_NOT_EQUAL(xyzsStatic.FrameTaken, xyzsRotating.FrameTaken);

    // Test that permutation is properly set
    const EulerOrder& order = EulerOrderXYZS;
    // Permutation should be in range [0, 2]
    TEST_ASSERT_TRUE(order.Permutation.X >= 0.0f && order.Permutation.X <= 2.0f);
    TEST_ASSERT_TRUE(order.Permutation.Y >= 0.0f && order.Permutation.Y <= 2.0f);
    TEST_ASSERT_TRUE(order.Permutation.Z >= 0.0f && order.Permutation.Z <= 2.0f);
}

void TestEulerConstants::RunAllTests() {
    RUN_TEST(TestStaticFrameConstants);
    RUN_TEST(TestRotatingFrameConstants);
    RUN_TEST(TestEulerConstantsWrapper);
    RUN_TEST(TestEdgeCases);
}
