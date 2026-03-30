// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file asset_module.hpp
 * @brief Asset pipeline module - kernel-aware IModule.
 *
 * The module provides:
 * - Unified asset handle system (generational indices)
 * - Extensible format converters
 * - Asset manifest (catalog of known assets)
 * - Dependency graph for cascading hot-reload
 * - Async loading via background worker thread
 * - Console commands for asset inspection
 *
 * @date 03/14/2026
 * @author Coela
 */
#pragma once

#include <koilo/kernel/unified_module.hpp>
#include <koilo/kernel/module.hpp>
#include <koilo/kernel/console/command_provider.hpp>
#include <koilo/kernel/asset/asset_registry.hpp>
#include <koilo/kernel/asset/asset_converter.hpp>
#include <koilo/kernel/asset/asset_manifest.hpp>
#include <koilo/kernel/asset/asset_job_queue.hpp>
#include <koilo/core/platform/file_watcher.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <memory>
#include <string>

namespace koilo {

class KoiloKernel;

/**
 * @class AssetModule
 * @brief Unified asset pipeline - kernel-level registry with script integration.
 */
class AssetModule : public IModule, public ICommandProvider {
public:
    AssetModule();
    ~AssetModule() override;

    // -- IModule interface -----------------------------------------

    ModuleInfo GetInfo() const override;
    bool Initialize(KoiloKernel& kernel) override;
    void Update(float dt) override;
    void Render(Color888* buffer, int width, int height) override;
    void Shutdown() override;

    // ICommandProvider
    std::vector<CommandDef> GetCommands() const override;

    // -- Kernel module descriptor (static lifecycle) ---------------

    static const ModuleDesc& GetModuleDesc();
    static bool Init(KoiloKernel& kernel);
    static void Tick(float dt);
    static void HandleMessage(const Message& msg);
    static void KernelShutdown();

    // -- Singleton access ------------------------------------------

    static AssetModule* Instance() { return instance_; }

    // -- Public API ------------------------------------------------

    AssetHandle Load(const std::string& path);
    void LoadAsync(const std::string& path, AssetJobQueue::LoadCallback callback);
    void Unload(AssetHandle handle);

    template<typename T>
    T* Get(AssetHandle handle) { return registry_.Get<T>(handle); }

    AssetHandle Find(const std::string& path) const { return registry_.FindByPath(path); }
    bool IsValid(AssetHandle handle) const { return registry_.IsValid(handle); }
    int ReloadChanged();
    size_t GarbageCollect();

    // -- Script-friendly API ---------------------------------------

    uint32_t ScriptLoad(const std::string& path);
    void ScriptUnload(uint32_t handleValue);
    bool ScriptIsValid(uint32_t handleValue) const;
    std::string ScriptGetType(uint32_t handleValue) const;
    std::string ScriptGetState(uint32_t handleValue) const;
    std::string ScriptGetPath(uint32_t handleValue) const;
    size_t GetAssetCount() const;
    size_t GetTotalMemory() const;

    // -- Sub-system access -----------------------------------------

    AssetRegistry&      Registry()   { return registry_; }
    ConverterRegistry&  Converters() { return converters_; }
    AssetManifest&      Manifest()   { return manifest_; }
    AssetJobQueue&      Jobs()       { return jobQueue_; }

private:
    static AssetModule* instance_;

    AssetRegistry       registry_;
    ConverterRegistry   converters_;
    AssetManifest       manifest_;
    AssetJobQueue       jobQueue_;
    FileWatcher         watcher_;

    float               hotReloadTimer_ = 0.0f;
    static constexpr float kHotReloadInterval = 1.0f;

    void SetupFileWatcher();

    KL_BEGIN_FIELDS(AssetModule)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(AssetModule)
        KL_METHOD_AUTO(AssetModule, ScriptLoad, "Load asset by path"),
        KL_METHOD_AUTO(AssetModule, ScriptUnload, "Unload asset by handle"),
        KL_METHOD_AUTO(AssetModule, ScriptIsValid, "Check if handle is valid"),
        KL_METHOD_AUTO(AssetModule, ScriptGetType, "Get asset type name"),
        KL_METHOD_AUTO(AssetModule, ScriptGetState, "Get asset state name"),
        KL_METHOD_AUTO(AssetModule, ScriptGetPath, "Get asset path"),
        KL_METHOD_AUTO(AssetModule, GetAssetCount, "Get registered asset count"),
        KL_METHOD_AUTO(AssetModule, GetTotalMemory, "Get total memory usage"),
        KL_METHOD_AUTO(AssetModule, ReloadChanged, "Reload changed assets"),
        KL_METHOD_AUTO(AssetModule, GarbageCollect, "Run garbage collection")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AssetModule)
        KL_CTOR0(AssetModule)
    KL_END_DESCRIBE(AssetModule)
};

} // namespace koilo
