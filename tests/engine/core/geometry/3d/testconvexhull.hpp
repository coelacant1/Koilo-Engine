// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <unity.h>
#include <koilo/core/geometry/3d/convexhull.hpp>
#include <utils/testhelpers.hpp>

class TestConvexHull {
public:

    static void TestComputeBounds();

    static void RunAllTests() {
        RUN_TEST(TestEmptySupport);
        RUN_TEST(TestTetrahedronSupport);
        RUN_TEST(TestComputeBounds);
        RUN_TEST(TestAddVertexAndFace);
    }
};
