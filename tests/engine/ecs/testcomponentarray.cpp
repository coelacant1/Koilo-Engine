// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcomponentarray.cpp
 * @brief Tests for ComponentArray flat entity-by-index mapping, Reserve,
 *        GetEntities, and swap-remove correctness.
 */
#include "testcomponentarray.hpp"
#include <koilo/systems/ecs/entitymanager.hpp>

using namespace koilo;

struct TestComp { int value; };

static Entity MakeEntity(uint32_t index, uint32_t gen = 1) {
    return Entity(static_cast<EntityID>(gen) << 32 | index);
}

// -- Flat index lookup --

static void test_GetEntityReturnsCorrectEntity() {
    ComponentArray<TestComp> arr;
    Entity e1 = MakeEntity(1);
    Entity e2 = MakeEntity(2);
    Entity e3 = MakeEntity(3);
    arr.Add(e1, {10});
    arr.Add(e2, {20});
    arr.Add(e3, {30});

    TEST_ASSERT_TRUE(arr.GetEntity(0) == e1);
    TEST_ASSERT_TRUE(arr.GetEntity(1) == e2);
    TEST_ASSERT_TRUE(arr.GetEntity(2) == e3);
}

static void test_GetEntityOutOfBoundsReturnsNull() {
    ComponentArray<TestComp> arr;
    arr.Add(MakeEntity(1), {10});
    Entity null = arr.GetEntity(99);
    TEST_ASSERT_TRUE(null == Entity());
}

// -- GetEntities --

static void test_GetEntitiesParallelToComponents() {
    ComponentArray<TestComp> arr;
    Entity e1 = MakeEntity(10);
    Entity e2 = MakeEntity(20);
    arr.Add(e1, {100});
    arr.Add(e2, {200});

    const auto& entities = arr.GetEntities();
    const auto& comps = arr.GetComponents();
    TEST_ASSERT_EQUAL_UINT32(2, entities.size());
    TEST_ASSERT_EQUAL_UINT32(entities.size(), comps.size());
    TEST_ASSERT_TRUE(entities[0] == e1);
    TEST_ASSERT_TRUE(entities[1] == e2);
    TEST_ASSERT_EQUAL_INT(100, comps[0].value);
    TEST_ASSERT_EQUAL_INT(200, comps[1].value);
}

// -- Reserve --

static void test_ReserveDoesNotChangeSize() {
    ComponentArray<TestComp> arr;
    arr.Reserve(100);
    TEST_ASSERT_EQUAL_UINT32(0, arr.Size());
}

// -- Swap-remove correctness with flat index --

static void test_RemoveSwapsLastAndUpdatesIndex() {
    ComponentArray<TestComp> arr;
    Entity e1 = MakeEntity(1);
    Entity e2 = MakeEntity(2);
    Entity e3 = MakeEntity(3);
    arr.Add(e1, {10});
    arr.Add(e2, {20});
    arr.Add(e3, {30});

    // Remove middle element (e2). e3 should swap into index 1.
    arr.Remove(e2);
    TEST_ASSERT_EQUAL_UINT32(2, arr.Size());
    TEST_ASSERT_NULL(arr.Get(e2));

    // e1 still at index 0
    TEST_ASSERT_TRUE(arr.GetEntity(0) == e1);
    TEST_ASSERT_EQUAL_INT(10, arr.Get(e1)->value);

    // e3 should now be at index 1 (swapped in)
    TEST_ASSERT_TRUE(arr.GetEntity(1) == e3);
    TEST_ASSERT_EQUAL_INT(30, arr.Get(e3)->value);
}

static void test_RemoveLastElementNoSwap() {
    ComponentArray<TestComp> arr;
    Entity e1 = MakeEntity(1);
    Entity e2 = MakeEntity(2);
    arr.Add(e1, {10});
    arr.Add(e2, {20});

    // Removing the last element should not need a swap
    arr.Remove(e2);
    TEST_ASSERT_EQUAL_UINT32(1, arr.Size());
    TEST_ASSERT_TRUE(arr.GetEntity(0) == e1);
    TEST_ASSERT_EQUAL_INT(10, arr.Get(e1)->value);
}

// -- Clear --

static void test_ClearResetsEverything() {
    ComponentArray<TestComp> arr;
    arr.Add(MakeEntity(1), {10});
    arr.Add(MakeEntity(2), {20});
    arr.Clear();
    TEST_ASSERT_EQUAL_UINT32(0, arr.Size());
    TEST_ASSERT_EQUAL_UINT32(0, arr.GetEntities().size());
}

// -- Add duplicate replaces value --

static void test_AddDuplicateReplacesValue() {
    ComponentArray<TestComp> arr;
    Entity e1 = MakeEntity(1);
    arr.Add(e1, {10});
    arr.Add(e1, {99});
    TEST_ASSERT_EQUAL_UINT32(1, arr.Size());
    TEST_ASSERT_EQUAL_INT(99, arr.Get(e1)->value);
}

// -- Registration --

void TestComponentArray::RunAllTests() {
    RUN_TEST(test_GetEntityReturnsCorrectEntity);
    RUN_TEST(test_GetEntityOutOfBoundsReturnsNull);
    RUN_TEST(test_GetEntitiesParallelToComponents);
    RUN_TEST(test_ReserveDoesNotChangeSize);
    RUN_TEST(test_RemoveSwapsLastAndUpdatesIndex);
    RUN_TEST(test_RemoveLastElementNoSwap);
    RUN_TEST(test_ClearResetsEverything);
    RUN_TEST(test_AddDuplicateReplacesValue);
}
