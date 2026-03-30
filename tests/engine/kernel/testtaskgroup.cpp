// SPDX-License-Identifier: GPL-3.0-or-later
#include "testtaskgroup.hpp"
#include <koilo/kernel/cancellation_token.hpp>
#include <koilo/kernel/task_group.hpp>
#include <koilo/kernel/thread_pool.hpp>
#include <atomic>
#include <chrono>
#include <thread>

using namespace koilo;

void TestTaskGroup::TestCancellationTokenBasic() {
    CancellationToken token;
    TEST_ASSERT_FALSE(token.IsCancelled());

    token.Cancel();
    TEST_ASSERT_TRUE(token.IsCancelled());

    token.Reset();
    TEST_ASSERT_FALSE(token.IsCancelled());
}

void TestTaskGroup::TestCancellationTokenParentChild() {
    auto parent = std::make_shared<CancellationToken>();
    auto child1 = parent->CreateChild();
    auto child2 = parent->CreateChild();

    TEST_ASSERT_FALSE(child1->IsCancelled());
    TEST_ASSERT_FALSE(child2->IsCancelled());

    // Cancelling parent propagates to children
    parent->Cancel();
    TEST_ASSERT_TRUE(child1->IsCancelled());
    TEST_ASSERT_TRUE(child2->IsCancelled());

    // Child created after cancellation is immediately cancelled
    auto child3 = parent->CreateChild();
    TEST_ASSERT_TRUE(child3->IsCancelled());
}

void TestTaskGroup::TestTaskGroupSpawnAndWait() {
    ThreadPool pool(2);
    std::atomic<int> counter{0};

    {
        TaskGroup group("test-spawn", pool);
        group.Spawn("inc1", [&counter]() { counter.fetch_add(1); });
        group.Spawn("inc2", [&counter]() { counter.fetch_add(1); });
        group.Spawn("inc3", [&counter]() { counter.fetch_add(1); });
        // ~TaskGroup waits for all
    }

    TEST_ASSERT_EQUAL_INT(3, counter.load());
    pool.Shutdown();
}

void TestTaskGroup::TestTaskGroupReturnValue() {
    ThreadPool pool(2);

    TaskGroup group("test-return", pool);
    auto h1 = group.Spawn("add", []() -> int { return 21 + 21; });
    auto h2 = group.Spawn("mul", []() -> int { return 6 * 7; });

    TEST_ASSERT_EQUAL_INT(42, h1.Get());
    TEST_ASSERT_EQUAL_INT(42, h2.Get());

    pool.Shutdown();
}

void TestTaskGroup::TestTaskGroupCancellation() {
    ThreadPool pool(2);
    std::atomic<int> executed{0};

    {
        TaskGroup group("test-cancel", pool);

        // Spawn a task that will block briefly
        group.Spawn("blocker", [&executed]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            executed.fetch_add(1);
        });

        // Cancel the group immediately
        group.Cancel();

        // Spawn more tasks after cancel - they should see cancellation
        group.Spawn("cancelled1", [&executed]() { executed.fetch_add(10); });
        group.Spawn("cancelled2", [&executed]() { executed.fetch_add(10); });
    }

    // The blocker may have already started before Cancel(), so executed is 0 or 1
    // The cancelled tasks should not have added 10 each
    TEST_ASSERT_TRUE(executed.load() <= 1);

    pool.Shutdown();
}

void TestTaskGroup::TestTaskGroupRAIIWait() {
    ThreadPool pool(2);
    std::atomic<bool> finished{false};

    {
        TaskGroup group("test-raii", pool);
        group.Spawn("slow", [&finished]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            finished.store(true);
        });
        // Destructor must block until "slow" completes
    }

    TEST_ASSERT_TRUE(finished.load());
    pool.Shutdown();
}

void TestTaskGroup::TestTaskGroupListTasks() {
    ThreadPool pool(1);
    std::atomic<int> gate{0};

    TaskGroup group("test-list", pool);
    group.Spawn("alpha", [&gate]() { while (gate.load() == 0) std::this_thread::yield(); });

    TEST_ASSERT_EQUAL(1u, group.TaskCount());

    auto tasks = group.ListTasks();
    TEST_ASSERT_EQUAL(1u, tasks.size());
    TEST_ASSERT_EQUAL_STRING("alpha", tasks[0].first.c_str());

    // Release the gate so task completes
    gate.store(1);
    group.WaitAll();

    TEST_ASSERT_TRUE(group.AllDone());
    TEST_ASSERT_EQUAL(1u, group.CountByStatus(TaskStatus::Completed));

    pool.Shutdown();
}

void TestTaskGroup::TestTaskStatusNames() {
    TEST_ASSERT_EQUAL_STRING("pending",   TaskStatusName(TaskStatus::Pending));
    TEST_ASSERT_EQUAL_STRING("running",   TaskStatusName(TaskStatus::Running));
    TEST_ASSERT_EQUAL_STRING("done",      TaskStatusName(TaskStatus::Completed));
    TEST_ASSERT_EQUAL_STRING("failed",    TaskStatusName(TaskStatus::Failed));
    TEST_ASSERT_EQUAL_STRING("cancelled", TaskStatusName(TaskStatus::Cancelled));
}

void TestTaskGroup::RunAllTests() {
    RUN_TEST(TestCancellationTokenBasic);
    RUN_TEST(TestCancellationTokenParentChild);
    RUN_TEST(TestTaskGroupSpawnAndWait);
    RUN_TEST(TestTaskGroupReturnValue);
    RUN_TEST(TestTaskGroupCancellation);
    RUN_TEST(TestTaskGroupRAIIWait);
    RUN_TEST(TestTaskGroupListTasks);
    RUN_TEST(TestTaskStatusNames);
}
