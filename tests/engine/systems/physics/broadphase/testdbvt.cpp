// SPDX-License-Identifier: GPL-3.0-or-later
#include "testdbvt.hpp"
#include <koilo/core/geometry/3d/aabb.hpp>
#include <utils/testhelpers.hpp>
#include <vector>
#include <utility>

using namespace koilo;

namespace {
AABB MakeBox(float cx, float cy, float cz, float h = 0.5f) {
    return AABB(Vector3D(cx-h, cy-h, cz-h), Vector3D(cx+h, cy+h, cz+h));
}
}

void TestDBVT::TestInsertAssignsHandleAndContainsAabb() {
    DynamicAABBTree t;
    int dummy = 1;
    auto h = t.Insert(MakeBox(0,0,0), &dummy, 0.05f);
    TEST_ASSERT_NOT_EQUAL(-1, h);
    TEST_ASSERT_EQUAL_PTR(&dummy, t.GetUserData(h));
    TEST_ASSERT_EQUAL_size_t(1u, t.NodeCount());
}

void TestDBVT::TestRemoveFreesHandle() {
    DynamicAABBTree t;
    int dummy = 1;
    auto h = t.Insert(MakeBox(0,0,0), &dummy);
    t.Remove(h);
    TEST_ASSERT_EQUAL_size_t(0u, t.NodeCount());
}

void TestDBVT::TestMoveNoOpWhenWithinFat() {
    DynamicAABBTree t;
    int dummy = 1;
    auto h = t.Insert(MakeBox(0,0,0, 1.0f), &dummy, 0.5f);
    bool moved = t.Move(h, MakeBox(0.1f, 0.0f, 0.0f, 1.0f), 0.5f);
    TEST_ASSERT_FALSE(moved);
}

void TestDBVT::TestMoveReinsertsWhenEscaping() {
    DynamicAABBTree t;
    int dummy = 1;
    auto h = t.Insert(MakeBox(0,0,0, 1.0f), &dummy, 0.05f);
    bool moved = t.Move(h, MakeBox(10,0,0, 1.0f), 0.05f);
    TEST_ASSERT_TRUE(moved);
}

void TestDBVT::TestQueryOverlapsLeaves() {
    DynamicAABBTree t;
    int a=1,b=2,c=3;
    t.Insert(MakeBox(0,0,0), &a);
    t.Insert(MakeBox(10,0,0), &b);
    t.Insert(MakeBox(0,10,0), &c);

    int hits = 0;
    t.Query(MakeBox(0,0,0, 1.0f), [&](std::int32_t){ ++hits; return true; });
    TEST_ASSERT_EQUAL_INT(1, hits);
}

void TestDBVT::TestQueryAllPairsReturnsCanonicalOrder() {
    DynamicAABBTree t;
    int a=1,b=2;
    auto h1 = t.Insert(MakeBox(0,0,0, 1.0f), &a);
    auto h2 = t.Insert(MakeBox(0.5f,0,0, 1.0f), &b);
    std::vector<std::pair<std::int32_t, std::int32_t>> pairs;
    t.QueryAllPairs(pairs);
    TEST_ASSERT_EQUAL_size_t(1u, pairs.size());
    TEST_ASSERT_TRUE(pairs[0].first < pairs[0].second);
    (void)h1; (void)h2;
}

void TestDBVT::TestQueryAllPairsExcludesNonOverlapping() {
    DynamicAABBTree t;
    int a=1,b=2;
    t.Insert(MakeBox(0,0,0), &a);
    t.Insert(MakeBox(10,0,0), &b);
    std::vector<std::pair<std::int32_t, std::int32_t>> pairs;
    t.QueryAllPairs(pairs);
    TEST_ASSERT_EQUAL_size_t(0u, pairs.size());
}

void TestDBVT::TestQuality() {
    DynamicAABBTree t;
    int a=1;
    t.Insert(MakeBox(0,0,0), &a);
    auto q = t.ComputeQuality();
    TEST_ASSERT_EQUAL_INT(0, q.height);
}

void TestDBVT::RunAllTests() {
    RUN_TEST(TestInsertAssignsHandleAndContainsAabb);
    RUN_TEST(TestRemoveFreesHandle);
    RUN_TEST(TestMoveNoOpWhenWithinFat);
    RUN_TEST(TestMoveReinsertsWhenEscaping);
    RUN_TEST(TestQueryOverlapsLeaves);
    RUN_TEST(TestQueryAllPairsReturnsCanonicalOrder);
    RUN_TEST(TestQueryAllPairsExcludesNonOverlapping);
    RUN_TEST(TestQuality);
}
