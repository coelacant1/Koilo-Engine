#include "project.hpp"

Project::Project(CameraManager* cameras, uint8_t numObjects) 
    : cameras(cameras), scene(numObjects) {
        
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

    //controller->Display();

    displayTime = ((float)(uc3d::Time::Micros() - previousDisplayTime)) / 1000000.0f;
}

void Project::PrintStats(){
    #ifdef PRINTINFO
        #ifdef DEBUG
        Serial.print("Free memory ");
        Serial.print(Debug::FreeMem(), 3);
        Serial.print("Kb, ");
        #endif
        Serial.print("FPS: ");
        Serial.print(GetFrameRate(), 0);
        Serial.print(", Animated in ");
        Serial.print(GetAnimationTime(), 4);

        Serial.print("s, Rendered in ");
        Serial.print(GetRenderTime(), 4);

        Serial.print("s, Displayed in ");
        Serial.print(GetDisplayTime(), 4);
        Serial.println("s");
    #endif
}