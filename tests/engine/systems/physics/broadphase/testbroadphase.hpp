// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <unity.h>

class TestBroadphase {
public:
    static void TestAddAssignsMonotonicProxyId();
    static void TestPlanesGoToPlaneRegistryNotTree();
    static void TestPairsSortedByProxyId();
    static void TestPlaneVsBoxPairProduced();
    static void TestRemoveDropsPair();
    static void TestUpdateRefreshesAabb();
    static void RunAllTests();
};
