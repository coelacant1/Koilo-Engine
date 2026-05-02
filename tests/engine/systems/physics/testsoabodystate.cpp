// SPDX-License-Identifier: GPL-3.0-or-later
#include "testsoabodystate.hpp"

using namespace koilo;

void TestSoaBodyState::TestAddAndAccess() {
    SoaBodyState s;
    auto h = s.Add(Vector3D(1,2,3), Quaternion(), 0.5f, Matrix3x3::Identity());
    TEST_ASSERT_EQUAL(1u, s.Size());
    TEST_ASSERT_TRUE(s.IsAlive(h));
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 1.0, s.Position(h).X);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 0.5, s.InverseMass(h));
}

void TestSoaBodyState::TestRemoveFreesSlot() {
    SoaBodyState s;
    auto h = s.Add(Vector3D(0,0,0), Quaternion(), 1.0f, Matrix3x3::Identity());
    s.Remove(h);
    TEST_ASSERT_FALSE(s.IsAlive(h));
    TEST_ASSERT_EQUAL(0u, s.Size());
    TEST_ASSERT_EQUAL(1u, s.Capacity()); // slot held for reuse
}

void TestSoaBodyState::TestHandleReuse() {
    SoaBodyState s;
    auto h0 = s.Add(Vector3D(1,0,0), Quaternion(), 1.0f, Matrix3x3::Identity());
    auto h1 = s.Add(Vector3D(2,0,0), Quaternion(), 1.0f, Matrix3x3::Identity());
    s.Remove(h0);
    auto h2 = s.Add(Vector3D(3,0,0), Quaternion(), 1.0f, Matrix3x3::Identity());
    TEST_ASSERT_EQUAL(h0, h2);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 3.0, s.Position(h2).X);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 2.0, s.Position(h1).X);
}

void TestSoaBodyState::TestFlagsAndVelocity() {
    SoaBodyState s;
    auto h = s.Add(Vector3D(0,0,0), Quaternion(), 1.0f, Matrix3x3::Identity(),
                   SoaBodyState::FlagBullet);
    s.LinearVelocity(h) = Vector3D(5,0,0);
    s.AngularVelocity(h) = Vector3D(0,1,0);
    TEST_ASSERT_EQUAL(SoaBodyState::FlagBullet, s.Flags(h));
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 5.0, s.LinearVelocity(h).X);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 1.0, s.AngularVelocity(h).Y);
}
