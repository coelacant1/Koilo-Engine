// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/kernel/module_loader.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/module_abi_adapters.hpp>
#include <koilo/kernel/engine_error.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <koilo/core/interfaces/iscript_bridge.hpp>

#ifdef __linux__
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>

namespace koilo {

// Adapter that wraps C ABI function pointers as an IModule.
class DynamicModuleAdapter : public IModule {
public:
    DynamicModuleAdapter(void* handle, KoiloModuleHeader* header,
                         KoiloModuleInitFunc init, KoiloModuleUpdateFunc update,
                         KoiloModuleRenderFunc render, KoiloModuleShutdownFunc shutdown,
                         const std::string& libPath = "")
        : handle_(handle), header_(header)
        , initFunc_(init), updateFunc_(update)
        , renderFunc_(render), shutdownFunc_(shutdown)
        , libraryPath_(libPath), loadTime_(std::time(nullptr)) {}

    ~DynamicModuleAdapter() override {
#ifdef __linux__
        if (handle_) dlclose(handle_);
#elif defined(_WIN32)
        if (handle_) FreeLibrary((HMODULE)handle_);
#endif
    }

    ModuleInfo GetInfo() const override {
        return {
            header_->name,
            header_->version,
            static_cast<ModulePhase>(header_->phase)
        };
    }

    bool Initialize(KoiloKernel& kernel) override {
        kernel_ = &kernel;
        if (!initFunc_) return false;

        // Build EngineServices for C modules
        // The engine pointer passed to all callbacks is KoiloKernel*
        services_.api_size = sizeof(EngineServices);
        services_.register_global = [](void* eng, const char* name, const char* cls, void* inst) {
            auto* k = static_cast<KoiloKernel*>(eng);
            auto* bridge = k->Services().Get<IScriptBridge>("script_bridge");
            if (bridge) bridge->RegisterGlobal(name, cls, inst);
        };
        services_.get_global   = nullptr;
        services_.set_variable = [](void* eng, const char* name, double value) {
            auto* k = static_cast<KoiloKernel*>(eng);
            auto* bridge = k->Services().Get<IScriptBridge>("script_bridge");
            if (bridge) bridge->SetScriptVariable(name, value);
        };
        services_.get_variable = [](void* eng, const char* name) -> double {
            auto* k = static_cast<KoiloKernel*>(eng);
            auto* bridge = k->Services().Get<IScriptBridge>("script_bridge");
            return bridge ? bridge->GetScriptVariableNum(name) : 0.0;
        };
        services_.log_info  = [](const char*) {};
        services_.log_error = [](const char*) {};
        services_.alloc   = [](uint32_t size) -> void* { return ::malloc(size); };
        services_.dealloc = [](void* ptr) { ::free(ptr); };
        services_.get_time       = nullptr;
        services_.get_delta_time = nullptr;
        services_.get_pixel_buffer  = nullptr;
        services_.get_buffer_width  = nullptr;
        services_.get_buffer_height = nullptr;
        services_.register_class   = nullptr;
        services_.register_exports = nullptr;

        // ABI v3 extension point registration
        services_.register_command        = &koilo::AbiRegisterCommand;
        services_.register_input_listener = &koilo::AbiRegisterInputListener;
        services_.register_component      = &koilo::AbiRegisterComponent;
        services_.register_widget_type    = &koilo::AbiRegisterWidgetType;
        services_.register_render_pass    = &koilo::AbiRegisterRenderPass;
        std::memset(services_.reserved_v3, 0, sizeof(services_.reserved_v3));

        return initFunc_(&services_, kernel_) != 0;
    }

    void Update(float dt) override {
        if (updateFunc_) updateFunc_(dt);
    }

    void Render(Color888* buffer, int width, int height) override {
        if (renderFunc_) renderFunc_(buffer, width, height);
    }

    void Shutdown() override {
        if (shutdownFunc_) shutdownFunc_();
    }

    std::string GetLibraryPath() const { return libraryPath_; }
    time_t GetLoadTime() const { return loadTime_; }

private:
    void* handle_;
    KoiloModuleHeader* header_;
    KoiloModuleInitFunc initFunc_;
    KoiloModuleUpdateFunc updateFunc_;
    KoiloModuleRenderFunc renderFunc_;
    KoiloModuleShutdownFunc shutdownFunc_;
    EngineServices services_{};
    std::string libraryPath_;
    time_t loadTime_ = 0;
};

bool ModuleLoader::LoadFromLibrary(const std::string& path) {
#ifdef __linux__
    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if (!handle) return false;

    auto getHeader = (KoiloModuleGetHeaderFunc)dlsym(handle, "koilo_module_get_header");
    auto init      = (KoiloModuleInitFunc)dlsym(handle, "koilo_module_init");
    if (!getHeader || !init) { dlclose(handle); return false; }

    auto* header = getHeader();
    if (!header || header->magic != KL_MODULE_MAGIC) { dlclose(handle); return false; }

    auto update   = (KoiloModuleUpdateFunc)dlsym(handle, "koilo_module_update");
    auto render   = (KoiloModuleRenderFunc)dlsym(handle, "koilo_module_render");
    auto shutdown = (KoiloModuleShutdownFunc)dlsym(handle, "koilo_module_shutdown");

    Register(std::make_unique<DynamicModuleAdapter>(
        handle, header, init, update, render, shutdown, path));
    return true;

#elif defined(_WIN32)
    HMODULE handle = LoadLibraryA(path.c_str());
    if (!handle) return false;

    auto getHeader = (KoiloModuleGetHeaderFunc)GetProcAddress(handle, "koilo_module_get_header");
    auto init      = (KoiloModuleInitFunc)GetProcAddress(handle, "koilo_module_init");
    if (!getHeader || !init) { FreeLibrary(handle); return false; }

    auto* header = getHeader();
    if (!header || header->magic != KL_MODULE_MAGIC) { FreeLibrary(handle); return false; }

    auto update   = (KoiloModuleUpdateFunc)GetProcAddress(handle, "koilo_module_update");
    auto render   = (KoiloModuleRenderFunc)GetProcAddress(handle, "koilo_module_render");
    auto shutdown = (KoiloModuleShutdownFunc)GetProcAddress(handle, "koilo_module_shutdown");

    Register(std::make_unique<DynamicModuleAdapter>(
        handle, header, init, update, render, shutdown, path));
    return true;
#else
    return false;
#endif
}

int ModuleLoader::ScanAndLoad(const std::string& directory) {
    int loaded = 0;
#ifdef __linux__
    DIR* dir = opendir(directory.c_str());
    if (!dir) return 0;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name(entry->d_name);
        if (name.size() > 3 && name.substr(name.size() - 3) == ".so") {
            std::string fullPath = directory + "/" + name;
            if (LoadFromLibrary(fullPath)) loaded++;
        }
    }
    closedir(dir);
#endif
    return loaded;
}

bool ModuleLoader::TryLoad(const std::string& name) {
    // Already loaded?
    if (HasModule(name)) return true;
    if (searchPath_.empty()) return false;

#ifdef __linux__
    // Try common naming patterns: name.so, libkoilo_name.so, koilo_name.so
    const std::string candidates[] = {
        searchPath_ + "/" + name + ".so",
        searchPath_ + "/libkoilo_" + name + ".so",
        searchPath_ + "/koilo_" + name + ".so"
    };
    for (auto& path : candidates) {
        if (LoadFromLibrary(path)) {
            // Initialize the newly loaded module
            if (kernel_ && !modules_.empty()) {
                auto& m = modules_.back();
                if (m->Initialize(*kernel_)) {
                    initialized_.push_back(m.get());
                    return true;
                }
            }
            return false;
        }
    }
#endif
    return false;
}

bool ModuleLoader::ReloadModule(const std::string& name) {
#ifdef __linux__
    // Find the module to reload
    for (size_t i = 0; i < modules_.size(); ++i) {
        if (modules_[i]->GetInfo().name == name) {
            // Remove from initialized list
            auto it = std::find(initialized_.begin(), initialized_.end(), modules_[i].get());
            if (it != initialized_.end()) {
                (*it)->Shutdown();
                initialized_.erase(it);
            }

            // DynamicModuleAdapter destructor calls dlclose
            auto* adapter = dynamic_cast<DynamicModuleAdapter*>(modules_[i].get());
            if (!adapter) return false;  // Static modules can't be reloaded

            std::string libPath = adapter->GetLibraryPath();
            if (libPath.empty()) return false;

            // Remove old module
            modules_.erase(modules_.begin() + i);

            // Load fresh copy
            if (LoadFromLibrary(libPath) && kernel_ && !modules_.empty()) {
                auto& m = modules_.back();
                if (m->Initialize(*kernel_)) {
                    initialized_.push_back(m.get());
                    return true;
                }
            }
            return false;
        }
    }
#endif
    return false;
}

int ModuleLoader::CheckAndReload() {
    int reloaded = 0;
#ifdef __linux__
    // Check modification times for each dynamic module
    for (size_t i = 0; i < modules_.size(); ++i) {
        auto* adapter = dynamic_cast<DynamicModuleAdapter*>(modules_[i].get());
        if (!adapter) continue;

        std::string path = adapter->GetLibraryPath();
        if (path.empty()) continue;

        struct stat st;
        if (stat(path.c_str(), &st) != 0) continue;

        time_t currentMtime = st.st_mtime;
        if (currentMtime > adapter->GetLoadTime()) {
            std::string name = adapter->GetInfo().name;
            if (ReloadModule(name)) reloaded++;
        }
    }
#endif
    return reloaded;
}

void ModuleLoader::HandleModuleFault(IModule* m, ModuleFaultRecord& fr,
                                     const char* error) {
    auto info = m->GetInfo();
    fr.RecordFault(error);

    KL_ERR("Kernel", "Module '%s' faulted during Update/Render: %s",
           info.name, error);

    if (kernel_) {
        kernel_->ReportError(ErrorSeverity::Degraded,
                             ErrorSystem::Module,
                             "Module '%s' faulted: %s",
                             info.name, error);

        struct { const char* name; } payload{info.name};
        kernel_->Messages().Send(MakeMessage(MSG_MODULE_FAULTED, MODULE_KERNEL, payload));
    }

    if (fr.IsPermanentlyFailed()) {
        KL_ERR("Kernel", "Module '%s' permanently disabled after %u faults",
               info.name, fr.totalFaults);
    } else {
        pendingRestarts_.push_back(m);
    }
}

void ModuleLoader::ProcessPendingRestarts() {
    if (pendingRestarts_.empty()) return;

    for (auto* m : pendingRestarts_) {
        auto& fr = faults_[m];
        if (!fr.CooldownExpired() || !fr.ShouldRestart()) continue;

        auto info = m->GetInfo();
        KL_LOG("Kernel", "Attempting restart of module '%s' (fault %u/%u)",
                info.name, fr.consecutiveFaults,
                ModuleFaultPolicy::kMaxConsecutiveFaults);

        try {
            m->Shutdown();
        } catch (...) {}

        try {
            if (kernel_ && m->Initialize(*kernel_)) {
                fr.ClearFault();
                KL_LOG("Kernel", "Module '%s' restarted successfully",
                        info.name);

                struct { const char* name; } payload{info.name};
                kernel_->Messages().Send(
                    MakeMessage(MSG_MODULE_RESTARTED, MODULE_KERNEL, payload));
            } else {
                fr.RecordFault("restart init failed");
                KL_ERR("Kernel", "Module '%s' restart failed", info.name);
            }
        } catch (const std::exception& e) {
            fr.RecordFault(e.what());
            KL_ERR("Kernel", "Module '%s' restart threw: %s",
                   info.name, e.what());
        } catch (...) {
            fr.RecordFault("unknown exception during restart");
        }
    }
    pendingRestarts_.clear();
}

bool ModuleLoader::RestartFaultedModule(const std::string& name) {
    for (auto* m : initialized_) {
        if (m->GetInfo().name != name) continue;

        auto& fr = faults_[m];
        if (fr.consecutiveFaults == 0) return false;  // Not faulted

        // Force immediate restart (bypass cooldown)
        fr.faultCooldown = 0.0f;

        try { m->Shutdown(); } catch (...) {}

        try {
            if (kernel_ && m->Initialize(*kernel_)) {
                fr.ClearFault();
                KL_LOG("Kernel", "Module '%s' manually restarted", name.c_str());
                return true;
            }
        } catch (...) {}

        fr.RecordFault("manual restart failed");
        return false;
    }
    return false;
}

} // namespace koilo
