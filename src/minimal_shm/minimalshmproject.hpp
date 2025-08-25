#pragma once

#include "../../lib/ptx/project/project.hpp"
#include "../../lib/ptx/core/math/vector2d.hpp"
#include "../../lib/ptx/core/math/vector3d.hpp"
#include "../../lib/ptx/systems/render/core/cameralayout.hpp"
#include "../../lib/ptx/core/math/transform.hpp"
#include "../../lib/ptx/systems/render/core/pixelgroup.hpp"
#include "../../lib/ptx/systems/render/core/camera.hpp"
#include "../../lib/ptx/systems/render/core/cameramanager.hpp"
#include "../../lib/ptx/systems/hardware/virtualcontroller.hpp"
#include "../../lib/ptx/core/platform/console.hpp"
#include "../../lib/ptx/systems/render/material/implementations/spiralmaterial.hpp"

#include "ws35pixels.hpp"

class MinimalShmProject : public Project {
public:
    MinimalShmProject();
    void Initialize() override;
    void Update(float ratio) override;

private:
    static constexpr uint32_t kW = 192;
    static constexpr uint32_t kH = 96;
    static constexpr uint32_t kCount = kW * kH;

    RGBColor rainbowSpectrum[6] = {RGBColor(255, 0, 0), RGBColor(255, 255, 0), RGBColor(0, 255, 0), RGBColor(0, 255, 255), RGBColor(0, 0, 255), RGBColor(255, 0, 255)};
    SpiralMaterial<6> spiralMaterial = SpiralMaterial<6>(rainbowSpectrum, 3.0f, 7.0f);

    CameraLayout layout1_ { CameraLayout::ZForward, CameraLayout::YUp };
    CameraLayout layout2_ { CameraLayout::ZForward, CameraLayout::YUp };
    Transform    cam1Xform_ { Vector3D(), Vector3D(0,0,-500), Vector3D(1,1,1) };
    Transform    cam2Xform_ { Vector3D(), Vector3D(0,0,-500), Vector3D(1,1,1) };
    PixelGroup<kCount> pixels_ { Vector2D(float(kW) * 3.0f, float(kH) * 3.0f), Vector2D(0,0), kW };
    //PixelGroup<kCount> ws35pg { Vector2D(float(kW), float(kH)), Vector2D(0,0), kW };
    PixelGroup<571> ws35pg {WS35Layout::WS35Pixels, IPixelGroup::Direction::MAXTOZERO};
    Camera<kCount>     camera_ { &cam1Xform_, &layout1_, &pixels_ };
    Camera<571> ks35           { &cam2Xform_, &layout2_, &ws35pg };
    CameraManager      cams_   { new CameraBase*[2]{ &camera_, &ks35 }, 2 };

    VirtualController  vctrl_  { &cams_, "/ptx_ctrl", "/ptx_reg" };
};
