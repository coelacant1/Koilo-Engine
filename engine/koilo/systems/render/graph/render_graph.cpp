// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file render_graph.cpp
 * @brief Render graph implementation: DAG construction, topological sort,
 *        resource lifetime computation.
 */
#include "render_graph.hpp"
#include <koilo/kernel/logging/log.hpp>
#include <queue>
#include <algorithm>

namespace koilo::rhi {

void RenderGraph::AddPass(std::string              name,
                          std::vector<std::string> reads,
                          std::vector<std::string> writes,
                          std::function<void()>    execute) {
    passes_.push_back({std::move(name),
                       std::move(reads),
                       std::move(writes),
                       std::move(execute)});
    compiled_ = false;
}

bool RenderGraph::Compile() {
    compiled_ = false;
    order_.clear();
    lifetimes_.clear();

    const size_t n = passes_.size();
    if (n == 0) { compiled_ = true; return true; }

    // -- Build the last-writer map (resource -> most recent writer index) --
    // We scan passes in insertion order.  For each pass:
    //   - For every resource it READS:  add an edge from the last writer to this pass.
    //   - For every resource it WRITES: add a WAW edge from the last writer, then
    //     update the last-writer entry.

    std::unordered_map<std::string, size_t> lastWriter;   // resource -> pass index
    std::vector<std::vector<size_t>> adj(n);              // adjacency list
    std::vector<int> inDeg(n, 0);

    auto addEdge = [&](size_t from, size_t to) {
        adj[from].push_back(to);
        ++inDeg[to];
    };

    for (size_t i = 0; i < n; ++i) {
        const auto& pass = passes_[i];

        // RAW: depend on the last writer of each resource we read
        for (const auto& r : pass.reads) {
            auto it = lastWriter.find(r);
            if (it != lastWriter.end() && it->second != i) {
                addEdge(it->second, i);
            }
        }

        // WAW: depend on the last writer of each resource we write
        for (const auto& w : pass.writes) {
            auto it = lastWriter.find(w);
            if (it != lastWriter.end() && it->second != i) {
                addEdge(it->second, i);
            }
            lastWriter[w] = i;
        }
    }

    // -- Stable topological sort (Kahn's algorithm with FIFO queue) ------
    // Using a FIFO queue preserves insertion order among passes at the
    // same dependency level, giving predictable results.

    std::queue<size_t> q;
    for (size_t i = 0; i < n; ++i) {
        if (inDeg[i] == 0) q.push(i);
    }

    order_.reserve(n);
    while (!q.empty()) {
        size_t cur = q.front();
        q.pop();
        order_.push_back(cur);

        for (size_t next : adj[cur]) {
            if (--inDeg[next] == 0) {
                q.push(next);
            }
        }
    }

    if (order_.size() != n) {
        KL_ERR("RenderGraph", "Dependency cycle detected (%zu of %zu passes sorted)",
               order_.size(), n);
        order_.clear();
        return false;
    }

    // -- Compute resource lifetimes in execution order -------------------
    for (int execIdx = 0; execIdx < static_cast<int>(order_.size()); ++execIdx) {
        const auto& pass = passes_[order_[execIdx]];

        for (const auto& w : pass.writes) {
            auto& lt = lifetimes_[w];
            if (lt.firstWrite < 0) lt.firstWrite = execIdx;
        }
        for (const auto& r : pass.reads) {
            lifetimes_[r].lastRead = execIdx;
        }
    }

    compiled_ = true;
    return true;
}

void RenderGraph::Execute() {
    if (!compiled_) {
        KL_WARN("RenderGraph", "Execute() called without a successful Compile()");
        return;
    }

    for (size_t idx : order_) {
        if (passes_[idx].execute) {
            passes_[idx].execute();
        }
    }
}

void RenderGraph::Clear() {
    passes_.clear();
    order_.clear();
    lifetimes_.clear();
    compiled_ = false;
}

std::vector<std::string> RenderGraph::GetExecutionOrder() const {
    std::vector<std::string> names;
    names.reserve(order_.size());
    for (size_t idx : order_) {
        names.push_back(passes_[idx].name);
    }
    return names;
}

const ResourceLifetime* RenderGraph::GetLifetime(const std::string& resource) const {
    auto it = lifetimes_.find(resource);
    return it != lifetimes_.end() ? &it->second : nullptr;
}

} // namespace koilo::rhi
