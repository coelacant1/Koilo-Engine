// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file thread_pool.cpp
 * @brief ThreadPool implementation - fixed worker threads with priority queue.
 */
#include <koilo/kernel/thread_pool.hpp>
#include <algorithm>

namespace koilo {

ThreadPool::ThreadPool(unsigned numThreads) {
    if (numThreads == 0) {
        unsigned hw = std::thread::hardware_concurrency();
        numThreads = (hw > 1) ? hw - 1 : 1;
    }

    workers_.reserve(numThreads);
    for (unsigned i = 0; i < numThreads; ++i) {
        workers_.emplace_back(&ThreadPool::WorkerLoop, this);
    }
}

ThreadPool::~ThreadPool() {
    Shutdown();
}

void ThreadPool::Enqueue(std::function<void()> task, JobPriority priority) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopped_.load(std::memory_order_relaxed)) return;
        jobs_.push({priority, nextSeq_++, std::move(task)});
    }
    cv_.notify_one();
}

size_t ThreadPool::PendingCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return jobs_.size();
}

void ThreadPool::Shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopped_.load(std::memory_order_relaxed)) return;
        stopped_.store(true, std::memory_order_relaxed);
        // Drain pending jobs
        while (!jobs_.empty()) jobs_.pop();
    }
    cv_.notify_all();

    for (auto& w : workers_) {
        if (w.joinable()) w.join();
    }
    workers_.clear();
}

void ThreadPool::WorkerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] {
                return stopped_.load(std::memory_order_relaxed) || !jobs_.empty();
            });
            if (stopped_.load(std::memory_order_relaxed)) return;
            if (jobs_.empty()) continue;

            task = std::move(const_cast<PrioritizedJob&>(jobs_.top()).task);
            jobs_.pop();
        }
        task();
    }
}

} // namespace koilo
