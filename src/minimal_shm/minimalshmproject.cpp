#include "minimalshmproject.hpp"

MinimalShmProject::MinimalShmProject() : Project() {}

void MinimalShmProject::Initialize() {
    SetCameraManager(&cams_);
    SetController(&vctrl_);
    vctrl_.Initialize();
}

void MinimalShmProject::Update(float ratio) {
    spiralMaterial.SetBend(3.0f + sinf(ratio * 5.0f) * 1.0f);
    spiralMaterial.SetPositionOffset(Vector2D(96.0f + (sinf(ratio * 5.0f) * 48.0f), 96.0f + (sinf(ratio * 5.0f) * 48.0f)));
    spiralMaterial.SetRotationAngle(ratio * 360.0f);
    spiralMaterial.Update(1.0f);

    IPixelGroup* pg1 = cams_.GetCameras()[0]->GetPixelGroup();
    const uint32_t n1 = pg1->GetPixelCount();
    RGBColor* cols1 = pg1->GetColors();

    for (uint32_t i = 0; i < n1; ++i) {
        float u = float(i % kW) / float(kW-1);
        float v = float(i / kW) / float(kH-1);

        const Vector3D pos = Vector3D(u*255, v*255, 0.0f);
        const Vector3D nul = Vector3D();
        SurfaceProperties sP = SurfaceProperties(pos, nul, nul);
        
        RGBColor rgb = spiralMaterial.GetShader()->Shade(sP, spiralMaterial);
        cols1[i] = rgb;
        //cols[i] = RGBColor(uint8_t(u*255), uint8_t(v*255), uint8_t((0.5f+0.5f*sinf(ratio))*255));
    }


    IPixelGroup* pg2 = cams_.GetCameras()[1]->GetPixelGroup();
    const uint32_t n2 = pg2->GetPixelCount();
    RGBColor* cols2 = pg2->GetColors();

    for (uint32_t i = 0; i < n2; ++i) {
        float u = pg2->GetCoordinate(i).X;
        float v = pg2->GetCoordinate(i).Y;

        const Vector3D pos = Vector3D(u*255, v*255, 0.0f);
        const Vector3D nul = Vector3D();
        SurfaceProperties sP = SurfaceProperties(pos, nul, nul);
        
        RGBColor rgb = spiralMaterial.GetShader()->Shade(sP, spiralMaterial);
        cols2[i] = rgb;
    }

    //uc3d::Console::Print(GetAnimationTime() / 1000.0f, 5);
    //uc3d::Console::Print("\t");
    //uc3d::Console::Print(GetDisplayTime() / 1000.0f, 5);
    //uc3d::Console::Print("\t");
    //uc3d::Console::Print(GetRenderTime() / 1000.0f, 5);
    //uc3d::Console::Print("\t");
    uc3d::Console::Println(GetFrameRate(), 2);
}
