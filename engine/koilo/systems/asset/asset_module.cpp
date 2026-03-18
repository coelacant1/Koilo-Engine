// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file asset_module.cpp
 * @brief AssetModule implementation - lifecycle, loading, hot-reload, console.
 */
#include "asset_module.hpp"
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/console/console_module.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <koilo/scripting/koiloscript_engine.hpp>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace koilo {

AssetModule* AssetModule::instance_ = nullptr;

AssetModule::AssetModule() = default;
AssetModule::~AssetModule() = default;

// -- IModule interface ---------------------------------------------

ModuleInfo AssetModule::GetInfo() const {
    return {"asset", "0.1.0", ModulePhase::Core};
}

bool AssetModule::Initialize(KoiloKernel& kernel) {
    kernel_ = &kernel;
    instance_ = this;
    jobQueue_.SetPool(&kernel.Pool());
    SetupFileWatcher();
    RegisterConsoleCommands();

    kernel.Services().Register("assets", this);

    // Register with script engine if available.
    auto* engine = kernel.Services().Get<scripting::KoiloScriptEngine>("script");
    if (engine) {
        (void)AssetModule::Describe();
        engine->RegisterGlobal("assets", "AssetModule", this);
    }

    return true;
}

void AssetModule::Update(float dt) {
    // Process completed async loads.
    jobQueue_.ProcessCompleted(4);

    // Periodic hot-reload polling.
    hotReloadTimer_ += dt;
    if (hotReloadTimer_ >= kHotReloadInterval) {
        hotReloadTimer_ = 0.0f;
        watcher_.Poll();
    }
}

void AssetModule::Render(Color888*, int, int) {
    // Asset pipeline has no render pass.
}

void AssetModule::Shutdown() {
    watcher_.Clear();

    // Unregister from kernel services.
    if (kernel_) {
        kernel_->Services().Unregister("assets");
    }

    kernel_ = nullptr;

    // Clear singleton if this is the active instance.
    if (instance_ == this) {
        instance_ = nullptr;
    }
}

// -- Kernel module descriptor --------------------------------------

const ModuleDesc& AssetModule::GetModuleDesc() {
    static ModuleDesc desc{};
    desc.name = "koilo.asset";
    desc.version = KL_VERSION(0, 1, 0);
    desc.requiredCaps = Cap::FileRead;
    desc.Init = &AssetModule::Init;
    desc.Tick = &AssetModule::Tick;
    desc.OnMessage = &AssetModule::HandleMessage;
    desc.Shutdown = &AssetModule::KernelShutdown;
    return desc;
}

bool AssetModule::Init(KoiloKernel& kernel) {
    // Static kernel-path: creates a module and initializes via IModule::Initialize.
    // When used through ModuleLoader, Initialize() is called directly and this
    // static path is not used.
    auto* mod = new AssetModule();
    instance_ = mod;
    return mod->Initialize(kernel);
}

void AssetModule::Tick(float dt) {
    if (instance_) {
        instance_->Update(dt);
    }
}

void AssetModule::HandleMessage(const Message& /*msg*/) {
    // Future: handle asset-related messages.
}

void AssetModule::KernelShutdown() {
    if (instance_) {
        instance_->Shutdown();
        instance_ = nullptr;
    }
}

// -- File watcher setup --------------------------------------------

void AssetModule::SetupFileWatcher() {
    watcher_.SetCallback([this](const std::string& path) {
        AssetHandle handle = registry_.FindByPath(path);
        if (handle.IsNull()) return;

        KL_LOG("AssetModule", "File changed: %s - marking for reload", path.c_str());
        registry_.SetState(handle, AssetState::Registered);

        // Cascade invalidation to dependents.
        auto dependents = registry_.GetDependents(handle);
        for (AssetHandle dep : dependents) {
            registry_.SetState(dep, AssetState::Registered);
        }

        // Notify bus
        if (kernel_) {
            kernel_->Messages().Send(MakeSignal(MSG_ASSET_CHANGED));
        }
    });
}

// -- Public API ----------------------------------------------------

AssetHandle AssetModule::Load(const std::string& path) {
    // Check if already registered and loaded.
    AssetHandle existing = registry_.FindByPath(path);
    if (!existing.IsNull()) {
        AssetState state = registry_.GetState(existing);
        if (state == AssetState::Loaded) return existing;
    }

    // Infer type from extension.
    std::string ext;
    auto dot = path.rfind('.');
    if (dot != std::string::npos) {
        ext = path.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }
    AssetType type = AssetManifest::InferType(ext);

    // Register if new.
    AssetHandle handle = existing.IsNull()
        ? registry_.Register(path, type)
        : existing;

    registry_.SetState(handle, AssetState::Loading);

    // Check if a converter is needed.
    IAssetConverter* converter = converters_.FindByExtension(ext);
    if (converter) {
        // Convert to engine format first.
        std::string targetExt = converter->GetTargetExtension();
        std::string targetPath = path.substr(0, dot) + targetExt;
        if (!converter->Convert(path, targetPath)) {
            KL_ERR("AssetModule", "Conversion failed for %s: %s",
                   path.c_str(), converter->GetLastError().c_str());
            registry_.SetState(handle, AssetState::Failed);
            return handle;
        }
        // Update path to the converted file.
        // (registry path stays as original for dedup)
    }

    registry_.SetState(handle, AssetState::Loaded);

    // Notify bus
    if (kernel_) {
        kernel_->Messages().Send(MakeSignal(MSG_ASSET_LOADED));
    }

    // Watch for hot-reload.
    watcher_.Watch(path);

    return handle;
}

void AssetModule::LoadAsync(const std::string& path, AssetJobQueue::LoadCallback callback) {
    // Infer type from extension.
    std::string ext;
    auto dot = path.rfind('.');
    if (dot != std::string::npos) {
        ext = path.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }
    AssetType type = AssetManifest::InferType(ext);

    AssetHandle handle = registry_.FindByPath(path);
    if (handle.IsNull()) {
        handle = registry_.Register(path, type);
    }

    registry_.SetState(handle, AssetState::Loading);

    // The load function runs on the worker thread.
    auto loadFn = [this, path](AssetHandle h) -> AssetLoadResult {
        AssetLoadResult result;
        result.handle = h;
        // For now, just mark as loaded - actual file reading will be
        // wired in p5-integrate-existing when we connect existing loaders.
        result.success = true;
        return result;
    };

    auto wrappedCallback = [this, path, callback](const AssetLoadResult& result) {
        if (result.success) {
            registry_.SetState(result.handle, AssetState::Loaded);
            if (result.data) {
                registry_.SetData<void>(result.handle,
                    std::shared_ptr<void>(result.data), result.memoryBytes);
            }
            watcher_.Watch(path);
        } else {
            registry_.SetState(result.handle, AssetState::Failed);
            KL_ERR("AssetModule", "Async load failed for %s: %s",
                   path.c_str(), result.error.c_str());
        }
        if (callback) callback(result);
    };

    jobQueue_.RequestLoad(handle, std::move(loadFn), std::move(wrappedCallback));
}

void AssetModule::Unload(AssetHandle handle) {
    if (!registry_.IsValid(handle)) return;

    const std::string& path = registry_.GetPath(handle);
    watcher_.Unwatch(path);

    registry_.SetState(handle, AssetState::Unloading);
    registry_.Unregister(handle);

    if (kernel_) {
        kernel_->Messages().Send(MakeSignal(MSG_ASSET_UNLOADED));
    }
}

int AssetModule::ReloadChanged() {
    return watcher_.Poll();
}

size_t AssetModule::GarbageCollect() {
    auto candidates = registry_.CollectGarbage();
    size_t freedBytes = 0;
    for (AssetHandle handle : candidates) {
        freedBytes += registry_.GetMemoryBytes(handle);
        Unload(handle);
    }
    return freedBytes;
}

// -- Script-friendly API -------------------------------------------

uint32_t AssetModule::ScriptLoad(const std::string& path) {
    return Load(path).value;
}

void AssetModule::ScriptUnload(uint32_t handleValue) {
    AssetHandle h;
    h.value = handleValue;
    Unload(h);
}

bool AssetModule::ScriptIsValid(uint32_t handleValue) const {
    AssetHandle h;
    h.value = handleValue;
    return registry_.IsValid(h);
}

std::string AssetModule::ScriptGetType(uint32_t handleValue) const {
    AssetHandle h;
    h.value = handleValue;
    if (!registry_.IsValid(h)) return "Invalid";
    return AssetTypeName(h.Type());
}

std::string AssetModule::ScriptGetState(uint32_t handleValue) const {
    AssetHandle h;
    h.value = handleValue;
    if (!registry_.IsValid(h)) return "Invalid";
    return AssetStateName(registry_.GetState(h));
}

std::string AssetModule::ScriptGetPath(uint32_t handleValue) const {
    AssetHandle h;
    h.value = handleValue;
    return registry_.GetPath(h);
}

size_t AssetModule::GetAssetCount() const {
    return registry_.Count();
}

size_t AssetModule::GetTotalMemory() const {
    return registry_.TotalMemory();
}

// -- Console commands ----------------------------------------------

void AssetModule::RegisterConsoleCommands() {
    auto* console = ConsoleModule::Instance();
    if (!console) return;

    auto& cmds = console->Commands();

    cmds.Register({"assets", "assets [list|info|memory|gc|reload] [args...]",
        "Asset pipeline management",
        [this](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty() || args[0] == "list") {
                // Optional type filter: assets list Mesh
                AssetType filterType = AssetType::Generic;
                bool hasFilter = false;
                if (args.size() >= 2) {
                    std::string typeName = args[1];
                    std::transform(typeName.begin(), typeName.end(), typeName.begin(), ::tolower);
                    if (typeName == "mesh")     { filterType = AssetType::Mesh; hasFilter = true; }
                    else if (typeName == "texture") { filterType = AssetType::Texture; hasFilter = true; }
                    else if (typeName == "audio")   { filterType = AssetType::Audio; hasFilter = true; }
                    else if (typeName == "script")  { filterType = AssetType::Script; hasFilter = true; }
                    else if (typeName == "font")    { filterType = AssetType::Font; hasFilter = true; }
                    else if (typeName == "markup")  { filterType = AssetType::Markup; hasFilter = true; }
                    else if (typeName == "material") { filterType = AssetType::Material; hasFilter = true; }
                }

                std::ostringstream ss;
                size_t count = 0;
                registry_.ForEach([&](AssetHandle handle, const AssetSlot& slot) {
                    if (hasFilter && slot.type != filterType) return;
                    ss << "  [" << AssetTypeName(slot.type) << "] "
                       << slot.path << " - " << AssetStateName(slot.state);
                    if (slot.memoryBytes > 0) {
                        ss << " (" << slot.memoryBytes << " bytes)";
                    }
                    ss << "\n";
                    ++count;
                });

                if (count == 0) {
                    return ConsoleResult::Ok("No assets registered.");
                }
                return ConsoleResult::Ok(std::to_string(count) + " asset(s):\n" + ss.str());
            }

            if (args[0] == "info") {
                if (args.size() < 2) return ConsoleResult::Error("Usage: assets info <path>");
                AssetHandle h = registry_.FindByPath(args[1]);
                if (h.IsNull()) return ConsoleResult::Error("Not found: " + args[1]);

                const AssetSlot* slot = nullptr;
                std::ostringstream ss;
                registry_.ForEach([&](AssetHandle handle, const AssetSlot& s) {
                    if (handle == h) {
                        ss << "Path: " << s.path << "\n"
                           << "Type: " << AssetTypeName(s.type) << "\n"
                           << "State: " << AssetStateName(s.state) << "\n"
                           << "Memory: " << s.memoryBytes << " bytes\n"
                           << "Dependencies: " << s.dependencies.size() << "\n";
                        for (AssetHandle dep : s.dependencies) {
                            ss << "  -> " << registry_.GetPath(dep) << "\n";
                        }
                        auto dependents = registry_.GetDependents(h);
                        ss << "Dependents: " << dependents.size() << "\n";
                        for (AssetHandle dep : dependents) {
                            ss << "  ← " << registry_.GetPath(dep) << "\n";
                        }
                    }
                });
                return ConsoleResult::Ok(ss.str());
            }

            if (args[0] == "memory") {
                size_t total = registry_.TotalMemory();
                size_t count = registry_.Count();
                size_t loaded = registry_.CountByState(AssetState::Loaded);
                std::ostringstream ss;
                ss << "Assets: " << count << " registered, " << loaded << " loaded\n"
                   << "Memory: " << total << " bytes ("
                   << std::fixed << std::setprecision(2)
                   << (total / 1024.0 / 1024.0) << " MB)";
                return ConsoleResult::Ok(ss.str());
            }

            if (args[0] == "gc") {
                size_t freed = GarbageCollect();
                return ConsoleResult::Ok("Freed " + std::to_string(freed) + " bytes.");
            }

            if (args[0] == "reload") {
                int changed = ReloadChanged();
                return ConsoleResult::Ok("Reloaded " + std::to_string(changed) + " asset(s).");
            }

            if (args[0] == "manifest") {
                if (args.size() >= 2 && args[1] == "scan") {
                    std::string dir = (args.size() >= 3) ? args[2] : ".";
                    manifest_.Scan(dir);
                    return ConsoleResult::Ok("Scanned " + std::to_string(manifest_.Count()) + " files.");
                }
                if (args.size() >= 2 && args[1] == "save") {
                    std::string path = (args.size() >= 3) ? args[2] : "assets.kasset";
                    if (manifest_.Save(path)) {
                        return ConsoleResult::Ok("Manifest saved to " + path);
                    }
                    return ConsoleResult::Error("Failed to save manifest.");
                }
                if (args.size() >= 2 && args[1] == "load") {
                    std::string path = (args.size() >= 3) ? args[2] : "assets.kasset";
                    if (manifest_.Load(path)) {
                        return ConsoleResult::Ok("Loaded " + std::to_string(manifest_.Count()) + " entries.");
                    }
                    return ConsoleResult::Error("Failed to load manifest.");
                }
                return ConsoleResult::Error("Usage: assets manifest [scan|save|load] [path]");
            }

            return ConsoleResult::Error("Unknown subcommand: " + args[0] +
                "\nUsage: assets [list|info|memory|gc|reload|manifest]");
        }, nullptr
    });
}

} // namespace koilo
