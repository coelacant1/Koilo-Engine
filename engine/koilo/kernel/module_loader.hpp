// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file module_loader.hpp
 * @brief Module loader for registration, lifecycle, and dynamic loading.
 *
 * @date 01/28/2026
 * @author Coela
 */
#pragma once

#include <koilo/kernel/module_api.hpp>
#include <koilo/kernel/module_fault.hpp>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include "../registry/reflect_macros.hpp"

namespace koilo {

class KoiloKernel;

// Manages engine modules - registration, lifecycle, and dynamic loading.
///
// Modules are sorted by phase (Core -> System -> Render -> Overlay) so
// initialization and rendering happen in the correct order.
class ModuleLoader {
public:
    enum class LoadMode { Eager, Lazy };

    // Register a statically-linked module.
    void Register(std::unique_ptr<IModule> module) {
        modules_.push_back(std::move(module));
        sorted_ = false;
    }

    // Initialize all registered modules via kernel in phase order.
    void InitializeAll(KoiloKernel& kernel) {
        kernel_ = &kernel;
        SortIfNeeded();
        for (auto& m : modules_) {
            if (m->Initialize(kernel)) {
                initialized_.push_back(m.get());
            }
        }
    }

    // Update all initialized modules (with fault isolation).
    void UpdateAll(float dt) {
        for (auto it = initialized_.begin(); it != initialized_.end(); ++it) {
            auto* m = *it;
            auto& fr = faults_[m];

            if (fr.consecutiveFaults > 0 && fr.faultCooldown > 0.0f) {
                fr.TickCooldown(dt);
                continue;  // Skip faulted modules during cooldown
            }

            try {
                m->Update(dt);
                if (fr.consecutiveFaults > 0) fr.ClearFault();
            } catch (const std::exception& e) {
                HandleModuleFault(m, fr, e.what());
            } catch (...) {
                HandleModuleFault(m, fr, "unknown exception");
            }
        }
        ProcessPendingRestarts();
    }

    // Render all initialized modules (with fault isolation).
    void RenderAll(Color888* buffer, int width, int height) {
        for (auto* m : initialized_) {
            auto& fr = faults_[m];
            if (fr.consecutiveFaults > 0) continue;  // Skip faulted

            try {
                m->Render(buffer, width, height);
            } catch (const std::exception& e) {
                HandleModuleFault(m, fr, e.what());
            } catch (...) {
                HandleModuleFault(m, fr, "unknown exception");
            }
        }
    }

    // Shutdown all modules in reverse order.
    void ShutdownAll() {
        for (auto it = initialized_.rbegin(); it != initialized_.rend(); ++it) {
            (*it)->Shutdown();
        }
        initialized_.clear();
        modules_.clear();
        kernel_ = nullptr;
    }

    // Check if a module is loaded and initialized.
    bool HasModule(const std::string& name) const {
        for (auto* m : initialized_) {
            if (m->GetInfo().name == name) return true;
        }
        return false;
    }

    // List all initialized module names.
    std::vector<std::string> ListModules() const {
        std::vector<std::string> names;
        for (auto* m : initialized_) {
            names.push_back(m->GetInfo().name);
        }
        return names;
    }

    // Get a module by name (nullptr if not found).
    IModule* GetModule(const std::string& name) {
        for (auto* m : initialized_) {
            if (m->GetInfo().name == name) return m;
        }
        return nullptr;
    }

    // Unload a single module by name. Calls Shutdown() and removes it.
    // Script globals registered by the module will become inaccessible.
    // Returns true if the module was found and unloaded.
    bool UnloadModule(const std::string& name) {
        // Remove from initialized list
        for (auto it = initialized_.begin(); it != initialized_.end(); ++it) {
            if ((*it)->GetInfo().name == name) {
                (*it)->Shutdown();
                initialized_.erase(it);
                break;
            }
        }
        // Remove from modules list (destroys the unique_ptr)
        for (auto it = modules_.begin(); it != modules_.end(); ++it) {
            if ((*it)->GetInfo().name == name) {
                modules_.erase(it);
                return true;
            }
        }
        return false;
    }

    // Load a dynamic module from a shared library (.so/.dll).
    // Returns true if loaded successfully.
    bool LoadFromLibrary(const std::string& path);

    // Scan a directory for .so/.dll/.kmod files and load them.
    int ScanAndLoad(const std::string& directory);

    // Try to load a module by name from the search directory (lazy loading).
    // If found and loaded, initializes it and returns true.
    bool TryLoad(const std::string& name);

    // Set the directory to search for dynamic modules.
    void SetModuleSearchPath(const std::string& path) { searchPath_ = path; }
    const std::string& GetModuleSearchPath() const { return searchPath_; }

    // Set module loading strategy (Eager = load all at boot, Lazy = load on first reference).
    void SetLoadMode(LoadMode mode) { loadMode_ = mode; }
    LoadMode GetLoadMode() const { return loadMode_; }

    // Reload a module from its library path (hot-reload, desktop only).
    // Shuts down the old instance, dlclose, dlopen, re-initialize.
    bool ReloadModule(const std::string& name);

    // Attempt to restart a faulted module by name (shutdown + re-init).
    bool RestartFaultedModule(const std::string& name);

    // Get fault record for a module (nullptr if not tracked).
    const ModuleFaultRecord* GetFaultRecord(const std::string& name) const {
        for (auto* m : initialized_) {
            if (m->GetInfo().name == name) {
                auto it = faults_.find(m);
                return (it != faults_.end()) ? &it->second : nullptr;
            }
        }
        return nullptr;
    }

    // Check all loaded dynamic modules for file changes.
    // Returns the number of modules reloaded.
    int CheckAndReload();

private:
    void SortIfNeeded() {
        if (sorted_) return;
        std::sort(modules_.begin(), modules_.end(),
            [](const auto& a, const auto& b) {
                return static_cast<uint8_t>(a->GetInfo().phase) <
                       static_cast<uint8_t>(b->GetInfo().phase);
            });
        sorted_ = true;
    }

    std::vector<std::unique_ptr<IModule>> modules_;
    std::vector<IModule*> initialized_;  // raw ptrs into modules_
    std::unordered_map<IModule*, ModuleFaultRecord> faults_;
    std::vector<IModule*> pendingRestarts_;
    bool sorted_ = false;
    KoiloKernel* kernel_ = nullptr;
    std::string searchPath_;
    LoadMode loadMode_ = LoadMode::Eager;

    void HandleModuleFault(IModule* m, ModuleFaultRecord& fr, const char* error);
    void ProcessPendingRestarts();

    KL_BEGIN_FIELDS(ModuleLoader)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(ModuleLoader)
        KL_METHOD_AUTO(ModuleLoader, UpdateAll, "Update all"),
        KL_METHOD_AUTO(ModuleLoader, RenderAll, "Render all"),
        KL_METHOD_AUTO(ModuleLoader, ShutdownAll, "Shutdown all"),
        KL_METHOD_AUTO(ModuleLoader, HasModule, "Has module"),
        KL_METHOD_AUTO(ModuleLoader, ListModules, "List modules"),
        KL_METHOD_AUTO(ModuleLoader, GetModule, "Get module"),
        KL_METHOD_AUTO(ModuleLoader, UnloadModule, "Unload module"),
        KL_METHOD_AUTO(ModuleLoader, LoadFromLibrary, "Load from library"),
        KL_METHOD_AUTO(ModuleLoader, ScanAndLoad, "Scan and load"),
        KL_METHOD_AUTO(ModuleLoader, TryLoad, "Try load"),
        KL_METHOD_AUTO(ModuleLoader, SetModuleSearchPath, "Set module search path"),
        KL_METHOD_AUTO(ModuleLoader, GetModuleSearchPath, "Get module search path"),
        KL_METHOD_AUTO(ModuleLoader, SetLoadMode, "Set load mode"),
        KL_METHOD_AUTO(ModuleLoader, GetLoadMode, "Get load mode"),
        KL_METHOD_AUTO(ModuleLoader, ReloadModule, "Reload module"),
        KL_METHOD_AUTO(ModuleLoader, CheckAndReload, "Check and reload")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ModuleLoader)
        /* No reflected ctors. */
    KL_END_DESCRIBE(ModuleLoader)

};

} // namespace koilo
