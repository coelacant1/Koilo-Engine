// SPDX-License-Identifier: GPL-3.0-or-later
#include "testconvexhull.hpp"

using namespace koilo;

);
    TEST_ASSERT_EQUAL(4u, h.VertexCount());

    Vector3D s = h.Support(Vector3D(1, 0, 0));
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 1.0, s.X);

    s = h.Support(Vector3D(-1, 0, 0));
    TEST_ASSERT_FLOAT_WITHIN(1e-5, -1.0, s.X);

    s = h.Support(Vector3D(0, 1, 0));
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 1.0, s.Y);

    s = h.Support(Vector3D(0, 0, 1));
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 1.0, s.Z);
}

void TestConvexHull::TestComputeBounds() {
    ConvexHull h({
        Vector3D(-2, -3, -4),
        Vector3D( 5,  6,  7),
        Vector3D( 0,  0,  0)
    });
    AABB a = h.ComputeBounds();
    TEST_ASSERT_FLOAT_WITHIN(1e-5, -2.0, a.min.X);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, -3.0, a.min.Y);
    TEST_ASSERT_FLOAT_WITHIN(1e-5,  7.0, a.max.Z);
}

