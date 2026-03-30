// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testtaskgroup.hpp
 * @brief Unit tests for CancellationToken, TaskGroup, and TaskHandle.
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

#include <unity.h>
#include <koilo/kernel/cancellation_token.hpp>
#include <koilo/kernel/task_group.hpp>
#include <koilo/kernel/thread_pool.hpp>
#include <utils/testhelpers.hpp>

class TestTaskGroup {
public:
    static void TestCancellationTokenBasic();
    static void TestCancellationTokenParentChild();
    static void TestTaskGroupSpawnAndWait();
    static void TestTaskGroupReturnValue();
    static void TestTaskGroupCancellation();
    static void TestTaskGroupRAIIWait();
    static void TestTaskGroupListTasks();
    static void TestTaskStatusNames();

    static void RunAllTests();
};
