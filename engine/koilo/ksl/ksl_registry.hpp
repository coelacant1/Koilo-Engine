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

// Registry: scans a directory for .kso + .glsl shader pairs, loads them
class KSLRegistry {
public:
    KSLRegistry() = default;

    // Scan a directory for shader modules
    // Looks for matching pairs: name.kso + name.glsl
    // Also loads standalone .glsl (GPU-only) or .kso (CPU-only)
    // If uber.glsl + uber_ids.txt exist, loads uber-shader for shared GPU program
    int ScanDirectory(const std::string& dir,
                      const std::string& vertexShaderSrc = "",
                      const KSLSymbolTable* symbols = nullptr) {
#ifdef KL_HAVE_FILESYSTEM
        namespace fs = std::filesystem;
        int loaded = 0;

        if (!fs::exists(dir) || !fs::is_directory(dir)) return 0;

        // Retain vertex shader source for RHI shader creation (Phase 17f)
        if (!vertexShaderSrc.empty()) {
            vertexShaderSource_ = vertexShaderSrc;
        }

        auto endsWith = [](const std::string& s, const std::string& suffix) {
            return s.size() >= suffix.size() &&
                   s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
        };

        // Try loading uber-shader first (GL only)
#ifdef KL_HAVE_OPENGL_BACKEND
        bool hasUber = false;
        std::string uberGlsl = dir + "/uber.glsl";
        std::string uberIds  = dir + "/uber_ids.txt";
        if (!vertexShaderSrc.empty() && fs::exists(uberGlsl) && fs::exists(uberIds)) {
            KSLModule uberMod;
            if (uberMod.LoadGLSL(uberGlsl, vertexShaderSrc)) {
                uberProgram_ = uberMod.GetGLProgram();
                uberMod.DetachGLProgram();  // prevent cleanup

                // Parse shader ID mapping
                std::ifstream idFile(uberIds);
                std::string line;
                while (std::getline(idFile, line)) {
                    int id; char name[64];
                    if (sscanf(line.c_str(), "%d %63s", &id, name) == 2) {
                        uberShaderIDs_[std::string(name)] = id;
                    }
                }
                hasUber = true;
            }
        }
#endif

        // Collect all shader names (exclude uber files)
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

            // If uber-shader loaded, assign shared program + shader ID
#ifdef KL_HAVE_OPENGL_BACKEND
            if (hasUber) {
                auto it = uberShaderIDs_.find(name);
                if (it != uberShaderIDs_.end()) {
                    mod->SetUberProgram(uberProgram_, it->second);
                    hasGPU = true;
                }
            } else if (fs::exists(glslPath) && !vertexShaderSrc.empty()) {
                hasGPU = mod->LoadGLSL(glslPath, vertexShaderSrc);
            }
#else
            // No GL context - load GLSL for source retention (RHI shader creation)
            if (fs::exists(glslPath) && !vertexShaderSrc.empty()) {
                hasGPU = mod->LoadGLSL(glslPath, vertexShaderSrc);
            }
#endif

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

    bool HasUberShader() const { return uberProgram_ != 0; }

#ifdef KL_HAVE_OPENGL_BACKEND
    unsigned int GetUberProgram() const { return uberProgram_; }
#endif

    void Clear() {
#ifdef KL_HAVE_OPENGL_BACKEND
        if (uberProgram_) {
            glDeleteProgram(uberProgram_);
        }
#endif
        uberProgram_ = 0;
        uberShaderIDs_.clear();
        modules_.clear();
        vertexSPIRV_.clear();
        spirvData_.clear();
        vertexShaderSource_.clear();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<KSLModule>> modules_;
    // Always present so every TU sees the same layout (ODR compliance).
    unsigned int uberProgram_ = 0;
    std::unordered_map<std::string, int> uberShaderIDs_;

    // SPIR-V bytecode storage (loaded but not consumed until Vulkan backend creates pipelines)
    std::vector<uint32_t> vertexSPIRV_;
    std::unordered_map<std::string, std::vector<uint32_t>> spirvData_;

    // GLSL source retention for RHI shader creation (Phase 17f)
    std::string vertexShaderSource_;

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

