// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file task_group.hpp
 * @brief Structured concurrency: TaskGroup and TaskHandle.
 *
 * A TaskGroup owns a set of child tasks submitted to a ThreadPool.
 * Its destructor blocks until all children complete (RAII guarantee).
 * If any task throws, siblings are cancelled automatically.
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

#include "cancellation_token.hpp"
#include "thread_pool.hpp"

#include <chrono>
#include <future>
#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <functional>
#include <stdexcept>

namespace koilo {

/// Status of a task within a group.
enum class TaskStatus : uint8_t {
    Pending,
    Running,
    Completed,
    Failed,
    Cancelled
};

inline const char* TaskStatusName(TaskStatus s) {
    switch (s) {
        case TaskStatus::Pending:   return "pending";
        case TaskStatus::Running:   return "running";
        case TaskStatus::Completed: return "done";
        case TaskStatus::Failed:    return "failed";
        case TaskStatus::Cancelled: return "cancelled";
        default:                    return "?";
    }
}

/// Lightweight handle to a spawned task's result.
template<typename T>
class TaskHandle {
public:
    TaskHandle() = default;
    explicit TaskHandle(std::future<T>&& fut) : future_(std::move(fut)) {}

    T Get() { return future_.get(); }

    bool IsReady() const {
        return future_.valid() &&
               future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    void Wait() const { if (future_.valid()) future_.wait(); }
    bool Valid() const { return future_.valid(); }

private:
    std::future<T> future_;
};

template<>
class TaskHandle<void> {
public:
    TaskHandle() = default;
    explicit TaskHandle(std::future<void>&& fut) : future_(std::move(fut)) {}

    void Get() { future_.get(); }

    bool IsReady() const {
        return future_.valid() &&
               future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    void Wait() const { if (future_.valid()) future_.wait(); }
    bool Valid() const { return future_.valid(); }

private:
    std::future<void> future_;
};

/// Internal record of a task within a group.
struct TaskRecord {
    std::string             name;
    std::future<void>       waiter;   ///< Signals when task finishes (regardless of result)
    std::atomic<TaskStatus> status{TaskStatus::Pending};
};

/// A group of tasks with shared cancellation and RAII wait-for-all.
class TaskGroup {
public:
    TaskGroup(const std::string& name, ThreadPool& pool,
              std::shared_ptr<CancellationToken> parentToken = nullptr)
        : name_(name), pool_(pool)
    {
        if (parentToken) {
            token_ = parentToken->CreateChild();
        } else {
            token_ = std::make_shared<CancellationToken>();
        }
    }

    TaskGroup(const TaskGroup&) = delete;
    TaskGroup& operator=(const TaskGroup&) = delete;

    /// Destructor: blocks until all spawned tasks complete.
    ~TaskGroup() { WaitAll(); }

    /// Spawn a task that returns a value.
    template<typename F>
    auto Spawn(const std::string& taskName, F&& callable)
        -> TaskHandle<std::invoke_result_t<F>>
    {
        using R = std::invoke_result_t<F>;

        auto record = std::make_shared<TaskRecord>();
        record->name = taskName;
        record->status.store(TaskStatus::Running, std::memory_order_relaxed);

        // Separate completion signal (always fulfilled, even on exception)
        auto done = std::make_shared<std::promise<void>>();
        record->waiter = done->get_future();

        auto tokenCopy = token_;

        auto future = pool_.Submit(JobPriority::Normal,
            [fn = std::forward<F>(callable), tokenCopy, record, done]() mutable -> R {
                if (tokenCopy->IsCancelled()) {
                    record->status.store(TaskStatus::Cancelled, std::memory_order_relaxed);
                    done->set_value();
                    if constexpr (std::is_void_v<R>) return;
                    else return R{};
                }
                try {
                    if constexpr (std::is_void_v<R>) {
                        fn();
                        record->status.store(TaskStatus::Completed, std::memory_order_relaxed);
                        done->set_value();
                    } else {
                        auto result = fn();
                        record->status.store(TaskStatus::Completed, std::memory_order_relaxed);
                        done->set_value();
                        return result;
                    }
                } catch (...) {
                    record->status.store(TaskStatus::Failed, std::memory_order_relaxed);
                    tokenCopy->Cancel(); // Cancel siblings on failure
                    done->set_value();
                    throw;
                }
            });

        {
            std::lock_guard<std::mutex> lock(mutex_);
            records_.push_back(record);
        }

        return TaskHandle<R>(std::move(future));
    }

    /// Cancel all tasks in this group.
    void Cancel() { token_->Cancel(); }

    /// Block until all spawned tasks complete or are cancelled.
    void WaitAll() {
        std::vector<std::shared_ptr<TaskRecord>> snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            snapshot = records_;
        }
        for (auto& rec : snapshot) {
            if (rec->waiter.valid()) {
                rec->waiter.wait();
            }
        }
    }

    /// Check if all tasks are done.
    bool AllDone() const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& rec : records_) {
            auto s = rec->status.load(std::memory_order_acquire);
            if (s == TaskStatus::Running || s == TaskStatus::Pending)
                return false;
        }
        return true;
    }

    const std::string& Name() const { return name_; }
    std::shared_ptr<CancellationToken> Token() const { return token_; }

    size_t TaskCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return records_.size();
    }

    size_t CountByStatus(TaskStatus status) const {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t n = 0;
        for (auto& rec : records_)
            if (rec->status.load(std::memory_order_relaxed) == status) ++n;
        return n;
    }

    /// Get task records for console inspection.
    std::vector<std::pair<std::string, TaskStatus>> ListTasks() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::pair<std::string, TaskStatus>> result;
        result.reserve(records_.size());
        for (auto& rec : records_)
            result.emplace_back(rec->name, rec->status.load(std::memory_order_relaxed));
        return result;
    }

private:
    std::string name_;
    ThreadPool& pool_;
    std::shared_ptr<CancellationToken> token_;
    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<TaskRecord>> records_;
};

} // namespace koilo
