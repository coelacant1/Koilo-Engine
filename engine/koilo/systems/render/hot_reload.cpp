// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/render/hot_reload.hpp>
#include <koilo/ksl/ksl_shader.hpp>
#include <koilo/systems/render/pipeline/render_pipeline.hpp>
#include <koilo/ksl/ksl_registry.hpp>
#include <koilo/scripting/koiloscript_engine.hpp>
#include <chrono>

namespace koilo {

void HotReloadService::WatchShaders(rhi::RenderPipeline* pipeline) {
    if (!pipeline) return;
    pipeline_ = pipeline;
    registry_ = pipeline->GetRegistry();
    if (!registry_) return;

    auto paths = registry_->GetShaderFilePaths();
    for (auto& p : paths) {
        shaderWatcher_.Watch(p);
    }

    shaderWatcher_.SetCallback([this](const std::string& path) {
        if (!registry_ || !pipeline_) return;

        std::string shaderName = registry_->FindShaderByPath(path);
        if (shaderName.empty()) return;

        bool ok = registry_->ReloadShader(shaderName);
        if (!ok) ok = registry_->ReloadSPIRV(shaderName);

        if (ok) {
            pipeline_->InvalidatePipelineCache();
            ++shaderReloads_;
            KL_LOG("HotReload", "Reloaded shader: %s", shaderName.c_str());
        } else {
            KL_WARN("HotReload", "Failed to reload shader: %s",
                    shaderName.c_str());
        }
    });

    KL_LOG("HotReload", "Watching %zu shader file(s)", paths.size());
}

void HotReloadService::WatchScript(const std::string& path,
                                   scripting::KoiloScriptEngine* engine) {
    if (path.empty() || !engine) return;
    engine_ = engine;

    scriptWatcher_.Watch(path);
    scriptWatcher_.SetCallback([this](const std::string& changedPath) {
        if (!engine_) return;

        if (engine_->Reload()) {
            ++scriptReloads_;
            KL_LOG("HotReload", "Reloaded script: %s", changedPath.c_str());
        } else {
            KL_WARN("HotReload", "Script reload failed: %s",
                    engine_->GetError());
        }
    });

    KL_LOG("HotReload", "Watching script: %s", path.c_str());
}

int HotReloadService::Poll() {
    if (!enabled_) return 0;
    int changed = 0;
    changed += shaderWatcher_.Poll();
    changed += scriptWatcher_.Poll();
    return changed;
}

} // namespace koilo
