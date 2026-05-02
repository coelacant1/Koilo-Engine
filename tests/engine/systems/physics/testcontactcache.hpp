// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <unity.h>
#include <koilo/systems/physics/contact.hpp>
#include <koilo/systems/physics/contactmanifold.hpp>
#include <koilo/systems/physics/contactcache.hpp>
#include <utils/testhelpers.hpp>

class TestContactCache {
public:
    static void TestContactDefaults();
    static void TestManifoldAddBelowCap();
    static void TestManifoldMergeByFeatureIdPreservesImpulses();
    static void TestManifoldReplacesShallowestWhenFull();
    static void TestManifoldRejectsShallowerThanAll();
    static void TestManifoldDeepestIndex();
    static void TestManifoldToCollisionInfo();
    static void TestKeyCanonicalOrdering();
    static void TestCacheTouchInsertsAndWarmStarts();
    static void TestCacheEndFrameDropsStale();
    static void TestCacheWriteImpulses();
    static void TestCacheDeterministicIteration();
    static void RunAllTests();
};
