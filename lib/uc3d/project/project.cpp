#include "project.hpp"

Project::Project(CameraManager* cameras, Controller* controller, uint8_t numObjects) 
    : cameras(cameras), controller(controller), scene(numObjects) {
        
    previousAnimationTime = uc3d::Time::Micros();
    previousRenderTime = uc3d::Time::Micros();
    previousDisplayTime = uc3d::Time::Micros();
}

void Project::RenderStartTimer() {
    previousRenderTime = uc3d::Time::Micros();
}

void Project::RenderEndTimer() {
    renderTime = ((float)(uc3d::Time::Micros() - previousRenderTime)) / 1000000.0f;
}

float Project::GetAnimationTime() {
    return animationTime;
}

float Project::GetRenderTime() {
    return renderTime;
}

float Project::GetDisplayTime() {
    return displayTime;
}

float Project::GetFrameRate() {
    return avgFPS.Filter(1.0f / (renderTime + animationTime + displayTime));
}

void Project::Animate(float ratio) {
    previousAnimationTime = uc3d::Time::Micros();

    Update(ratio);

    animationTime = ((float)(uc3d::Time::Micros() - previousAnimationTime)) / 1000000.0f;
}

void Project::Render() {
    previousRenderTime = uc3d::Time::Micros();

    RenderingEngine::Rasterize(&scene, cameras);
    //RenderingEngine::DisplayWhite(cameras);

    renderTime = ((float)(uc3d::Time::Micros() - previousRenderTime)) / 1000000.0f;
}

void Project::Display() {
    previousDisplayTime = uc3d::Time::Micros();

    controller->Display();

    displayTime = ((float)(uc3d::Time::Micros() - previousDisplayTime)) / 1000000.0f;
}

void Project::PrintStats(){
    uc3d::Console::Print("FPS: ");
    uc3d::Console::Print(GetFrameRate(), 0);
    uc3d::Console::Print(", Animated in ");
    uc3d::Console::Print(GetAnimationTime(), 4);

    uc3d::Console::Print("s, Rendered in ");
    uc3d::Console::Print(GetRenderTime(), 4);

    uc3d::Console::Print("s, Displayed in ");
    uc3d::Console::Print(GetDisplayTime(), 4);
    uc3d::Console::Println("s");
}
