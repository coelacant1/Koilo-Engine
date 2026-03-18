// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file scene_module.hpp
 * @brief Scene/Camera subsystem as an IModule with Core lifecycle phase.
 *
 * Owns Scene, Camera, PixelGroup, CameraLayout, and camera Transform.
 * Created during BuildScene based on display configuration, then
 * registered as script globals and kernel services.
 *
 * @date 01/28/2026
 * @author Coela
 */
#pragma once

#include <koilo/kernel/unified_module.hpp>
#include <memory>

namespace koilo {

class Scene;
class Camera;
class CameraLayout;
class PixelGroup;
class Transform;

class SceneModule : public IModule {
public:
    SceneModule();
    ~SceneModule() override;

    ModuleInfo GetInfo() const override;
    bool Initialize(KoiloKernel& kernel) override;
    void Update(float dt) override;
    void Shutdown() override;

    /// Build the camera and scene from display dimensions.
    /// Called by the engine after display config is read from the script.
    void BuildCamera(int pixelWidth, int pixelHeight);

    Scene*      GetScene()      const { return scene_.get(); }
    Camera*     GetCamera()     const { return camera_.get(); }
    PixelGroup* GetPixelGroup() const { return pixelGroup_.get(); }

private:
    std::unique_ptr<PixelGroup>   pixelGroup_;
    std::unique_ptr<Transform>    cameraTransform_;
    std::unique_ptr<CameraLayout> cameraLayout_;
    std::unique_ptr<Camera>       camera_;
    std::unique_ptr<Scene>        scene_;
};

} // namespace koilo
