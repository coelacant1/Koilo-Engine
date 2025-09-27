#include <ptx/project/project.hpp>

Project::Project(CameraManager* cameras, Controller* controller, uint8_t numObjects) 
    : cameras(cameras), controller(controller), scene(numObjects) {
        
    previousAnimationTime = ptx::Time::Micros();
    previousRenderTime = ptx::Time::Micros();
    previousDisplayTime = ptx::Time::Micros();
}

void Project::RenderStartTimer() {
    previousRenderTime = ptx::Time::Micros();
}

void Project::RenderEndTimer() {
    renderTime = ((float)(ptx::Time::Micros() - previousRenderTime)) / 1000000.0f;
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
    previousAnimationTime = ptx::Time::Micros();

    Update(ratio);

    animationTime = ((float)(ptx::Time::Micros() - previousAnimationTime)) / 1000000.0f;
}

void Project::Render() {
    previousRenderTime = ptx::Time::Micros();

    RenderingEngine::Rasterize(&scene, cameras);
    //RenderingEngine::DisplayWhite(cameras);

    renderTime = ((float)(ptx::Time::Micros() - previousRenderTime)) / 1000000.0f;
}

void Project::Display() {
    previousDisplayTime = ptx::Time::Micros();

    controller->Display();

    displayTime = ((float)(ptx::Time::Micros() - previousDisplayTime)) / 1000000.0f;
}

void Project::PrintStats(){
    ptx::Console::Print("FPS: ");
    ptx::Console::Print(GetFrameRate(), 0);
    ptx::Console::Print(", Animated in ");
    ptx::Console::Print(GetAnimationTime(), 4);

    ptx::Console::Print("s, Rendered in ");
    ptx::Console::Print(GetRenderTime(), 4);

    ptx::Console::Print("s, Displayed in ");
    ptx::Console::Print(GetDisplayTime(), 4);
    ptx::Console::Println("s");
}
