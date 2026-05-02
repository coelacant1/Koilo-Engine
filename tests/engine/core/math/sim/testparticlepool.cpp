// SPDX-License-Identifier: GPL-3.0-or-later
#include "testparticlepool.hpp"

using namespace koilo;

void TestParticlePool::TestAddIncrementsSize() {
    ParticlePool<int> p;
    TEST_ASSERT_EQUAL(0u, p.Size());
    p.Add(7);
    p.Add(11);
    TEST_ASSERT_EQUAL(2u, p.Size());
    TEST_ASSERT_EQUAL(2u, p.Capacity());
}

void TestParticlePool::TestRemoveFreesSlot() {
    ParticlePool<int> p;
    auto h0 = p.Add(1);
    auto h1 = p.Add(2);
    p.Remove(h0);
    TEST_ASSERT_FALSE(p.IsAlive(h0));
    TEST_ASSERT_TRUE(p.IsAlive(h1));
    TEST_ASSERT_EQUAL(1u, p.Size());
    TEST_ASSERT_EQUAL(2u, p.Capacity());
}

void TestParticlePool::TestHandleStability() {
    ParticlePool<int> p;
    auto h0 = p.Add(10);
    auto h1 = p.Add(20);
    auto h2 = p.Add(30);
    p.Remove(h1);
    auto h3 = p.Add(40);
    TEST_ASSERT_EQUAL(h1, h3);
    TEST_ASSERT_EQUAL(10, p.Get(h0));
    TEST_ASSERT_EQUAL(40, p.Get(h3));
    TEST_ASSERT_EQUAL(30, p.Get(h2));
}

void TestParticlePool::TestForEachVisitsLive() {
    ParticlePool<int> p;
    p.Add(1); auto h1 = p.Add(2); p.Add(3);
    p.Remove(h1);
    int sum = 0;
    p.ForEach([&](ParticlePool<int>::Handle, int& v){ sum += v; });
    TEST_ASSERT_EQUAL(4, sum);
}

void TestParticlePool::TestConstraintGraphAdd() {
    ConstraintGraph g;
    auto a = g.AddNode(100);
    auto b = g.AddNode(200);
    auto e = g.AddEdge(a, b, 7);
    TEST_ASSERT_EQUAL(2u, g.NodeCount());
    TEST_ASSERT_EQUAL(1u, g.EdgeCount());
    TEST_ASSERT_EQUAL(100u, g.GetNodeExternalId(a));
    TEST_ASSERT_EQUAL(7u, g.GetEdge(e).type);
    TEST_ASSERT_EQUAL(a, g.GetEdge(e).a);
    TEST_ASSERT_EQUAL(b, g.GetEdge(e).b);
}
