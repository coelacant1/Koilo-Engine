// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testscratchscope.cpp
 * @brief Tests for LinearAllocator::ScratchScope RAII save/restore.
 */
#include "testscratchscope.hpp"
#include <koilo/kernel/memory/linear_allocator.hpp>

using namespace koilo;

// -- Scope restores marker on destruction --

static void test_ScopeRestoresUsed() {
    LinearAllocator alloc(1024);
    alloc.Allocate(64);
    size_t before = alloc.Used();

    {
        LinearAllocator::ScratchScope scope(alloc);
        scope.Allocate(128);
        TEST_ASSERT_TRUE(alloc.Used() > before);
    }
    // After scope destruction, Used() should return to the saved marker
    TEST_ASSERT_EQUAL_UINT32(before, alloc.Used());
}

// -- Nested scopes restore independently --

static void test_NestedScopes() {
    LinearAllocator alloc(1024);

    alloc.Allocate(32);
    size_t level0 = alloc.Used();

    {
        LinearAllocator::ScratchScope outer(alloc);
        outer.Allocate(64);
        size_t level1 = alloc.Used();

        {
            LinearAllocator::ScratchScope inner(alloc);
            inner.Allocate(128);
            TEST_ASSERT_TRUE(alloc.Used() > level1);
        }
        // Inner scope restored
        TEST_ASSERT_EQUAL_UINT32(level1, alloc.Used());
    }
    // Outer scope restored
    TEST_ASSERT_EQUAL_UINT32(level0, alloc.Used());
}

// -- Scope allocations are usable within scope --

static void test_ScopeAllocationsUsable() {
    LinearAllocator alloc(1024);
    {
        LinearAllocator::ScratchScope scope(alloc);
        int* data = scope.Allocate<int>(4);
        TEST_ASSERT_NOT_NULL(data);
        data[0] = 10;
        data[1] = 20;
        data[2] = 30;
        data[3] = 40;
        TEST_ASSERT_EQUAL_INT(10, data[0]);
        TEST_ASSERT_EQUAL_INT(40, data[3]);
    }
}

// -- Empty scope is a no-op --

static void test_EmptyScopeNoOp() {
    LinearAllocator alloc(1024);
    alloc.Allocate(100);
    size_t before = alloc.Used();
    {
        LinearAllocator::ScratchScope scope(alloc);
        // No allocations
    }
    TEST_ASSERT_EQUAL_UINT32(before, alloc.Used());
}

// -- Registration --

void TestScratchScope::RunAllTests() {
    RUN_TEST(test_ScopeRestoresUsed);
    RUN_TEST(test_NestedScopes);
    RUN_TEST(test_ScopeAllocationsUsable);
    RUN_TEST(test_EmptyScopeNoOp);
}
