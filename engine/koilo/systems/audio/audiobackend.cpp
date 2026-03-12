// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file audiobackend.cpp
 * @brief Audio backend implementation - device init, PCM mixing, 3D audio.
 *
 * Uses SDL3 audio when available (KL_AUDIO_SDL3), otherwise stubs out
 * device init so the engine builds headless without any audio library.
 */

#ifdef KL_AUDIO_SDL3
#include <SDL3/SDL.h>
#endif

#include <koilo/systems/audio/audiobackend.hpp>
#include <cstring>
#include <algorithm>

namespace koilo {

#ifdef KL_AUDIO_SDL3
// SDL3 audio stream callback - fills output buffer with mixed PCM
static void SDLCALL SDLAudioStreamCallback(void* userdata, SDL_AudioStream* stream,
                                           int additional_amount, int /*total_amount*/) {
    if (additional_amount <= 0) return;

    auto* self = static_cast<AudioBackend*>(userdata);
    auto channels = static_cast<unsigned int>(self->GetChannels());
    unsigned int frameCount = static_cast<unsigned int>(additional_amount) / (channels * sizeof(float));

    // Use a stack buffer for small amounts, heap for large
    constexpr int kStackLimit = 16384;
    float stackBuf[kStackLimit / sizeof(float)];
    float* buf = stackBuf;
    bool heapAlloc = false;
    if (additional_amount > kStackLimit) {
        buf = new float[additional_amount / sizeof(float)];
        heapAlloc = true;
    }

    std::memset(buf, 0, static_cast<size_t>(additional_amount));
    self->MixFrames(buf, frameCount);
    SDL_PutAudioStreamData(stream, buf, additional_amount);

    if (heapAlloc) delete[] buf;
}
#endif

AudioBackend::AudioBackend() {}

AudioBackend::~AudioBackend() {
    Shutdown();
}

bool AudioBackend::Initialize(int sampleRate, int channels) {
    if (initialized_) return true;

    sampleRate_ = sampleRate;
    channels_ = channels;

#ifdef KL_AUDIO_SDL3
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            return false;
        }
    }

    SDL_AudioSpec spec{};
    spec.format   = SDL_AUDIO_F32;
    spec.channels = channels;
    spec.freq     = sampleRate;

    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec,
        SDLAudioStreamCallback, this);
    if (!stream) {
        return false;
    }

    audioStream_ = stream;
    SDL_ResumeAudioStreamDevice(stream);
#endif

    initialized_ = true;
    return true;
}

void AudioBackend::Shutdown() {
    if (!initialized_) return;

#ifdef KL_AUDIO_SDL3
    if (audioStream_) {
        auto* stream = static_cast<SDL_AudioStream*>(audioStream_);
        SDL_PauseAudioStreamDevice(stream);
        SDL_DestroyAudioStream(stream);
        audioStream_ = nullptr;
    }
#endif

    initialized_ = false;

    std::lock_guard<std::mutex> lock(sourceMutex_);
    playingSources_.clear();
}

void AudioBackend::AddSource(std::shared_ptr<AudioSource> source, std::shared_ptr<AudioClip> clip) {
    std::lock_guard<std::mutex> lock(sourceMutex_);
    for (auto& ps : playingSources_) {
        if (ps.source.get() == source.get()) return;
    }
    playingSources_.push_back({source, clip, 0});
}

void AudioBackend::RemoveSource(AudioSource* source) {
    std::lock_guard<std::mutex> lock(sourceMutex_);
    playingSources_.erase(
        std::remove_if(playingSources_.begin(), playingSources_.end(),
            [source](const PlayingSource& ps) { return ps.source.get() == source; }),
        playingSources_.end());
}

void AudioBackend::ClearSources() {
    std::lock_guard<std::mutex> lock(sourceMutex_);
    playingSources_.clear();
}

void AudioBackend::MixFrames(float* output, unsigned int frameCount) {
    std::lock_guard<std::mutex> lock(sourceMutex_);

    for (auto it = playingSources_.begin(); it != playingSources_.end(); ) {
        auto& ps = *it;
        auto* src = ps.source.get();
        auto* clip = ps.clip.get();

        if (!src || !clip || !clip->IsLoaded() || src->GetState() != AudioSourceState::Playing) {
            ++it;
            continue;
        }

        const auto& data = clip->GetData();
        if (data.empty()) { ++it; continue; }

        // Determine source PCM format
        int srcChannels = (clip->GetFormat() == AudioFormat::Stereo16 || clip->GetFormat() == AudioFormat::Stereo8) ? 2 : 1;
        int bytesPerSample = (clip->GetFormat() == AudioFormat::Mono16 || clip->GetFormat() == AudioFormat::Stereo16) ? 2 : 1;
        int bytesPerFrame = srcChannels * bytesPerSample;

        float volume = src->GetVolume() * CalculateAttenuation(ps);
        float pan = src->IsSpatial() ? CalculatePan(ps) : src->GetPan();
        float leftGain  = volume * std::min(1.0f, 1.0f - pan);
        float rightGain = volume * std::min(1.0f, 1.0f + pan);

        for (unsigned int f = 0; f < frameCount; ++f) {
            if (ps.readCursor >= data.size()) {
                if (src->IsLooping()) {
                    ps.readCursor = 0;
                } else {
                    break;
                }
            }

            // Decode one sample (16-bit or 8-bit -> float)
            float sample = 0.0f;
            if (bytesPerSample == 2 && ps.readCursor + 1 < data.size()) {
                int16_t s16 = static_cast<int16_t>(data[ps.readCursor] | (data[ps.readCursor + 1] << 8));
                sample = s16 / 32768.0f;
            } else if (bytesPerSample == 1) {
                sample = (data[ps.readCursor] - 128) / 128.0f;
            }

            // Mix into stereo output
            output[f * channels_]     += sample * leftGain;
            if (channels_ >= 2) {
                output[f * channels_ + 1] += sample * rightGain;
            }

            ps.readCursor += static_cast<size_t>(bytesPerFrame);
        }

        // If cursor past end and not looping, source stops
        if (ps.readCursor >= data.size() && !src->IsLooping()) {
            src->Stop();
        }

        ++it;
    }
}

// -- 3D Audio helpers --------------------------------------------------------

float AudioBackend::CalculateAttenuation(const PlayingSource& ps) const {
    if (!ps.source->IsSpatial()) return 1.0f;

    float dist = (ps.source->GetPosition() - listenerPos_).Magnitude();
    float minDist = ps.source->GetMinDistance();
    float maxDist = ps.source->GetMaxDistance();
    float rolloff = ps.source->GetRolloffFactor();

    if (dist <= minDist) return 1.0f;
    if (dist >= maxDist) return 0.0f;

    // Inverse distance clamped model (OpenAL-compatible)
    return minDist / (minDist + rolloff * (dist - minDist));
}

float AudioBackend::CalculatePan(const PlayingSource& ps) const {
    if (!ps.source->IsSpatial()) return 0.0f;

    Vector3D toSource = ps.source->GetPosition() - listenerPos_;
    float len = toSource.Magnitude();
    if (len < 0.001f) return 0.0f;

    // Right vector from listener forward (cross with world up)
    Vector3D up(0, 1, 0);
    Vector3D right = listenerFwd_.CrossProduct(up);
    float rightLen = right.Magnitude();
    if (rightLen < 0.001f) return 0.0f;
    right = right * (1.0f / rightLen);

    float dot = toSource.DotProduct(right) / len;
    return std::max(-1.0f, std::min(1.0f, dot));
}

} // namespace koilo
