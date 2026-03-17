// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file module_manager.hpp
 * @brief Module registration, dependency resolution, and lifecycle management.
 *
 * @date 11/29/2025
 * @author Coela
 */
#pragma once
#include <koilo/kernel/module.hpp>
#include <koilo/kernel/capabilities.hpp>
#include <koilo/kernel/memory/arena_allocator.hpp>
#include <koilo/kernel/message_bus.hpp>
#include <koilo/kernel/service_registry.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include "../registry/reflect_macros.hpp"

namespace koilo {

/// Manages module registration, dependency resolution, lifecycle, and capabilities.
class ModuleManager {
public:
    ModuleManager(MessageBus& bus, ServiceRegistry& services);
    ~ModuleManager();

    ModuleManager(const ModuleManager&) = delete;
    ModuleManager& operator=(const ModuleManager&) = delete;

    /// Register a module descriptor. Returns the assigned ModuleId.
    ModuleId RegisterModule(const ModuleDesc& desc, Cap grantedCaps = CAP_TRUSTED);

    /// Resolve dependencies and initialize all registered modules in order.
    /// Returns false if dependency resolution fails (prints diagnostics).
    bool InitializeAll(KoiloKernel& kernel);

    /// Tick all initialized modules.
    void TickAll(float dt);

    /// Shut down all modules in reverse initialization order.
    void ShutdownAll();

    /// Get module state by name.
    ModuleState GetState(const std::string& name) const;

    /// Get module ID by name. Returns 0 if not found.
    ModuleId GetId(const std::string& name) const;

    /// Get capabilities granted to a module.
    Cap GetCaps(ModuleId id) const;

    /// Check if a module has a specific capability.
    bool HasCapability(ModuleId id, Cap required) const;

    /// Grant additional capabilities to a module.
    void GrantCap(ModuleId id, Cap cap);

    /// Revoke capabilities from a module.
    void RevokeCap(ModuleId id, Cap cap);

    struct ModuleInfo {
        std::string  name;
        ModuleId     id;
        uint32_t     version;
        ModuleState  state;
        Cap          grantedCaps;
        Cap          requiredCaps;

        KL_BEGIN_FIELDS(ModuleInfo)
            KL_FIELD(ModuleInfo, name, "Name", 0, 0),
            KL_FIELD(ModuleInfo, id, "Id", 0, 0),
            KL_FIELD(ModuleInfo, version, "Version", 0, 4294967295),
            KL_FIELD(ModuleInfo, state, "State", 0, 0),
            KL_FIELD(ModuleInfo, grantedCaps, "Granted caps", 0, 0),
            KL_FIELD(ModuleInfo, requiredCaps, "Required caps", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(ModuleInfo)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(ModuleInfo)
            /* No reflected ctors. */
        KL_END_DESCRIBE(ModuleInfo)

    };

    /// List all registered modules.
    std::vector<ModuleInfo> ListModules() const;

    size_t ModuleCount() const { return entries_.size(); }

private:
    struct ModuleEntry {
        ModuleDesc  desc;
        ModuleId    id;
        ModuleState state;
        Cap         grantedCaps;

        KL_BEGIN_FIELDS(ModuleEntry)
            KL_FIELD(ModuleEntry, desc, "Desc", 0, 0),
            KL_FIELD(ModuleEntry, id, "Id", 0, 0),
            KL_FIELD(ModuleEntry, state, "State", 0, 0),
            KL_FIELD(ModuleEntry, grantedCaps, "Granted caps", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(ModuleEntry)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(ModuleEntry)
            /* No reflected ctors. */
        KL_END_DESCRIBE(ModuleEntry)

    };

    /// Topological sort of modules by dependency order.
    bool ResolveDependencies(std::vector<size_t>& order) const;

    std::vector<ModuleEntry> entries_;
    std::unordered_map<std::string, size_t> nameIndex_;
    std::vector<size_t> initOrder_;
    ModuleId nextId_ = 1; // 0 = kernel
    MessageBus& bus_;
    ServiceRegistry& services_;

    KL_BEGIN_FIELDS(ModuleManager)
        /* Internal state - not reflectable. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(ModuleManager)
        KL_METHOD_AUTO(ModuleManager, ModuleCount, "Module count")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ModuleManager)
        /* Non-copyable, requires references - no reflected ctors. */
    KL_END_DESCRIBE(ModuleManager)

};

} // namespace koilo
