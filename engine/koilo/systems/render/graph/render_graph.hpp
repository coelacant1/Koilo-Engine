// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file render_graph.hpp
 * @brief Lightweight render graph with pass DAG and resource lifetime tracking.
 *
 * Passes declare which virtual resources they read and write.  Compile()
 * builds a dependency DAG, runs a stable topological sort, and computes
 * resource lifetimes.  Execute() runs the passes in the compiled order.
 *
 * Virtual resources are string names (e.g. "offscreen", "swapchain") that
 * represent GPU textures, buffers, or other render targets.  The graph does
 * not allocate or manage the physical resources -- it only tracks ordering
 * and lifetimes so that the caller (or a future allocator) can make
 * informed decisions.
 *
 * @date 03/29/2026
 * @author Coela Can't
 */
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace koilo { class GPUTimingManager; }

namespace koilo::rhi {

/// A single pass in the render graph.
struct RenderPassNode {
    std::string              name;
    std::vector<std::string> reads;
    std::vector<std::string> writes;
    std::function<void()>    execute;
};

/// Resource lifetime expressed as execution-order indices.
struct ResourceLifetime {
    int firstWrite = -1;   ///< Execution-order index of the first pass that writes this resource.
    int lastRead   = -1;   ///< Execution-order index of the last pass that reads this resource.
};

/**
 * @class RenderGraph
 * @brief Lightweight single-queue render graph.
 *
 * Usage:
 * @code
 *   RenderGraph graph;
 *   graph.AddPass("scene", {}, {"offscreen"}, [&]{ ... });
 *   graph.AddPass("blit",  {"offscreen"}, {"swapchain"}, [&]{ ... });
 *   graph.Compile();
 *   graph.Execute();
 *   graph.Clear();
 * @endcode
 */
class RenderGraph {
public:
    RenderGraph() = default;

    /// Add a pass.  Insertion order is used as a tiebreaker for passes
    /// at the same dependency level (stable topological sort).
    void AddPass(std::string              name,
                 std::vector<std::string> reads,
                 std::vector<std::string> writes,
                 std::function<void()>    execute);

    /// Build the dependency DAG and compute the execution order.
    /// Returns false if the graph contains a cycle (programming error).
    bool Compile();

    /// Run all passes in compiled order.  Requires a successful Compile().
    void Execute();

    /// Run all passes with optional GPU timing instrumentation.
    /// If @p gpuTiming is non-null and enabled, wraps each pass with
    /// BeginPass/EndPass timestamp queries.
    void Execute(GPUTimingManager* gpuTiming);

    /// Reset the graph for the next frame (clears passes and compiled state).
    void Clear();

    /// @name Post-Compile queries
    /// @{
    bool   IsCompiled() const { return compiled_; }
    size_t PassCount()  const { return passes_.size(); }

    /// Return the execution order as a list of pass names.
    std::vector<std::string> GetExecutionOrder() const;

    /// Query the lifetime of a virtual resource (or nullptr if unknown).
    const ResourceLifetime* GetLifetime(const std::string& resource) const;

    /// Return all tracked resource lifetimes.
    const std::unordered_map<std::string, ResourceLifetime>& GetLifetimes() const {
        return lifetimes_;
    }
    /// @}

private:
    std::vector<RenderPassNode> passes_;
    std::vector<size_t>         order_;       ///< Indices into passes_ in execution order.
    std::unordered_map<std::string, ResourceLifetime> lifetimes_;
    bool compiled_ = false;
};

} // namespace koilo::rhi
