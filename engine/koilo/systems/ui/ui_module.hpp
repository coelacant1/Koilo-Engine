// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_module.hpp
 * @brief UI system as an IModule with Overlay lifecycle phase.
 *
 * Owns the UI instance and manages font loading, animation updates,
 * and GPU/software rendering through the module lifecycle.
 *
 * @date 02/06/2026
 * @author Coela
 */
#pragma once

#include <koilo/kernel/unified_module.hpp>
#include <memory>

namespace koilo {

class UI;

class UIModule : public IModule {
public:
    UIModule();
    ~UIModule() override;

    ModuleInfo GetInfo() const override;
    bool Initialize(KoiloKernel& kernel) override;
    void Update(float dt) override;
    void Render(Color888* buffer, int width, int height) override;
    void Shutdown() override;

    UI* GetUI() { return ui_.get(); }

    /// Render UI overlay via GPU (OpenGL/Vulkan path).
    void RenderGPUOverlay(int viewportW, int viewportH, float dt);

    /// Update animations only (software path, called separately).
    void UpdateAnimations(float dt);

private:
    std::unique_ptr<UI> ui_;

    /// Ensure font is loaded (lazy init).
    void EnsureFont();
};

} // namespace koilo
