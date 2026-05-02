// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file render_module.hpp
 * @brief Render backend as an IModule with Render lifecycle phase.
 *
 * Owns the IRenderBackend instance and provides the GPU/software
 * render pipeline. The backend is set externally by the host after
 * the module is created (not during Initialize).
 *
 * @date 02/06/2026
 * @author Coela
 */
#pragma once

#include <koilo/kernel/unified_module.hpp>
#include <memory>

namespace koilo {

class IRenderBackend;

class RenderModule : public IModule {
public:
    RenderModule();
    ~RenderModule() override;

    ModuleInfo GetInfo() const override;
    bool Initialize(KoiloKernel& kernel) override;
    void Update(float dt) override;
    void Shutdown() override;

    /// Set/replace the render backend. Initializes if not already done.
    void SetBackend(std::unique_ptr<IRenderBackend> backend);

    /// Get the current render backend (may be nullptr).
    IRenderBackend* GetBackend() const { return backend_.get(); }

private:
    std::unique_ptr<IRenderBackend> backend_;
};

} // namespace koilo
