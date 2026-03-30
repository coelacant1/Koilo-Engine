// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
/**
 * @file hot_reload.hpp
 * @brief Shader and script hot-reload via file watching.
 *
 * Monitors shader directories and script files for changes.
 * On shader change: reloads source, invalidates pipeline cache.
 * On script change: triggers engine reload.
 */

#include <koilo/core/platform/file_watcher.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <string>
#include <vector>
#include <functional>

namespace ksl { class KSLRegistry; }

namespace koilo {

namespace rhi { class RenderPipeline; }
namespace scripting { class KoiloScriptEngine; }

/// Manages hot-reload for shaders and scripts via FileWatcher polling.
class HotReloadService {
public:
    HotReloadService() = default;

    /// Set up shader watching from the pipeline's KSL registry.
    void WatchShaders(rhi::RenderPipeline* pipeline);

    /// Watch a script file for changes.
    void WatchScript(const std::string& path,
                     scripting::KoiloScriptEngine* engine);

    /// Poll for file changes. Call once per frame (or periodically).
    /// Returns number of files that changed.
    int Poll();

    /// Enable or disable hot-reload.
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

    /// Number of files being watched.
    size_t WatchCount() const { return shaderWatcher_.Count() + scriptWatcher_.Count(); }

    /// Number of shader reloads performed since creation.
    int ShaderReloadCount() const { return shaderReloads_; }

    /// Number of script reloads performed since creation.
    int ScriptReloadCount() const { return scriptReloads_; }

private:
    FileWatcher shaderWatcher_;
    FileWatcher scriptWatcher_;

    rhi::RenderPipeline*            pipeline_ = nullptr;
    ::ksl::KSLRegistry*            registry_ = nullptr;
    scripting::KoiloScriptEngine*  engine_   = nullptr;

    bool enabled_ = true;
    float pollTimer_ = 0.0f;
    int shaderReloads_ = 0;
    int scriptReloads_ = 0;
};

} // namespace koilo
