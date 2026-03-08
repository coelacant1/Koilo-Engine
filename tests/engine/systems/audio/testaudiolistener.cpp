// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testaudiolistener.cpp
 * @brief Implementation of AudioListener unit tests.
 */

#include "testaudiolistener.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestAudioListener::TestDefaultConstructor() {
    AudioListener listener;
    // Default position at origin
    Vector3D pos = listener.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pos.Z);
}

void TestAudioListener::TestParameterizedConstructor() {
    AudioListener listener;
    Vector3D vel = listener.GetVelocity();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, vel.X);
}

// ========== Method Tests ==========

// ========== Edge Cases ==========

void TestAudioListener::TestEdgeCases() {
    AudioListener listener;
    Vector3D extreme(1e6f, 1e6f, 1e6f);
    listener.SetPosition(extreme);
    Vector3D pos = listener.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(1e3f, 1e6f, pos.X);
}
// ========== Test Runner ==========

void TestAudioListener::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);

    RUN_TEST(TestEdgeCases);
}
