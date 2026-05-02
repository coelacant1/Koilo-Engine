// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <unity.h>
#include <koilo/systems/physics/soa_body_state.hpp>
#include <utils/testhelpers.hpp>

class TestSoaBodyState {
public:
    static void TestAddAndAccess();
    static void TestRemoveFreesSlot();
    static void TestHandleReuse();
    static void TestFlagsAndVelocity();

    static void RunAllTests() {
        RUN_TEST(TestAddAndAccess);
        RUN_TEST(TestRemoveFreesSlot);
        RUN_TEST(TestHandleReuse);
        RUN_TEST(TestFlagsAndVelocity);
    }
};
