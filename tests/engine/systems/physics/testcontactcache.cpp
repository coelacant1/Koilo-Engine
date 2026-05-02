// SPDX-License-Identifier: GPL-3.0-or-later
#include "testcontactcache.hpp"

using namespace koilo;

void TestContactCache::TestContactDefaults() {
    Contact c;
    TEST_ASSERT_EQUAL_FLOAT(0.0f, c.depth);
    TEST_ASSERT_EQUAL_UINT64(0, c.featureId);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, c.accumulatedNormalImpulse);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, c.accumulatedTangentImpulse[0]);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, c.accumulatedTangentImpulse[1]);
}

void TestContactCache::TestManifoldAddBelowCap() {
    ContactManifold m;
    int i0 = m.AddContact(Contact(Vector3D(0,0,0), Vector3D(0,1,0), 0.1f, 1));
    int i1 = m.AddContact(Contact(Vector3D(1,0,0), Vector3D(0,1,0), 0.2f, 2));
    TEST_ASSERT_EQUAL_INT(0, i0);
    TEST_ASSERT_EQUAL_INT(1, i1);
    TEST_ASSERT_EQUAL_UINT8(2, m.count);
}

void TestContactCache::TestManifoldMergeByFeatureIdPreservesImpulses() {
    ContactManifold m;
    m.AddContact(Contact(Vector3D(0,0,0), Vector3D(0,1,0), 0.1f, 42));
    // Solver writes impulses.
    m.contacts[0].accumulatedNormalImpulse = 5.0f;
    m.contacts[0].accumulatedTangentImpulse[0] = 1.0f;
    m.contacts[0].accumulatedTangentImpulse[1] = 2.0f;
    // Re-adding the same featureId merges in place - impulses must survive.
    int idx = m.AddContact(Contact(Vector3D(9,9,9), Vector3D(1,0,0), 0.5f, 42));
    TEST_ASSERT_EQUAL_INT(0, idx);
    TEST_ASSERT_EQUAL_UINT8(1, m.count);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, m.contacts[0].accumulatedNormalImpulse);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, m.contacts[0].accumulatedTangentImpulse[0]);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, m.contacts[0].accumulatedTangentImpulse[1]);
    // But geometry is updated.
    TEST_ASSERT_EQUAL_FLOAT(0.5f, m.contacts[0].depth);
    TEST_ASSERT_EQUAL_FLOAT(9.0f, m.contacts[0].point.X);
}

void TestContactCache::TestManifoldReplacesShallowestWhenFull() {
    ContactManifold m;
    m.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.5f, 1));
    m.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.1f, 2)); // shallowest
    m.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.4f, 3));
    m.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.3f, 4));
    TEST_ASSERT_EQUAL_UINT8(4, m.count);
    int idx = m.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.9f, 5));
    TEST_ASSERT_EQUAL_INT(1, idx); // replaced index 1 (depth 0.1)
    TEST_ASSERT_EQUAL_UINT64(5, m.contacts[1].featureId);
    TEST_ASSERT_EQUAL_UINT8(4, m.count);
}

void TestContactCache::TestManifoldRejectsShallowerThanAll() {
    ContactManifold m;
    for (int i=0;i<4;++i)
        m.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.5f + 0.1f*i, 100+i));
    int idx = m.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.01f, 999));
    TEST_ASSERT_EQUAL_INT(-1, idx);
    TEST_ASSERT_EQUAL_UINT8(4, m.count);
}

void TestContactCache::TestManifoldDeepestIndex() {
    ContactManifold m;
    TEST_ASSERT_EQUAL_INT(-1, m.DeepestIndex());
    m.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.2f, 1));
    m.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.7f, 2));
    m.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.4f, 3));
    TEST_ASSERT_EQUAL_INT(1, m.DeepestIndex());
}

void TestContactCache::TestManifoldToCollisionInfo() {
    ContactManifold m;
    m.AddContact(Contact(Vector3D(1,2,3), Vector3D(0,1,0), 0.25f, 7));
    CollisionInfo info = m.ToCollisionInfo();
    TEST_ASSERT_EQUAL_FLOAT(1.0f, info.contactPoint.X);
    TEST_ASSERT_EQUAL_FLOAT(0.25f, info.penetrationDepth);
    TEST_ASSERT_NULL(info.colliderA);
}

void TestContactCache::TestKeyCanonicalOrdering() {
    ColliderProxy p1, p2;
    ContactKey k1 = ContactKey::Make(&p1, &p2, 5);
    ContactKey k2 = ContactKey::Make(&p2, &p1, 5);
    TEST_ASSERT_TRUE(k1 == k2);
}

void TestContactCache::TestCacheTouchInsertsAndWarmStarts() {
    ColliderProxy pa, pb;
    ContactCache cache;

    ContactManifold m1; m1.a = &pa; m1.b = &pb;
    m1.AddContact(Contact(Vector3D(0,0,0), Vector3D(0,1,0), 0.1f, 99));
    cache.BeginFrame();
    cache.Touch(m1);
    cache.EndFrame();
    TEST_ASSERT_EQUAL_size_t(1, cache.Size());

    // Solver writes impulses for that featureId.
    cache.WriteImpulses(&pa, &pb, 99, 4.0f, 0.5f, -0.25f);

    // Next frame: a fresh manifold for the same pair should be warm-started.
    ContactManifold m2; m2.a = &pa; m2.b = &pb;
    m2.AddContact(Contact(Vector3D(0.1f,0,0), Vector3D(0,1,0), 0.15f, 99));
    cache.BeginFrame();
    cache.Touch(m2);
    cache.EndFrame();
    TEST_ASSERT_EQUAL_FLOAT(4.0f,  m2.contacts[0].accumulatedNormalImpulse);
    TEST_ASSERT_EQUAL_FLOAT(0.5f,  m2.contacts[0].accumulatedTangentImpulse[0]);
    TEST_ASSERT_EQUAL_FLOAT(-0.25f, m2.contacts[0].accumulatedTangentImpulse[1]);
    TEST_ASSERT_EQUAL_FLOAT(0.15f, m2.contacts[0].depth); // geometry updated
}

void TestContactCache::TestCacheEndFrameDropsStale() {
    ColliderProxy pa, pb;
    ContactCache cache;
    ContactManifold m; m.a = &pa; m.b = &pb;
    m.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.1f, 1));
    m.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.1f, 2));
    cache.BeginFrame(); cache.Touch(m); cache.EndFrame();
    TEST_ASSERT_EQUAL_size_t(2, cache.Size());

    // Next frame: only feature 1 still exists.
    ContactManifold m2; m2.a = &pa; m2.b = &pb;
    m2.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.1f, 1));
    cache.BeginFrame(); cache.Touch(m2); cache.EndFrame();
    TEST_ASSERT_EQUAL_size_t(1, cache.Size());
    TEST_ASSERT_NOT_NULL(cache.Find(&pa, &pb, 1));
    TEST_ASSERT_NULL(cache.Find(&pa, &pb, 2));
}

void TestContactCache::TestCacheWriteImpulses() {
    ColliderProxy pa, pb;
    ContactCache cache;
    ContactManifold m; m.a = &pa; m.b = &pb;
    m.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.1f, 7));
    cache.BeginFrame(); cache.Touch(m); cache.EndFrame();

    cache.WriteImpulses(&pa, &pb, 7, 3.5f, 0.0f, 0.0f);
    auto* e = cache.Find(&pa, &pb, 7);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_FLOAT(3.5f, e->contact.accumulatedNormalImpulse);

    // Write to non-existent key is a no-op (no crash).
    cache.WriteImpulses(&pa, &pb, 12345, 9.0f, 0, 0);
    TEST_ASSERT_NULL(cache.Find(&pa, &pb, 12345));
}

void TestContactCache::TestCacheDeterministicIteration() {
    ColliderProxy pa, pb, pc;
    ContactCache cache;
    ContactManifold m1; m1.a = &pa; m1.b = &pb;
    m1.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.1f, 3));
    m1.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.1f, 1));
    ContactManifold m2; m2.a = &pa; m2.b = &pc;
    m2.AddContact(Contact(Vector3D(), Vector3D(0,1,0), 0.1f, 2));
    cache.BeginFrame(); cache.Touch(m1); cache.Touch(m2); cache.EndFrame();

    // Iteration must yield ascending keys (deterministic).
    auto prev = cache.begin();
    auto it = prev;
    if (it != cache.end()) ++it;
    while (it != cache.end()) {
        TEST_ASSERT_TRUE(prev->first < it->first);
        prev = it; ++it;
    }
    TEST_ASSERT_EQUAL_size_t(3, cache.Size());
}

void TestContactCache::RunAllTests() {
    RUN_TEST(TestContactDefaults);
    RUN_TEST(TestManifoldAddBelowCap);
    RUN_TEST(TestManifoldMergeByFeatureIdPreservesImpulses);
    RUN_TEST(TestManifoldReplacesShallowestWhenFull);
    RUN_TEST(TestManifoldRejectsShallowerThanAll);
    RUN_TEST(TestManifoldDeepestIndex);
    RUN_TEST(TestManifoldToCollisionInfo);
    RUN_TEST(TestKeyCanonicalOrdering);
    RUN_TEST(TestCacheTouchInsertsAndWarmStarts);
    RUN_TEST(TestCacheEndFrameDropsStale);
    RUN_TEST(TestCacheWriteImpulses);
    RUN_TEST(TestCacheDeterministicIteration);
}
