// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file service_registry.hpp
 * @brief Runtime service discovery for decoupled module communication.
 *
 * @date 11/12/2025
 * @author Coela
 */
#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace koilo {

class MessageBus;

/// Runtime service discovery. Modules register named services;
/// other modules discover them without compile-time coupling.
class ServiceRegistry {
public:
    ServiceRegistry() = default;
    ~ServiceRegistry() = default;

    ServiceRegistry(const ServiceRegistry&) = delete;
    ServiceRegistry& operator=(const ServiceRegistry&) = delete;

    /// Attach a message bus for service lifecycle events.
    void SetBus(MessageBus* bus) { bus_ = bus; }

    /// Register a service. Overwrites if name already registered.
    void Register(const std::string& name, void* service);

    /// Remove a service registration.
    void Unregister(const std::string& name);

    /// Get a service by name. Returns nullptr if not found.
    template<typename T>
    T* Get(const std::string& name) const {
        auto it = services_.find(name);
        return it != services_.end() ? static_cast<T*>(it->second) : nullptr;
    }

    /// Non-typed get (returns void*). Returns nullptr if not found.
    void* GetRaw(const std::string& name) const;

    /// Check if a service is registered.
    bool Has(const std::string& name) const;

    /// List all registered service names.
    std::vector<std::string> List() const;

    /// Number of registered services.
    size_t Count() const { return services_.size(); }

private:
    std::unordered_map<std::string, void*> services_;
    MessageBus* bus_ = nullptr;
};

} // namespace koilo
