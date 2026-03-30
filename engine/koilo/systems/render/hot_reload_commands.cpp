// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/ksl/ksl_shader.hpp>
#include <koilo/systems/render/hot_reload.hpp>
#include <koilo/systems/render/pipeline/render_pipeline.hpp>
#include <koilo/ksl/ksl_registry.hpp>
#include <koilo/scripting/koiloscript_engine.hpp>
#include <sstream>
#include <iomanip>

namespace koilo {

void RegisterHotReloadCommands(CommandRegistry& registry) {
    // -- shader.list: list loaded shaders --
    registry.Register({
        "shader.list",
        "shader.list",
        "List loaded KSL shaders",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto* pipeline = kernel.Services().Get<rhi::RenderPipeline>("render_backend");
            if (!pipeline) return ConsoleResult::Error("No render pipeline active");
            auto* reg = pipeline->GetRegistry();
            if (!reg) return ConsoleResult::Error("No shader registry");

            auto names = reg->ListShaders();
            std::ostringstream os;
            os << names.size() << " shader(s) loaded:\n";
            for (auto& n : names) {
                auto* mod = reg->GetModule(n);
                os << "  " << std::left << std::setw(24) << n;
                if (mod) {
                    if (mod->HasGLSLSource()) os << " [GLSL]";
                    if (mod->HasCPU())        os << " [CPU]";
                }
                os << "\n";
            }
            if (reg->HasSPIRV()) {
                auto spvNames = reg->ListSPIRVShaders();
                os << spvNames.size() << " SPIR-V shader(s):\n";
                for (auto& n : spvNames) {
                    os << "  " << n << " [SPIR-V]\n";
                }
            }
            return ConsoleResult::Ok(os.str());
        },
        nullptr
    });

    // -- shader.reload [name]: reload one or all shaders --
    registry.Register({
        "shader.reload",
        "shader.reload [name]",
        "Reload shader source from disk and invalidate pipeline cache",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            auto* pipeline = kernel.Services().Get<rhi::RenderPipeline>("render_backend");
            if (!pipeline) return ConsoleResult::Error("No render pipeline active");
            auto* reg = pipeline->GetRegistry();
            if (!reg) return ConsoleResult::Error("No shader registry");

            if (args.empty()) {
                int count = reg->ReloadAll();
                pipeline->InvalidatePipelineCache();
                std::ostringstream os;
                os << "Reloaded " << count << " shader(s), pipeline cache invalidated";
                return ConsoleResult::Ok(os.str());
            }

            const auto& name = args[0];
            bool ok = reg->ReloadShader(name);
            if (!ok) ok = reg->ReloadSPIRV(name);
            if (!ok) return ConsoleResult::Error("Shader not found or reload failed: " + name);

            pipeline->InvalidatePipelineCache();
            return ConsoleResult::Ok("Reloaded: " + name);
        },
        // Tab-complete shader names
        [](KoiloKernel& kernel, const std::vector<std::string>&,
           const std::string& partial) -> std::vector<std::string> {
            auto* pipeline = kernel.Services().Get<rhi::RenderPipeline>("render_backend");
            if (!pipeline) return {};
            auto* reg = pipeline->GetRegistry();
            if (!reg) return {};
            std::vector<std::string> matches;
            for (auto& n : reg->ListShaders()) {
                if (n.compare(0, partial.size(), partial) == 0)
                    matches.push_back(n);
            }
            return matches;
        }
    });

    // -- exec.reload: reload the current script --
    registry.Register({
        "exec.reload",
        "exec.reload",
        "Reload the current script from disk",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto* engine = kernel.Services().Get<scripting::KoiloScriptEngine>("script");
            if (!engine)
                return ConsoleResult::Error("No script engine registered");

            if (engine->Reload()) {
                return ConsoleResult::Ok("Script reloaded successfully");
            }
            return ConsoleResult::Error(
                std::string("Script reload failed: ") + engine->GetError());
        },
        nullptr
    });

    // -- hotreload.status: show hot-reload watcher state --
    registry.Register({
        "hotreload.status",
        "hotreload.status",
        "Show hot-reload watcher status",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto* hr = kernel.Services().Get<HotReloadService>("hot_reload");
            if (!hr) return ConsoleResult::Error("Hot-reload service not active");

            std::ostringstream os;
            os << "Hot-reload: " << (hr->IsEnabled() ? "enabled" : "disabled") << "\n"
               << "Watched files: " << hr->WatchCount() << "\n"
               << "Shader reloads: " << hr->ShaderReloadCount() << "\n"
               << "Script reloads: " << hr->ScriptReloadCount() << "\n";
            return ConsoleResult::Ok(os.str());
        },
        nullptr
    });

    // -- hotreload.toggle: enable/disable hot-reload --
    registry.Register({
        "hotreload.toggle",
        "hotreload.toggle",
        "Toggle hot-reload on/off",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto* hr = kernel.Services().Get<HotReloadService>("hot_reload");
            if (!hr) return ConsoleResult::Error("Hot-reload service not active");

            bool newState = !hr->IsEnabled();
            hr->SetEnabled(newState);
            return ConsoleResult::Ok(
                std::string("Hot-reload ") + (newState ? "enabled" : "disabled"));
        },
        nullptr
    });
}

} // namespace koilo
