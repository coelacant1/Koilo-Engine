#include <ptx/systems/audio/audiomanager.hpp>
#include <algorithm>

namespace ptx {

// === AudioListener Implementation ===

AudioListener::AudioListener()
    : position(0, 0, 0), velocity(0, 0, 0), forward(0, 0, -1), up(0, 1, 0) {
}

void AudioListener::SetOrientation(const Vector3D& fwd, const Vector3D& u) {
    forward = fwd;
    up = u;
}

// === AudioManager Implementation ===

AudioManager::AudioManager()
    : masterVolume(1.0f), musicVolume(1.0f), sfxVolume(1.0f), voiceVolume(1.0f),
      maxSources(32), speedOfSound(343.0f), dopplerFactor(1.0f) {
}

AudioManager::~AudioManager() {
    Shutdown();
}

bool AudioManager::Initialize(int maxSrcs) {
    maxSources = maxSrcs;

    // TODO: Initialize audio backend (OpenAL, SDL_mixer, miniaudio, etc.)
    // For now, this is a stub implementation

    return true;
}

void AudioManager::Shutdown() {
    // Stop and remove all sources
    RemoveAllSources();

    // Unload all clips
    UnloadAllClips();

    // TODO: Shutdown audio backend
}

void AudioManager::Update(float deltaTime) {
    // Update all audio sources
    for (auto& source : sources) {
        if (source) {
            source->Update(deltaTime);
        }
    }

    // Remove stopped non-looping sources
    sources.erase(
        std::remove_if(sources.begin(), sources.end(),
            [](const std::shared_ptr<AudioSource>& src) {
                return src->IsStopped() && !src->IsLooping();
            }),
        sources.end()
    );

    // TODO: Update 3D audio calculations (distance attenuation, Doppler, etc.)
    // This would involve calculating volume/pitch adjustments based on:
    // - Distance from listener to each source
    // - Velocity of source and listener (Doppler effect)
    // - Listener orientation (for stereo panning)
}

// === Audio Clip Management ===

std::shared_ptr<AudioClip> AudioManager::LoadClip(const std::string& name, const std::string& filepath) {
    // Check if already loaded
    auto it = clips.find(name);
    if (it != clips.end()) {
        return it->second;
    }

    // Create and load new clip
    auto clip = std::make_shared<AudioClip>(name);
    if (clip->LoadFromFile(filepath)) {
        clips[name] = clip;
        return clip;
    }

    return nullptr;
}

std::shared_ptr<AudioClip> AudioManager::GetClip(const std::string& name) {
    auto it = clips.find(name);
    if (it != clips.end()) {
        return it->second;
    }
    return nullptr;
}

void AudioManager::UnloadClip(const std::string& name) {
    auto it = clips.find(name);
    if (it != clips.end()) {
        it->second->Unload();
        clips.erase(it);
    }
}

void AudioManager::UnloadAllClips() {
    for (auto& pair : clips) {
        pair.second->Unload();
    }
    clips.clear();
}

// === Audio Source Management ===

std::shared_ptr<AudioSource> AudioManager::CreateSource() {
    // Check if we've reached max sources
    if (static_cast<int>(sources.size()) >= maxSources) {
        // TODO: Implement source stealing based on priority
        return nullptr;
    }

    auto source = std::make_shared<AudioSource>();
    sources.push_back(source);
    return source;
}

std::shared_ptr<AudioSource> AudioManager::CreateSource(std::shared_ptr<AudioClip> clip) {
    auto source = CreateSource();
    if (source) {
        source->SetClip(clip);
    }
    return source;
}

void AudioManager::RemoveSource(std::shared_ptr<AudioSource> source) {
    sources.erase(
        std::remove(sources.begin(), sources.end(), source),
        sources.end()
    );
}

void AudioManager::RemoveAllSources() {
    for (auto& source : sources) {
        if (source) {
            source->Stop();
        }
    }
    sources.clear();
}

// === Quick Play Methods ===

std::shared_ptr<AudioSource> AudioManager::PlaySound(const std::string& clipName, float volume,
                                                     float pitch, bool loop) {
    auto clip = GetClip(clipName);
    if (!clip) {
        return nullptr;
    }

    auto source = CreateSource(clip);
    if (!source) {
        return nullptr;
    }

    source->SetVolume(volume);
    source->SetPitch(pitch);
    source->SetLoop(loop);
    source->SetSpatial(false);  // 2D sound
    source->Play();

    return source;
}

std::shared_ptr<AudioSource> AudioManager::PlaySound3D(const std::string& clipName, const Vector3D& position,
                                                       float volume, float pitch, bool loop) {
    auto clip = GetClip(clipName);
    if (!clip) {
        return nullptr;
    }

    auto source = CreateSource(clip);
    if (!source) {
        return nullptr;
    }

    source->SetPosition(position);
    source->SetVolume(volume);
    source->SetPitch(pitch);
    source->SetLoop(loop);
    source->SetSpatial(true);  // 3D sound
    source->Play();

    return source;
}

void AudioManager::StopAll() {
    for (auto& source : sources) {
        if (source) {
            source->Stop();
        }
    }
}

void AudioManager::PauseAll() {
    for (auto& source : sources) {
        if (source && source->IsPlaying()) {
            source->Pause();
        }
    }
}

void AudioManager::ResumeAll() {
    for (auto& source : sources) {
        if (source && source->IsPaused()) {
            source->Play();
        }
    }
}

// === Volume Controls ===

void AudioManager::SetMasterVolume(float volume) {
    masterVolume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioManager::SetMusicVolume(float volume) {
    musicVolume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioManager::SetSFXVolume(float volume) {
    sfxVolume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioManager::SetVoiceVolume(float volume) {
    voiceVolume = std::clamp(volume, 0.0f, 1.0f);
}

// === Advanced Settings ===

void AudioManager::SetDopplerFactor(float factor) {
    dopplerFactor = std::clamp(factor, 0.0f, 2.0f);
}

int AudioManager::GetActiveSourceCount() const {
    int count = 0;
    for (const auto& source : sources) {
        if (source && source->IsPlaying()) {
            count++;
        }
    }
    return count;
}

} // namespace ptx
