// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file asset_job_queue.hpp
 * @brief Async asset loading via a background worker thread.
 *
 * Jobs are submitted from the main thread and executed on a worker thread.
 * Completed results are collected back on the main thread via
 * ProcessCompleted(), which calls user callbacks with a configurable
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
#include <condition_variable>
#include <thread>
#include <deque>
#include <atomic>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

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
 * @brief Schedules asset load/unload work on a background thread.
 */
class AssetJobQueue {
public:
    /// Callback invoked on the main thread when a load completes.
    using LoadCallback = std::function<void(const AssetLoadResult&)>;

    /// The actual load function executed on the worker thread.
    /// Must be thread-safe. Returns the loaded data + byte size.
    using LoadFunction = std::function<AssetLoadResult(AssetHandle)>;

    AssetJobQueue();
    ~AssetJobQueue();

    /// Start the worker thread. Call once during initialization.
    void Start();

    /// Stop the worker thread. Blocks until the thread joins.
    void Stop();

    /// Submit an async load request.
    void RequestLoad(AssetHandle handle, LoadFunction loader, LoadCallback callback);

    /// Process completed jobs on the main thread.
    /// Invokes callbacks for up to maxPerFrame completed jobs.
    /// Returns number of callbacks invoked.
    int ProcessCompleted(int maxPerFrame = 4);

    /// Number of jobs currently pending or in flight.
    size_t PendingCount() const;

    /// Check if a specific handle has a pending/in-flight job.
    bool IsLoading(AssetHandle handle) const;

    /// Check if the worker thread is running.
    bool IsRunning() const { return running_.load(); }

private:
    struct Job {
        AssetHandle   handle;
        LoadFunction  loader;
        LoadCallback  callback;

        KL_BEGIN_FIELDS(Job)
            KL_FIELD(Job, handle, "Handle", 0, 0),
            KL_FIELD(Job, loader, "Loader", 0, 0),
            KL_FIELD(Job, callback, "Callback", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(Job)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(Job)
            /* No reflected ctors. */
        KL_END_DESCRIBE(Job)

    };

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

    // Worker thread state.
    std::thread             worker_;
    std::atomic<bool>       running_{false};
    std::atomic<bool>       stopRequested_{false};

    // Pending jobs (main thread enqueues, worker dequeues).
    mutable std::mutex      pendingMutex_;
    std::condition_variable pendingCV_;
    std::deque<Job>         pendingJobs_;

    // Completed jobs (worker enqueues, main thread dequeues).
    mutable std::mutex      completedMutex_;
    std::deque<CompletedJob> completedJobs_;

    void WorkerLoop();

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
