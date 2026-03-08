// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file coroutine.hpp
 * @brief Coroutine support for KoiloScript  suspendable multi-frame execution.
 *
 * Coroutines allow script functions to yield execution and resume next frame,
 * enabling animation sequences, timed events, and async-style patterns.
 *
 * Usage from script:
 *   start_coroutine("MySequence");
 *   fn MySequence() {
 *       // do something
 *       yield;
 *       // continues next frame
 *       yield;
 *       // continues the frame after
 *   }
 */
#pragma once

#include <koilo/scripting/bytecode.hpp>
#include <koilo/scripting/nanboxed_value.hpp>
#include <koilo/scripting/script_class.hpp>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <memory>
#include "../registry/reflect_macros.hpp"

namespace koilo {
namespace scripting {

/**
 * @brief Snapshot of VM state for a suspended coroutine.
 */
struct CoroutineState {
    static constexpr int MAX_STACK = 256;
    static constexpr int MAX_FRAMES = 64;

    CoroutineState() = default;
    CoroutineState(CoroutineState&&) = default;
    CoroutineState& operator=(CoroutineState&&) = default;
    CoroutineState(const CoroutineState&) = delete;
    CoroutineState& operator=(const CoroutineState&) = delete;

    struct SavedFrame {
        const BytecodeChunk* chunk = nullptr;
        size_t ip = 0;
        int stackBase = 0;

        KL_BEGIN_FIELDS(SavedFrame)
            KL_FIELD(SavedFrame, chunk, "Chunk", 0, 0),
            KL_FIELD(SavedFrame, ip, "Ip", 0, 0),
            KL_FIELD(SavedFrame, stackBase, "Stack base", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(SavedFrame)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(SavedFrame)
            /* No reflected ctors. */
        KL_END_DESCRIBE(SavedFrame)

    };

    struct SavedIterator {
        std::vector<NanBoxedValue> values;
        size_t index = 0;

        KL_BEGIN_FIELDS(SavedIterator)
            KL_FIELD(SavedIterator, values, "Values", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(SavedIterator)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(SavedIterator)
            /* No reflected ctors. */
        KL_END_DESCRIBE(SavedIterator)

    };

    std::string name;                        // Function name
    SavedFrame frames[MAX_FRAMES];
    int frameCount = 0;
    NanBoxedValue stack[MAX_STACK];
    int stackTop = 0;
    std::vector<SavedIterator> iterators;
    bool finished = false;                   // Set when function returns normally
    int waitFrames = 0;                      // If > 0, skip this many frames before resuming

    // Heap pools owned by this coroutine
    std::deque<std::string> ownedStrings;
    std::deque<HeapArray> ownedArrays;
    std::deque<std::unordered_map<std::string, Value>> ownedMaps;
    std::vector<std::unique_ptr<ScriptInstance>> ownedInstances;

    KL_BEGIN_FIELDS(CoroutineState)
        KL_FIELD(CoroutineState, name, "Name", 0, 0),
        KL_FIELD(CoroutineState, iterators, "Iterators", 0, 0),
        KL_FIELD(CoroutineState, ownedArrays, "Owned arrays", 0, 0),
        KL_FIELD(CoroutineState, ownedMaps, "Owned maps", 0, 0),
        KL_FIELD(CoroutineState, ownedInstances, "Owned instances", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(CoroutineState)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(CoroutineState)
        KL_CTOR0(CoroutineState)
    KL_END_DESCRIBE(CoroutineState)

};

/**
 * @brief Manages active coroutines  resumes each per frame in Update().
 */
class CoroutineManager {
public:
    /**
     * @brief Start a new coroutine from a named function.
     * @param name Function name to run as coroutine.
     */
    void Start(const std::string& name);

    /**
     * @brief Resume all active coroutines. Called once per frame from ExecuteUpdate().
     */
    void ResumeAll();

    /**
     * @brief Check if any coroutines are active.
     */
    bool HasActive() const { return !coroutines_.empty(); }

    /**
     * @brief Get count of active coroutines.
     */
    size_t Count() const { return coroutines_.size(); }

    /**
     * @brief Clear all coroutines (e.g., on scene change).
     */
    void Clear() { coroutines_.clear(); pending_.clear(); }

    /**
     * @brief Access pending starts (consumed by VM during resume).
     */
    std::vector<std::string>& PendingStarts() { return pending_; }

    /**
     * @brief Access active coroutine states.
     */
    std::vector<CoroutineState>& Active() { return coroutines_; }

    /**
     * @brief Stop a coroutine by function name.
     * @return true if a matching coroutine was found and stopped.
     */
    bool StopByName(const std::string& name) {
        for (auto& co : coroutines_) {
            if (co.name == name) {
                co.finished = true;
                return true;
            }
        }
        return false;
    }

private:
    std::vector<CoroutineState> coroutines_;
    std::vector<std::string> pending_;   // Functions to start next ResumeAll()

    KL_BEGIN_FIELDS(CoroutineManager)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(CoroutineManager)
        KL_METHOD_AUTO(CoroutineManager, Start, "Start"),
        KL_METHOD_AUTO(CoroutineManager, ResumeAll, "Resume all"),
        KL_METHOD_AUTO(CoroutineManager, HasActive, "Has active"),
        KL_METHOD_AUTO(CoroutineManager, Count, "Count"),
        KL_METHOD_AUTO(CoroutineManager, Clear, "Clear"),
        KL_METHOD_AUTO(CoroutineManager, StopByName, "Stop by name"),
        KL_METHOD_AUTO(CoroutineManager, PendingStarts, "Pending starts"),
        KL_METHOD_AUTO(CoroutineManager, Active, "Active")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(CoroutineManager)
        /* No reflected ctors. */
    KL_END_DESCRIBE(CoroutineManager)

};

} // namespace scripting
} // namespace koilo
