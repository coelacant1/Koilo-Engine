// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <unity.h>
#include <koilo/core/math/sim/particlepool.hpp>
#include <koilo/core/math/sim/constraintgraph.hpp>
#include <utils/testhelpers.hpp>

class TestParticlePool {
public:
    static void TestAddIncrementsSize();
    static void TestRemoveFreesSlot();
    static void TestHandleStability();
    static void TestForEachVisitsLive();
    static void TestConstraintGraphAdd();

    static void RunAllTests() {
        RUN_TEST(TestAddIncrementsSize);
        RUN_TEST(TestRemoveFreesSlot);
        RUN_TEST(TestHandleStability);
        RUN_TEST(TestForEachVisitsLive);
        RUN_TEST(TestConstraintGraphAdd);
    }
};
