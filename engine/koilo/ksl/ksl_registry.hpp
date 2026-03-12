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

        auto endsWith = [](const std::string& s, const std::string& suffix) {
            return s.size() >= suffix.size() &&
                   s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
        };

        // Try loading uber-shader first
        bool hasUber = false;
#ifdef KL_HAVE_OPENGL_BACKEND
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
            if (hasUber) {
                auto it = uberShaderIDs_.find(name);
                if (it != uberShaderIDs_.end()) {
                    mod->SetUberProgram(uberProgram_, it->second);
                    hasGPU = true;
                }
            } else if (fs::exists(glslPath) && !vertexShaderSrc.empty()) {
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

    bool HasUberShader() const {
#ifdef KL_HAVE_OPENGL_BACKEND
        return uberProgram_ != 0;
#else
        return false;
#endif
    }

#ifdef KL_HAVE_OPENGL_BACKEND
    unsigned int GetUberProgram() const { return uberProgram_; }
#endif

    void Clear() {
#ifdef KL_HAVE_OPENGL_BACKEND
        if (uberProgram_) {
            glDeleteProgram(uberProgram_);
            uberProgram_ = 0;
        }
        uberShaderIDs_.clear();
#endif
        modules_.clear();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<KSLModule>> modules_;
#ifdef KL_HAVE_OPENGL_BACKEND
    unsigned int uberProgram_ = 0;
    std::unordered_map<std::string, int> uberShaderIDs_;
#endif
};

} // namespace ksl

