#include <ptx/systems/audio/audiosource.hpp>
#include <algorithm>

namespace ptx {

AudioSource::AudioSource()
    : clip(nullptr), state(AudioSourceState::Stopped), playbackPosition(0.0f),
      position(0, 0, 0), velocity(0, 0, 0), minDistance(1.0f), maxDistance(100.0f),
      rolloffFactor(1.0f), volume(1.0f), pitch(1.0f), pan(0.0f), loop(false),
      spatial(true), priority(128) {
}

AudioSource::AudioSource(std::shared_ptr<AudioClip> clip)
    : clip(clip), state(AudioSourceState::Stopped), playbackPosition(0.0f),
      position(0, 0, 0), velocity(0, 0, 0), minDistance(1.0f), maxDistance(100.0f),
      rolloffFactor(1.0f), volume(1.0f), pitch(1.0f), pan(0.0f), loop(false),
      spatial(true), priority(128) {
}

void AudioSource::Play() {
    if (clip == nullptr || !clip->IsLoaded()) {
        return;
    }

    state = AudioSourceState::Playing;

    // If we're at the end, restart from beginning
    if (playbackPosition >= clip->GetDuration()) {
        playbackPosition = 0.0f;
    }
}

void AudioSource::Pause() {
    if (state == AudioSourceState::Playing) {
        state = AudioSourceState::Paused;
    }
}

void AudioSource::Stop() {
    state = AudioSourceState::Stopped;
    playbackPosition = 0.0f;
}

void AudioSource::SetVolume(float vol) {
    volume = std::clamp(vol, 0.0f, 1.0f);
}

void AudioSource::SetPitch(float p) {
    pitch = std::clamp(p, 0.1f, 3.0f);
}

void AudioSource::SetPan(float p) {
    pan = std::clamp(p, -1.0f, 1.0f);
}

void AudioSource::SetPriority(int prio) {
    priority = std::clamp(prio, 0, 255);
}

void AudioSource::SetPlaybackPosition(float position) {
    if (clip == nullptr || !clip->IsLoaded()) {
        return;
    }

    playbackPosition = std::clamp(position, 0.0f, clip->GetDuration());
}

void AudioSource::Update(float deltaTime) {
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

} // namespace ptx
