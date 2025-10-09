#include <ptx/systems/scene/animation/animationtrack.hpp>

AnimationTrack::AnimationTrack(float min,
                               float max,
                               KeyFrameInterpolation::InterpolationMethod interpMethod,
                               std::size_t parameterCapacity,
                               std::size_t keyFrameCapacity)
    : track(min, max, interpMethod, parameterCapacity, keyFrameCapacity) {}

void AnimationTrack::Play() {
    track.Play();
}

void AnimationTrack::Pause() {
    track.Pause();
}

void AnimationTrack::RestartTime() {
    track.SetCurrentTime(0.0f);
}

float AnimationTrack::GetTime() const {
    return track.GetCurrentTime();
}

void AnimationTrack::Reset() {
    track.Reset();
}

float AnimationTrack::GetParameterValue() const {
    return track.GetParameterValue();
}

float AnimationTrack::Update() {
    return track.Update();
}

void AnimationTrack::AddParameter(float* parameter) {
    track.AddParameter(parameter);
}

void AnimationTrack::SetTime(float time) {
    track.SetCurrentTime(time);
}

std::size_t AnimationTrack::GetNumKeyFrames() const {
    return track.GetKeyFrameCount();
}

std::size_t AnimationTrack::GetNumParameters() const {
    return track.GetParameterCount();
}

void AnimationTrack::AddKeyFrame(float time, float value) {
    track.AddKeyFrame(time, value);
}

void AnimationTrack::AddKeyFrame(const KeyFrame& keyFrame) {
    track.AddKeyFrame(keyFrame);
}

void AnimationTrack::RemoveKeyFrame(std::size_t index) {
    track.RemoveKeyFrame(index);
}

void AnimationTrack::SetRange(float min, float max) {
    track.SetRange(min, max);
}

void AnimationTrack::SetMin(float min) {
    track.SetMin(min);
}

void AnimationTrack::SetMax(float max) {
    track.SetMax(max);
}

float AnimationTrack::GetMin() const {
    return track.GetMin();
}

float AnimationTrack::GetMax() const {
    return track.GetMax();
}

void AnimationTrack::SetPlaybackSpeed(float playbackSpeed) {
    track.SetPlaybackSpeed(playbackSpeed);
}

float AnimationTrack::GetPlaybackSpeed() const {
    return track.GetPlaybackSpeed();
}

void AnimationTrack::SetInterpolationMethod(KeyFrameInterpolation::InterpolationMethod interpMethod) {
    track.SetInterpolationMethod(interpMethod);
}

KeyFrameInterpolation::InterpolationMethod AnimationTrack::GetInterpolationMethod() const {
    return track.GetInterpolationMethod();
}
