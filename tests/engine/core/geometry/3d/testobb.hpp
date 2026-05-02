// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <unity.h>
#include <koilo/core/geometry/3d/obb.hpp>
#include <utils/testhelpers.hpp>

class TestOBB {
public:

    static void RunAllTests() {
        RUN_TEST(TestEnclosingAabbAxisAligned);
        RUN_TEST(TestEnclosingAabbRotated);
        RUN_TEST(TestContainsPointAxisAligned);
        RUN_TEST(TestContainsPointRotated);
        RUN_TEST(TestClosestPointInside);
        RUN_TEST(TestClosestPointOutside);
        RUN_TEST(TestOverlapsOverlapping);
        RUN_TEST(TestOverlapsSeparated);
        RUN_TEST(TestOverlapsAabb);
        RUN_TEST(TestRaycastHit);
        RUN_TEST(TestRaycastMiss);
    }
};
