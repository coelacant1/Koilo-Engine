// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "../registry/reflect_macros.hpp"

namespace koilo {
namespace scripting {

/**
 * @brief Registry for script-defined signals.
 * 
 * Supports connect/disconnect/emit pattern for decoupled communication.
 * Signal names are registered at load time; listeners are function names
 * resolved at emit time by the bytecode VM.
 */
class SignalRegistry {
public:
    // Register a signal name (from `signal` declaration)
    void DeclareSignal(const std::string& name) {
        signals_[name]; // create entry if not exists
    }

    // Check if a signal is declared
    bool HasSignal(const std::string& name) const {
        return signals_.count(name) > 0;
    }

    // Connect a handler function to a signal
    void Connect(const std::string& signalName, const std::string& handlerName) {
        signals_[signalName].push_back({handlerName, false});
    }

    // Connect a one-shot handler (auto-disconnects after first emit)
    void ConnectOnce(const std::string& signalName, const std::string& handlerName) {
        signals_[signalName].push_back({handlerName, true});
    }

    // Disconnect a handler from a signal
    void Disconnect(const std::string& signalName, const std::string& handlerName) {
        auto it = signals_.find(signalName);
        if (it == signals_.end()) return;
        auto& listeners = it->second;
        listeners.erase(
            std::remove_if(listeners.begin(), listeners.end(),
                [&](const Listener& l) { return l.handlerName == handlerName; }),
            listeners.end());
    }

    // Get all handlers for a signal, clearing one-shot entries
    std::vector<std::string> GetHandlers(const std::string& signalName) {
        auto it = signals_.find(signalName);
        if (it == signals_.end()) return {};
        
        std::vector<std::string> result;
        auto& listeners = it->second;
        for (auto& l : listeners) {
            result.push_back(l.handlerName);
        }
        // Remove one-shot listeners
        listeners.erase(
            std::remove_if(listeners.begin(), listeners.end(),
                [](const Listener& l) { return l.oneShot; }),
            listeners.end());
        return result;
    }

    void Clear() { signals_.clear(); }

private:
    struct Listener {
        std::string handlerName;
        bool oneShot = false;

        KL_BEGIN_FIELDS(Listener)
            KL_FIELD(Listener, handlerName, "Handler name", 0, 0),
            KL_FIELD(Listener, oneShot, "One shot", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(Listener)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(Listener)
            /* No reflected ctors. */
        KL_END_DESCRIBE(Listener)

    };
    std::unordered_map<std::string, std::vector<Listener>> signals_;

    KL_BEGIN_FIELDS(SignalRegistry)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(SignalRegistry)
        KL_METHOD_AUTO(SignalRegistry, DeclareSignal, "Declare signal"),
        KL_METHOD_AUTO(SignalRegistry, HasSignal, "Has signal"),
        KL_METHOD_AUTO(SignalRegistry, Connect, "Connect"),
        KL_METHOD_AUTO(SignalRegistry, ConnectOnce, "Connect once"),
        KL_METHOD_AUTO(SignalRegistry, Disconnect, "Disconnect"),
        KL_METHOD_AUTO(SignalRegistry, GetHandlers, "Get handlers"),
        KL_METHOD_AUTO(SignalRegistry, Clear, "Clear")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SignalRegistry)
        /* No reflected ctors. */
    KL_END_DESCRIBE(SignalRegistry)

};

} // namespace scripting
} // namespace koilo
