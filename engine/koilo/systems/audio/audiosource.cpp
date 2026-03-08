// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/audio/audiosource.hpp>
#include <koilo/core/time/timemanager.hpp>
#include <algorithm>

namespace koilo {

koilo::AudioSource::AudioSource()
    : clip(nullptr), state(AudioSourceState::Stopped), playbackPosition(0.0f),
      position(0, 0, 0), velocity(0, 0, 0), minDistance(1.0f), maxDistance(100.0f),
      rolloffFactor(1.0f), volume(1.0f), pitch(1.0f), pan(0.0f), loop(false),
      spatial(true), priority(128) {
}

koilo::AudioSource::AudioSource(std::shared_ptr<AudioClip> clip)
    : clip(clip), state(AudioSourceState::Stopped), playbackPosition(0.0f),
      position(0, 0, 0), velocity(0, 0, 0), minDistance(1.0f), maxDistance(100.0f),
      rolloffFactor(1.0f), volume(1.0f), pitch(1.0f), pan(0.0f), loop(false),
      spatial(true), priority(128) {
}

void koilo::AudioSource::Play() {
    if (clip == nullptr || !clip->IsLoaded()) {
        return;
    }

    state = AudioSourceState::Playing;

    // If we're at the end, restart from beginning
    if (playbackPosition >= clip->GetDuration()) {
        playbackPosition = 0.0f;
    }
}

void koilo::AudioSource::Pause() {
    if (state == AudioSourceState::Playing) {
        state = AudioSourceState::Paused;
    }
}

void koilo::AudioSource::Stop() {
    state = AudioSourceState::Stopped;
    playbackPosition = 0.0f;
}

void koilo::AudioSource::SetVolume(float vol) {
    volume = std::clamp(vol, 0.0f, 1.0f);
}

void koilo::AudioSource::SetPitch(float p) {
    pitch = std::clamp(p, 0.1f, 3.0f);
}

void koilo::AudioSource::SetPan(float p) {
    pan = std::clamp(p, -1.0f, 1.0f);
}

void koilo::AudioSource::SetPriority(int prio) {
    priority = std::clamp(prio, 0, 255);
}

void koilo::AudioSource::SetPlaybackPosition(float position) {
    if (clip == nullptr || !clip->IsLoaded()) {
        return;
    }

    playbackPosition = std::clamp(position, 0.0f, clip->GetDuration());
}

void koilo::AudioSource::Update() {
    float deltaTime = TimeManager::GetInstance().GetDeltaTime();
    if (state != AudioSourceState::Playing) {
        return;
    }

    if (clip == nullptr || !clip->IsLoaded()) {
        Stop();
        return;
    }

    // Advance playback position
    playbackPosition += deltaTime * pitch;

    // Check if we've reached the end
    if (playbackPosition >= clip->GetDuration()) {
        if (loop) {
            // Loop back to start
            playbackPosition = 0.0f;
        } else {
            // Stop playing
            Stop();
        }
    }
}

} // namespace koilo
