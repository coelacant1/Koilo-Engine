// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file asset_job_queue.cpp
 * @brief AssetJobQueue implementation - background worker thread.
 */
#include <koilo/kernel/asset/asset_job_queue.hpp>

namespace koilo {

AssetJobQueue::AssetJobQueue() = default;

AssetJobQueue::~AssetJobQueue() {
    Stop();
}

void AssetJobQueue::Start() {
    if (running_.load()) return;
    stopRequested_.store(false);
    running_.store(true);
    worker_ = std::thread(&AssetJobQueue::WorkerLoop, this);
}

void AssetJobQueue::Stop() {
    if (!running_.load()) return;
    stopRequested_.store(true);
    pendingCV_.notify_all();
    if (worker_.joinable()) worker_.join();
    running_.store(false);
}

void AssetJobQueue::RequestLoad(AssetHandle handle, LoadFunction loader, LoadCallback callback) {
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pendingJobs_.push_back({handle, std::move(loader), std::move(callback)});
    }
    pendingCV_.notify_one();
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

size_t AssetJobQueue::PendingCount() const {
    std::lock_guard<std::mutex> lock(pendingMutex_);
    return pendingJobs_.size();
}

bool AssetJobQueue::IsLoading(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(pendingMutex_);
    for (const auto& job : pendingJobs_) {
        if (job.handle == handle) return true;
    }
    return false;
}

void AssetJobQueue::WorkerLoop() {
    while (!stopRequested_.load()) {
        Job job;
        {
            std::unique_lock<std::mutex> lock(pendingMutex_);
            pendingCV_.wait(lock, [this] {
                return !pendingJobs_.empty() || stopRequested_.load();
            });
            if (stopRequested_.load()) break;
            if (pendingJobs_.empty()) continue;
            job = std::move(pendingJobs_.front());
            pendingJobs_.pop_front();
        }

        // Execute the load function on this worker thread.
        AssetLoadResult result;
        try {
            result = job.loader(job.handle);
        } catch (const std::exception& e) {
            result.handle = job.handle;
            result.success = false;
            result.error = e.what();
        } catch (...) {
            result.handle = job.handle;
            result.success = false;
            result.error = "Unknown exception during asset load";
        }

        // Enqueue the result for main-thread processing.
        {
            std::lock_guard<std::mutex> lock(completedMutex_);
            completedJobs_.push_back({std::move(result), std::move(job.callback)});
        }
    }
}

} // namespace koilo
