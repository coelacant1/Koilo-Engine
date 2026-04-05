#include <koilo/kernel/module_manager.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <koilo/kernel/engine_error.hpp>
#include <algorithm>
#include <queue>
#include <stdexcept>

namespace koilo {

ModuleManager::ModuleManager(MessageBus& bus, ServiceRegistry& services)
    : bus_(bus), services_(services) {
}

ModuleManager::~ModuleManager() {
    ShutdownAll();
}

ModuleId ModuleManager::RegisterModule(const ModuleDesc& desc, Cap grantedCaps) {
    if (!desc.name) return 0;

    // Check for duplicate registration
    if (nameIndex_.find(desc.name) != nameIndex_.end()) {
        KL_WARN("Kernel", "Module already registered: %s", desc.name);
        return 0;
    }

    ModuleId id = nextId_++;
    size_t idx = entries_.size();
    entries_.push_back({desc, id, ModuleState::Registered, grantedCaps});
    nameIndex_[desc.name] = idx;

    return id;
}

bool ModuleManager::InitializeAll(KoiloKernel& kernel) {
    kernel_ = &kernel;
    std::vector<size_t> order;
    if (!ResolveDependencies(order)) {
        return false;
    }

    initOrder_ = order;

    for (size_t idx : order) {
        auto& entry = entries_[idx];

        // Verify granted caps satisfy required caps
        if (!HasCap(entry.grantedCaps, entry.desc.requiredCaps)) {
            KL_ERR("Kernel", "Module '%s' requires capabilities not granted",
                   entry.desc.name);
            entry.state = ModuleState::Error;
            return false;
        }

        entry.state = ModuleState::Resolving;

        if (entry.desc.Init) {
            if (!entry.desc.Init(kernel)) {
                KL_ERR("Kernel", "Module '%s' initialization failed",
                       entry.desc.name);
                entry.state = ModuleState::Error;
                return false;
            }
        }

        entry.state = ModuleState::Running;

        // Subscribe module's OnMessage handler to the bus
        if (entry.desc.OnMessage) {
            auto handler = entry.desc.OnMessage;
            entry.busSubId = bus_.SubscribeAll([handler](const Message& msg) {
                handler(msg);
            });
        }

        // Notify other modules
        struct { const char* name; ModuleId id; } payload{entry.desc.name, entry.id};
        bus_.Send(MakeMessage(MSG_MODULE_LOADED, MODULE_KERNEL, payload));
    }

    return true;
}

void ModuleManager::TickAll(float dt) {
    for (size_t idx : initOrder_) {
        auto& entry = entries_[idx];

        // Tick cooldown timers for faulted modules
        if (entry.state == ModuleState::Faulted) {
            entry.fault.TickCooldown(dt);
            if (entry.fault.CooldownExpired() && entry.fault.ShouldRestart()) {
                RestartModule(entry.desc.name);
            }
            continue;
        }

        if (entry.state != ModuleState::Running || !entry.desc.Tick) continue;

        try {
            entry.desc.Tick(dt);
            // Successful tick clears consecutive fault counter
            if (entry.fault.consecutiveFaults > 0) {
                entry.fault.ClearFault();
            }
        } catch (const std::exception& e) {
            entry.fault.RecordFault(e.what());
            KL_ERR("Kernel", "Module '%s' faulted during Tick: %s",
                   entry.desc.name, e.what());

            if (kernel_) {
                kernel_->ReportError(ErrorSeverity::Degraded,
                                     ErrorSystem::Module,
                                     "Module '%s' faulted: %s",
                                     entry.desc.name, e.what());
            }

            if (entry.fault.IsPermanentlyFailed()) {
                entry.state = ModuleState::Error;
                KL_ERR("Kernel", "Module '%s' permanently disabled after %u faults",
                       entry.desc.name, entry.fault.totalFaults);
            } else {
                entry.state = ModuleState::Faulted;
                struct { const char* name; ModuleId id; } payload{entry.desc.name, entry.id};
                bus_.Send(MakeMessage(MSG_MODULE_FAULTED, MODULE_KERNEL, payload));
            }
        } catch (...) {
            entry.fault.RecordFault("unknown exception");
            entry.state = ModuleState::Error;
            KL_ERR("Kernel", "Module '%s' threw unknown exception; permanently disabled",
                   entry.desc.name);
        }
    }
}

bool ModuleManager::RestartModule(const std::string& name) {
    auto it = nameIndex_.find(name);
    if (it == nameIndex_.end()) return false;

    auto& entry = entries_[it->second];
    if (entry.state != ModuleState::Faulted) return false;
    if (!kernel_) return false;

    KL_LOG("Kernel", "Attempting restart of module '%s' (fault %u/%u)",
            entry.desc.name, entry.fault.consecutiveFaults,
            ModuleFaultPolicy::kMaxConsecutiveFaults);

    // Shutdown the faulted module
    if (entry.busSubId != 0) {
        bus_.Unsubscribe(entry.busSubId);
        entry.busSubId = 0;
    }
    if (entry.desc.Shutdown) {
        try { entry.desc.Shutdown(); } catch (...) {}
    }

    // Re-initialize
    if (entry.desc.Init && entry.desc.Init(*kernel_)) {
        entry.state = ModuleState::Running;

        if (entry.desc.OnMessage) {
            auto handler = entry.desc.OnMessage;
            entry.busSubId = bus_.SubscribeAll([handler](const Message& msg) {
                handler(msg);
            });
        }

        struct { const char* name; ModuleId id; } payload{entry.desc.name, entry.id};
        bus_.Send(MakeMessage(MSG_MODULE_RESTARTED, MODULE_KERNEL, payload));

        KL_LOG("Kernel", "Module '%s' restarted successfully", entry.desc.name);
        return true;
    }

    entry.state = ModuleState::Error;
    KL_ERR("Kernel", "Module '%s' restart failed; permanently disabled", entry.desc.name);
    return false;
}

void ModuleManager::ShutdownAll() {
    // Reverse initialization order
    for (auto it = initOrder_.rbegin(); it != initOrder_.rend(); ++it) {
        auto& entry = entries_[*it];
        if (entry.state == ModuleState::Running) {
            entry.state = ModuleState::ShuttingDown;

            // Unsubscribe module from the bus before shutdown
            if (entry.busSubId != 0) {
                bus_.Unsubscribe(entry.busSubId);
                entry.busSubId = 0;
            }

            if (entry.desc.Shutdown) {
                entry.desc.Shutdown();
            }
            entry.state = ModuleState::Unloaded;

            struct { const char* name; ModuleId id; } payload{entry.desc.name, entry.id};
            bus_.Send(MakeMessage(MSG_MODULE_UNLOADED, MODULE_KERNEL, payload));
        }
    }
    initOrder_.clear();
}

ModuleState ModuleManager::GetState(const std::string& name) const {
    auto it = nameIndex_.find(name);
    if (it == nameIndex_.end()) return ModuleState::Unloaded;
    return entries_[it->second].state;
}

ModuleId ModuleManager::GetId(const std::string& name) const {
    auto it = nameIndex_.find(name);
    if (it == nameIndex_.end()) return 0;
    return entries_[it->second].id;
}

Cap ModuleManager::GetCaps(ModuleId id) const {
    for (auto& e : entries_) {
        if (e.id == id) return e.grantedCaps;
    }
    return Cap::None;
}

bool ModuleManager::HasCapability(ModuleId id, Cap required) const {
#ifdef KL_BARE_METAL
    (void)id; (void)required;
    return true;
#else
    return HasCap(GetCaps(id), required);
#endif
}

void ModuleManager::GrantCap(ModuleId id, Cap cap) {
    for (auto& e : entries_) {
        if (e.id == id) { e.grantedCaps |= cap; return; }
    }
}

void ModuleManager::RevokeCap(ModuleId id, Cap cap) {
    for (auto& e : entries_) {
        if (e.id == id) { e.grantedCaps &= ~cap; return; }
    }
}

std::vector<ModuleManager::ModuleInfo> ModuleManager::ListModules() const {
    std::vector<ModuleInfo> result;
    result.reserve(entries_.size());
    for (auto& e : entries_) {
        result.push_back({e.desc.name, e.id, e.desc.version, e.state,
                          e.grantedCaps, e.desc.requiredCaps});
    }
    return result;
}

bool ModuleManager::ResolveDependencies(std::vector<size_t>& order) const {
    size_t n = entries_.size();
    // Build adjacency: in-degree per module
    std::vector<int> inDegree(n, 0);
    std::vector<std::vector<size_t>> adjList(n);

    for (size_t i = 0; i < n; ++i) {
        auto& entry = entries_[i];
        for (uint8_t d = 0; d < entry.desc.depCount; ++d) {
            const char* depName = entry.desc.dependencies[d];
            auto it = nameIndex_.find(depName);
            if (it == nameIndex_.end()) {
                KL_ERR("Kernel", "Module '%s' depends on unknown module '%s'",
                       entry.desc.name, depName);
                return false;
            }
            // depIdx must init before i
            adjList[it->second].push_back(i);
            ++inDegree[i];
        }
    }

    // Kahn's algorithm
    std::queue<size_t> ready;
    for (size_t i = 0; i < n; ++i) {
        if (inDegree[i] == 0) ready.push(i);
    }

    order.clear();
    order.reserve(n);
    while (!ready.empty()) {
        size_t cur = ready.front();
        ready.pop();
        order.push_back(cur);

        for (size_t neighbor : adjList[cur]) {
            if (--inDegree[neighbor] == 0) {
                ready.push(neighbor);
            }
        }
    }

    if (order.size() != n) {
        KL_ERR("Kernel", "Circular dependency detected among modules");
        return false;
    }

    return true;
}

} // namespace koilo
