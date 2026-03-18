// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file thread_pool.hpp
 * @brief Fixed-size thread pool with priority job submission.
 *
 * Provides a shared pool of worker threads for the kernel. Jobs are
 * submitted with an optional priority (Low, Normal, High) and executed
 * in priority order. Supports std::future-based result retrieval.
 *
 * @date 01/18/2026
 * @author Coela
 */
#pragma once

#include <functional>
#include <future>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <cstdint>
#include "../registry/reflect_macros.hpp"

namespace koilo {

/// Job priority levels. Higher numeric value = higher priority.
enum class JobPriority : uint8_t {
    Low    = 0,
    Normal = 1,
    High   = 2
};

/// Fixed-size thread pool with priority scheduling.
class ThreadPool {
public:
    /// Construct with explicit thread count (0 = hardware_concurrency - 1, min 1).
    explicit ThreadPool(unsigned numThreads = 0);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// Submit a job and get a future for its result.
    template<typename F, typename... Args>
    auto Submit(JobPriority priority, F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    /// Submit a job at Normal priority.
    template<typename F, typename... Args>
    auto Submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    /// Submit a fire-and-forget task (no future returned).
    void Enqueue(std::function<void()> task, JobPriority priority = JobPriority::Normal);

    /// Number of worker threads.
    unsigned ThreadCount() const { return static_cast<unsigned>(workers_.size()); }

    /// Number of jobs waiting to be executed.
    size_t PendingCount() const;

    /// True if the pool is accepting jobs.
    bool IsRunning() const { return !stopped_.load(std::memory_order_relaxed); }

    /// Drain all pending jobs without executing them and stop the pool.
    void Shutdown();

private:
    struct PrioritizedJob {
        JobPriority             priority;
        uint64_t                sequence;   // FIFO within same priority
        std::function<void()>   task;

        bool operator<(const PrioritizedJob& rhs) const {
            if (priority != rhs.priority)
                return static_cast<uint8_t>(priority) < static_cast<uint8_t>(rhs.priority);
            return sequence > rhs.sequence;  // lower sequence = submitted earlier = higher
        }
    };

    void WorkerLoop();

    std::vector<std::thread>                        workers_;
    std::priority_queue<PrioritizedJob>             jobs_;
    mutable std::mutex                              mutex_;
    std::condition_variable                         cv_;
    std::atomic<bool>                               stopped_{false};
    uint64_t                                        nextSeq_{0};

    KL_BEGIN_FIELDS(ThreadPool)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ThreadPool)
        KL_METHOD_AUTO(ThreadPool, ThreadCount, "Thread count"),
        KL_METHOD_AUTO(ThreadPool, PendingCount, "Pending count"),
        KL_METHOD_AUTO(ThreadPool, IsRunning, "Is running")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ThreadPool)
    KL_END_DESCRIBE(ThreadPool)
};

// --- Template implementations ---

template<typename F, typename... Args>
auto ThreadPool::Submit(JobPriority priority, F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>
{
    using ReturnType = std::invoke_result_t<F, Args...>;
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    auto future = task->get_future();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopped_.load(std::memory_order_relaxed)) {
            throw std::runtime_error("ThreadPool: submit on stopped pool");
        }
        jobs_.push({priority, nextSeq_++, [task]() { (*task)(); }});
    }
    cv_.notify_one();
    return future;
}

template<typename F, typename... Args>
auto ThreadPool::Submit(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>
{
    return Submit(JobPriority::Normal, std::forward<F>(f), std::forward<Args>(args)...);
}

} // namespace koilo
