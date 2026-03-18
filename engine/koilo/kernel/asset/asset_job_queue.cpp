// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file asset_job_queue.cpp
 * @brief AssetJobQueue implementation - dispatches to kernel ThreadPool.
 */
#include <koilo/kernel/asset/asset_job_queue.hpp>
#include <koilo/kernel/thread_pool.hpp>
#include <algorithm>

namespace koilo {

AssetJobQueue::AssetJobQueue() = default;
AssetJobQueue::~AssetJobQueue() = default;

void AssetJobQueue::RequestLoad(AssetHandle handle, LoadFunction loader, LoadCallback callback) {
    if (!pool_) return;

    // Track the handle as in-flight.
    {
        std::lock_guard<std::mutex> lock(handleMutex_);
        activeHandles_.push_back(handle);
    }
    inFlight_.fetch_add(1, std::memory_order_relaxed);

    // Capture by value - the lambda outlives this call.
    pool_->Enqueue([this, handle, loader = std::move(loader),
                    callback = std::move(callback)]() mutable {
        AssetLoadResult result;
        try {
            result = loader(handle);
        } catch (const std::exception& e) {
            result.handle = handle;
            result.success = false;
            result.error = e.what();
        } catch (...) {
            result.handle = handle;
            result.success = false;
            result.error = "Unknown exception during asset load";
        }

        // Enqueue completed result for main-thread processing.
        {
            std::lock_guard<std::mutex> lock(completedMutex_);
            completedJobs_.push_back({std::move(result), std::move(callback)});
        }

        // Remove from active handles.
        {
            std::lock_guard<std::mutex> lock(handleMutex_);
            auto it = std::find_if(activeHandles_.begin(), activeHandles_.end(),
                [&](const AssetHandle& h) { return h == handle; });
            if (it != activeHandles_.end()) activeHandles_.erase(it);
        }
        inFlight_.fetch_sub(1, std::memory_order_relaxed);
    }, JobPriority::Normal);
}

int AssetJobQueue::ProcessCompleted(int maxPerFrame) {
    std::deque<CompletedJob> batch;
    {
        std::lock_guard<std::mutex> lock(completedMutex_);
        int count = std::min(maxPerFrame, static_cast<int>(completedJobs_.size()));
        for (int i = 0; i < count; ++i) {
            batch.push_back(std::move(completedJobs_.front()));
            completedJobs_.pop_front();
        }
    }

    int invoked = 0;
    for (auto& cj : batch) {
        if (cj.callback) {
            cj.callback(cj.result);
        }
        ++invoked;
    }
    return invoked;
}

bool AssetJobQueue::IsLoading(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(handleMutex_);
    for (const auto& h : activeHandles_) {
        if (h == handle) return true;
    }
    return false;
}

} // namespace koilo
