// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file coroutine.cpp
 * @brief CoroutineManager implementation.
 */
#include <koilo/scripting/coroutine.hpp>
#include <algorithm>

namespace koilo {
namespace scripting {

void CoroutineManager::Start(const std::string& name) {
    pending_.push_back(name);
}

void CoroutineManager::ResumeAll() {
    // Remove finished coroutines
    coroutines_.erase(
        std::remove_if(coroutines_.begin(), coroutines_.end(),
                       [](const CoroutineState& c) { return c.finished; }),
        coroutines_.end());

    // Decrement wait counters, skip those still waiting
    for (auto& co : coroutines_) {
        if (co.waitFrames > 0) {
            co.waitFrames--;
        }
    }
}

} // namespace scripting
} // namespace koilo
