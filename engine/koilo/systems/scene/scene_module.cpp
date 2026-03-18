// SPDX-License-Identifier: GPL-3.0-or-later
#include "scene_module.hpp"
#include "scene.hpp"
#include "camera/camera.hpp"
#include "camera/cameralayout.hpp"
#include <koilo/systems/render/core/pixelgroup.hpp>
#include <koilo/core/math/transform.hpp>
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/kernel/kernel.hpp>

namespace koilo {

SceneModule::SceneModule() = default;
SceneModule::~SceneModule() { Shutdown(); }

ModuleInfo SceneModule::GetInfo() const {
    return {"scene", "0.1.0", ModulePhase::Core};
}

bool SceneModule::Initialize(KoiloKernel& kernel) {
    kernel_ = &kernel;
    // Camera/scene built later via BuildCamera() once display config is known
    return true;
}

void SceneModule::Update(float /*dt*/) {
    // Scene updates are driven by the script engine
}

void SceneModule::Shutdown() {
    if (kernel_) {
        auto& svc = kernel_->Services();
        if (svc.Has("scene"))  svc.Unregister("scene");
        if (svc.Has("camera")) svc.Unregister("camera");
    }
    camera_.reset();
    cameraLayout_.reset();
    cameraTransform_.reset();
    pixelGroup_.reset();
    scene_.reset();
}

void SceneModule::BuildCamera(int pixelWidth, int pixelHeight) {
    Vector2D size(static_cast<float>(pixelWidth), static_cast<float>(pixelHeight));
    Vector2D position(0, 0);
    pixelGroup_ = std::make_unique<PixelGroup>(pixelWidth * pixelHeight, size, position, pixelWidth);

    Vector3D camPos(0, 0, 8);
    cameraTransform_ = std::make_unique<Transform>(Vector3D(0, 0, 0), camPos, Vector3D(1, 1, 1));
    cameraLayout_ = std::make_unique<CameraLayout>(CameraLayout::YNForward, CameraLayout::ZUp);
    camera_ = std::make_unique<Camera>(cameraTransform_.get(), cameraLayout_.get(), pixelGroup_.get());

    scene_ = std::make_unique<Scene>();

    // Register as script globals
    auto* engine = kernel_ ? kernel_->Services().Get<scripting::KoiloScriptEngine>("script") : nullptr;
    if (engine) {
        if (scene_)  engine->RegisterGlobal("scene", "Scene", scene_.get());
        if (camera_) engine->RegisterGlobal("cam", "Camera", camera_.get());
    }

    // Register as kernel services
    if (kernel_) {
        if (scene_)  kernel_->Services().Register("scene", scene_.get());
        if (camera_) kernel_->Services().Register("camera", camera_.get());
    }
}

} // namespace koilo
