// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testheappool.cpp
 * @brief Tests for HeapPool: acquire, reset, high-water mark, chunk growth,
 *        and pointer stability.
 */
#include "testheappool.hpp"
#include <koilo/kernel/memory/heap_pool.hpp>
#include <string>
#include <vector>

using namespace koilo;

// -- Basic acquire and count --

static void test_AcquireIncrementsCount() {
    HeapPool<int> pool(8);
    TEST_ASSERT_EQUAL_UINT32(0, pool.Count());
    pool.Acquire() = 42;
    TEST_ASSERT_EQUAL_UINT32(1, pool.Count());
    pool.Acquire() = 99;
    TEST_ASSERT_EQUAL_UINT32(2, pool.Count());
}

static void test_ResetSetsCountToZero() {
    HeapPool<int> pool(8);
    pool.Acquire();
    pool.Acquire();
    pool.Acquire();
    TEST_ASSERT_EQUAL_UINT32(3, pool.Count());
    pool.Reset();
    TEST_ASSERT_EQUAL_UINT32(0, pool.Count());
}

// -- High-water mark --

static void test_HighWaterMarkTracked() {
    HeapPool<int> pool(16);
    for (int i = 0; i < 5; ++i) pool.Acquire();
    TEST_ASSERT_EQUAL_UINT32(5, pool.HighWaterMark());

    pool.Reset();
    for (int i = 0; i < 3; ++i) pool.Acquire();
    // Water mark should still be 5, not 3
    TEST_ASSERT_EQUAL_UINT32(5, pool.HighWaterMark());
}

static void test_HighWaterMarkGrows() {
    HeapPool<int> pool(16);
    for (int i = 0; i < 5; ++i) pool.Acquire();
    pool.Reset();
    for (int i = 0; i < 10; ++i) pool.Acquire();
    TEST_ASSERT_EQUAL_UINT32(10, pool.HighWaterMark());
}

// -- Chunk growth --

static void test_SingleChunkWhenUnderCapacity() {
    HeapPool<int> pool(16);
    for (int i = 0; i < 16; ++i) pool.Acquire();
    TEST_ASSERT_EQUAL_UINT32(1, pool.ChunkCount());
}

static void test_ChunkGrowthOnOverflow() {
    HeapPool<int> pool(4);
    // Fill first chunk
    for (int i = 0; i < 4; ++i) pool.Acquire();
    TEST_ASSERT_EQUAL_UINT32(1, pool.ChunkCount());
    // Trigger second chunk
    pool.Acquire();
    TEST_ASSERT_EQUAL_UINT32(2, pool.ChunkCount());
}

// -- Pointer stability --

static void test_PointerStabilityAcrossAcquire() {
    HeapPool<std::string> pool(4);
    std::string& first = pool.Acquire();
    first = "hello";
    std::string* firstPtr = &first;

    // Acquire more to fill chunk and potentially grow
    for (int i = 0; i < 8; ++i) pool.Acquire();

    // First element must still be at the same address
    TEST_ASSERT_EQUAL_PTR(firstPtr, &first);
    TEST_ASSERT_EQUAL_STRING("hello", firstPtr->c_str());
}

// -- Reuse after reset --

static void test_SlotsReusedAfterReset() {
    HeapPool<int> pool(8);
    int& a = pool.Acquire();
    a = 100;
    int* aPtr = &a;

    pool.Reset();
    int& b = pool.Acquire();
    // Should reuse the same slot
    TEST_ASSERT_EQUAL_PTR(aPtr, &b);
}

// -- String pool (mirrors VM usage) --

static void test_StringPoolUsagePattern() {
    HeapPool<std::string> pool(32);
    // Simulate a frame: allocate several strings
    pool.Acquire() = "script_var_a";
    pool.Acquire() = "hello world";
    pool.Acquire() = "concat result";
    TEST_ASSERT_EQUAL_UINT32(3, pool.Count());

    // Frame end: reset
    pool.Reset();
    TEST_ASSERT_EQUAL_UINT32(0, pool.Count());

    // Next frame: reuse
    std::string& s = pool.Acquire();
    s = "new_frame";
    TEST_ASSERT_EQUAL_STRING("new_frame", s.c_str());
}

// -- Capacity --

static void test_CapacityMatchesConstructed() {
    HeapPool<int> pool(8);
    TEST_ASSERT_EQUAL_UINT32(0, pool.Capacity());
    for (int i = 0; i < 5; ++i) pool.Acquire();
    TEST_ASSERT_EQUAL_UINT32(5, pool.Capacity());
    pool.Reset();
    // Capacity stays (slots are constructed, just reusable)
    TEST_ASSERT_EQUAL_UINT32(5, pool.Capacity());
}

// -- Registration --

void TestHeapPool::RunAllTests() {
    RUN_TEST(test_AcquireIncrementsCount);
    RUN_TEST(test_ResetSetsCountToZero);
    RUN_TEST(test_HighWaterMarkTracked);
    RUN_TEST(test_HighWaterMarkGrows);
    RUN_TEST(test_SingleChunkWhenUnderCapacity);
    RUN_TEST(test_ChunkGrowthOnOverflow);
    RUN_TEST(test_PointerStabilityAcrossAcquire);
    RUN_TEST(test_SlotsReusedAfterReset);
    RUN_TEST(test_StringPoolUsagePattern);
    RUN_TEST(test_CapacityMatchesConstructed);
}
