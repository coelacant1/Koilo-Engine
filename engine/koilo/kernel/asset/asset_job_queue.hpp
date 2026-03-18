// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file asset_job_queue.hpp
 * @brief Async asset loading via the kernel ThreadPool.
 *
 * Jobs are submitted from the main thread and dispatched to the kernel
 * thread pool. Completed results are collected back on the main thread
 * via ProcessCompleted(), which calls user callbacks with a configurable
 * per-frame budget.
 *
 * @date 02/11/2026
 * @author Coela
 */
#pragma once

#include <koilo/kernel/asset/asset_handle.hpp>
#include <functional>
#include <memory>
#include <string>
#include <mutex>
#include <deque>
#include <atomic>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

class ThreadPool;

/**
 * @struct AssetLoadResult
 * @brief Outcome of an async load job.
 */
struct AssetLoadResult {
    AssetHandle handle;
    bool        success = false;
    std::shared_ptr<void> data;      ///< Type-erased loaded data
    size_t      memoryBytes = 0;
    std::string error;               ///< Error message on failure

    KL_BEGIN_FIELDS(AssetLoadResult)
        KL_FIELD(AssetLoadResult, handle, "Handle", 0, 0),
        KL_FIELD(AssetLoadResult, success, "Success", 0, 1),
        KL_FIELD(AssetLoadResult, data, "Data", 0, 0),
        KL_FIELD(AssetLoadResult, error, "Error", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(AssetLoadResult)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AssetLoadResult)
        /* No reflected ctors. */
    KL_END_DESCRIBE(AssetLoadResult)

};

/**
 * @class AssetJobQueue
 * @brief Schedules asset load/unload work on the kernel thread pool.
 */
class AssetJobQueue {
public:
    /// Callback invoked on the main thread when a load completes.
    using LoadCallback = std::function<void(const AssetLoadResult&)>;

    /// The actual load function executed on a pool thread.
    /// Must be thread-safe. Returns the loaded data + byte size.
    using LoadFunction = std::function<AssetLoadResult(AssetHandle)>;

    AssetJobQueue();
    ~AssetJobQueue();

    /// Bind to a thread pool. Must be called before RequestLoad.
    void SetPool(ThreadPool* pool) { pool_ = pool; }

    /// Submit an async load request (dispatched to thread pool).
    void RequestLoad(AssetHandle handle, LoadFunction loader, LoadCallback callback);

    /// Process completed jobs on the main thread.
    /// Invokes callbacks for up to maxPerFrame completed jobs.
    /// Returns number of callbacks invoked.
    int ProcessCompleted(int maxPerFrame = 4);

    /// Number of jobs currently in-flight (submitted but not yet completed on main thread).
    size_t PendingCount() const { return inFlight_.load(std::memory_order_relaxed); }

    /// Check if a specific handle has an in-flight job.
    bool IsLoading(AssetHandle handle) const;

    /// Check if the queue is bound to a pool and operational.
    bool IsRunning() const { return pool_ != nullptr; }

private:
    struct CompletedJob {
        AssetLoadResult result;
        LoadCallback    callback;

        KL_BEGIN_FIELDS(CompletedJob)
            KL_FIELD(CompletedJob, result, "Result", 0, 0),
            KL_FIELD(CompletedJob, callback, "Callback", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(CompletedJob)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(CompletedJob)
            /* No reflected ctors. */
        KL_END_DESCRIBE(CompletedJob)

    };

    ThreadPool*                  pool_ = nullptr;
    std::atomic<size_t>          inFlight_{0};

    // In-flight handles (for IsLoading queries).
    mutable std::mutex           handleMutex_;
    std::deque<AssetHandle>      activeHandles_;

    // Completed jobs (pool threads enqueue, main thread dequeues).
    mutable std::mutex           completedMutex_;
    std::deque<CompletedJob>     completedJobs_;

    KL_BEGIN_FIELDS(AssetJobQueue)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(AssetJobQueue)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AssetJobQueue)
        KL_CTOR0(AssetJobQueue)
    KL_END_DESCRIBE(AssetJobQueue)

};

} // namespace koilo
