// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ksl_module.hpp"
#include "ksl_symbols.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <fstream>
#include <cstdio>

#ifdef KL_HAVE_FILESYSTEM
#include <filesystem>
#endif

namespace ksl {

/// Registry: scans a directory for .kso + .glsl shader pairs, loads them.
///
/// GLSL source is retained for backend-agnostic RHI shader creation.
/// No OpenGL (or any other GPU API) headers or calls are present.
class KSLRegistry {
public:
    KSLRegistry() = default;

    /// Scan a directory for shader modules.
    /// Looks for matching pairs: name.kso + name.glsl
    /// Also loads standalone .glsl (GPU-only) or .kso (CPU-only).
    int ScanDirectory(const std::string& dir,
                      const std::string& vertexShaderSrc = "",
                      const KSLSymbolTable* symbols = nullptr) {
#ifdef KL_HAVE_FILESYSTEM
        namespace fs = std::filesystem;
        int loaded = 0;

        if (!fs::exists(dir) || !fs::is_directory(dir)) return 0;

        // Retain vertex shader source for RHI shader creation
        if (!vertexShaderSrc.empty()) {
            vertexShaderSource_ = vertexShaderSrc;
        }

        auto endsWith = [](const std::string& s, const std::string& suffix) {
            return s.size() >= suffix.size() &&
                   s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
        };

        // Collect all shader names (skip uber files -- no longer used)
        std::vector<std::string> names;
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            std::string fname = entry.path().filename().string();
            std::string stem = entry.path().stem().string();

            if (stem == "uber" || stem == "uber_ids") continue;

            if (endsWith(fname, ".glsl") || endsWith(fname, ".kso")) {
                if (std::find(names.begin(), names.end(), stem) == names.end()) {
                    names.push_back(stem);
                }
            }
        }

        for (const auto& name : names) {
            auto mod = std::make_unique<KSLModule>();

            std::string ksoPath = dir + "/" + name + ".kso";
            std::string glslPath = dir + "/" + name + ".glsl";

            bool hasCPU = false, hasGPU = false;

            if (symbols && fs::exists(ksoPath)) {
                hasCPU = mod->LoadKSO(ksoPath, *symbols);
            }

            if (fs::exists(glslPath) && !vertexShaderSrc.empty()) {
                hasGPU = mod->LoadGLSL(glslPath, vertexShaderSrc);
            }

            if (hasCPU || hasGPU) {
                modules_[name] = std::move(mod);
                loaded++;
            }
        }

        return loaded;
#else
        (void)dir; (void)vertexShaderSrc; (void)symbols;
        return 0;
#endif
    }

    // Load a single GLSL file by name (for built-in shaders generated at build time)
    bool LoadGLSL(const std::string& name, const std::string& glslPath,
                  const std::string& vertexShaderSrc) {
        auto mod = std::make_unique<KSLModule>();
        if (!mod->LoadGLSL(glslPath, vertexShaderSrc)) return false;
        modules_[name] = std::move(mod);
        return true;
    }

    /**
     * @brief Scan a SPIR-V directory for compiled .frag.spv + .vert.spv files.
     *
     * Populates the spirvData_ map with raw SPIR-V bytecode for each shader.
     * The Vulkan render backend retrieves this data to create pipeline objects.
     *
     * @param spirvDir Path to directory containing .frag.spv files and scene.vert.spv.
     * @return Number of fragment SPIR-V modules loaded.
     */
    int ScanSPIRVDirectory(const std::string& spirvDir) {
#ifdef KL_HAVE_FILESYSTEM
        namespace fs = std::filesystem;
        int loaded = 0;

        if (!fs::exists(spirvDir) || !fs::is_directory(spirvDir)) return 0;

        spirvDir_ = spirvDir;

        // Load shared vertex shader
        std::string vertPath = spirvDir + "/scene.vert.spv";
        if (fs::exists(vertPath)) {
            vertexSPIRV_ = ReadBinaryFile(vertPath);
        }

        // Load per-shader fragment SPIR-V
        for (const auto& entry : fs::directory_iterator(spirvDir)) {
            if (!entry.is_regular_file()) continue;
            std::string fname = entry.path().filename().string();
            // Match *.frag.spv
            if (fname.size() > 9 &&
                fname.compare(fname.size() - 9, 9, ".frag.spv") == 0) {
                std::string stem = fname.substr(0, fname.size() - 9);
                auto data = ReadBinaryFile(entry.path().string());
                if (!data.empty()) {
                    spirvData_[stem] = std::move(data);
                    loaded++;
                }
            }
        }

        return loaded;
#else
        (void)spirvDir;
        return 0;
#endif
    }

    /** Get raw SPIR-V bytecode for a fragment shader by name. */
    const std::vector<uint32_t>& GetFragmentSPIRV(const std::string& name) const {
        static const std::vector<uint32_t> empty;
        auto it = spirvData_.find(name);
        return it != spirvData_.end() ? it->second : empty;
    }

    /** Get raw SPIR-V bytecode for the shared vertex shader. */
    const std::vector<uint32_t>& GetVertexSPIRV() const { return vertexSPIRV_; }

    /** Check if SPIR-V data is loaded. */
    bool HasSPIRV() const { return !spirvData_.empty() && !vertexSPIRV_.empty(); }

    /** Get retained GLSL vertex shader source (for RHI shader creation). */
    const std::string& GetVertexShaderSource() const { return vertexShaderSource_; }

    /** Check if GLSL source is available for RHI shader creation. */
    bool HasGLSLSources() const { return !vertexShaderSource_.empty(); }

    /** Get list of loaded SPIR-V shader names. */
    std::vector<std::string> ListSPIRVShaders() const {
        std::vector<std::string> names;
        names.reserve(spirvData_.size());
        for (const auto& [name, _] : spirvData_) {
            names.push_back(name);
        }
        return names;
    }

    KSLModule* GetModule(const std::string& name) const {
        auto it = modules_.find(name);
        return it != modules_.end() ? it->second.get() : nullptr;
    }

    bool HasModule(const std::string& name) const {
        return modules_.find(name) != modules_.end();
    }

    std::vector<std::string> ListShaders() const {
        std::vector<std::string> names;
        names.reserve(modules_.size());
        for (const auto& [name, _] : modules_) {
            names.push_back(name);
        }
        return names;
    }

    size_t Count() const { return modules_.size(); }

    /// Reload a single shader's GLSL source from disk.
    /// Returns true if the shader was found and reloaded successfully.
    bool ReloadShader(const std::string& name) {
        auto it = modules_.find(name);
        if (it == modules_.end()) return false;
        return it->second->ReloadGLSL();
    }

    /// Reload a single SPIR-V fragment shader from disk.
    bool ReloadSPIRV(const std::string& name) {
#ifdef KL_HAVE_FILESYSTEM
        auto it = spirvData_.find(name);
        if (it == spirvData_.end() || spirvDir_.empty()) return false;
        std::string path = spirvDir_ + "/" + name + ".frag.spv";
        auto data = ReadBinaryFile(path);
        if (data.empty()) return false;
        it->second = std::move(data);
        return true;
#else
        (void)name;
        return false;
#endif
    }

    /// Reload all shaders (GLSL + SPIR-V) from their original files.
    /// Returns the number of shaders successfully reloaded.
    int ReloadAll() {
        int count = 0;
        for (auto& [name, mod] : modules_) {
            if (mod->ReloadGLSL()) ++count;
        }
        for (auto& [name, _] : spirvData_) {
            if (ReloadSPIRV(name)) ++count;
        }
        return count;
    }

    /// Get all watched file paths (for FileWatcher setup).
    std::vector<std::string> GetShaderFilePaths() const {
        std::vector<std::string> paths;
        for (auto& [name, mod] : modules_) {
            const auto& p = mod->GetGLSLFilePath();
            if (!p.empty()) paths.push_back(p);
        }
#ifdef KL_HAVE_FILESYSTEM
        if (!spirvDir_.empty()) {
            for (auto& [name, _] : spirvData_) {
                paths.push_back(spirvDir_ + "/" + name + ".frag.spv");
            }
        }
#endif
        return paths;
    }

    /// Find shader name by file path (for FileWatcher callback).
    std::string FindShaderByPath(const std::string& path) const {
        for (auto& [name, mod] : modules_) {
            if (mod->GetGLSLFilePath() == path) return name;
        }
#ifdef KL_HAVE_FILESYSTEM
        // Check SPIR-V paths
        if (!spirvDir_.empty()) {
            for (auto& [name, _] : spirvData_) {
                if (path == spirvDir_ + "/" + name + ".frag.spv") return name;
            }
        }
#endif
        return {};
    }

    void Clear() {
        modules_.clear();
        vertexSPIRV_.clear();
        spirvData_.clear();
        vertexShaderSource_.clear();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<KSLModule>> modules_;

    // SPIR-V bytecode storage (loaded but not consumed until Vulkan backend creates pipelines)
    std::vector<uint32_t> vertexSPIRV_;
    std::unordered_map<std::string, std::vector<uint32_t>> spirvData_;

    // GLSL source retention for RHI shader creation
    std::string vertexShaderSource_;

    // Original scan directories for hot-reload
    std::string spirvDir_;

    /** Read a binary file as uint32_t words (SPIR-V format). */
    static std::vector<uint32_t> ReadBinaryFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return {};
        auto size = file.tellg();
        if (size <= 0 || size % 4 != 0) return {};
        file.seekg(0);
        std::vector<uint32_t> data(static_cast<size_t>(size) / 4);
        file.read(reinterpret_cast<char*>(data.data()), size);
        return data;
    }
};

} // namespace ksl

