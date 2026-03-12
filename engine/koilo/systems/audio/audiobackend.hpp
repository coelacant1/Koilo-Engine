// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file audiobackend.hpp
 * @brief Audio backend for desktop playback.
 *
 * Owns the audio device, mixes active sources, and sends PCM to speakers.
 * Uses SDL3 audio when available, otherwise stubs out device init.
 * Not reflected (internal implementation detail).
 *
 * @date 02/22/2026
 * @author Coela
 */

#pragma once

#include <koilo/systems/audio/audioclip.hpp>
#include <koilo/systems/audio/audiosource.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <vector>
#include <memory>
#include <mutex>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

class AudioListener;

/**
 * @class AudioBackend
 * @brief Low-level audio device management via SDL3.
 */
class AudioBackend {
public:
    AudioBackend();
    ~AudioBackend();

    bool Initialize(int sampleRate = 44100, int channels = 2);
    void Shutdown();
    bool IsInitialized() const { return initialized_; }

    // Source management for the mixer
    void AddSource(std::shared_ptr<AudioSource> source, std::shared_ptr<AudioClip> clip);
    void RemoveSource(AudioSource* source);
    void ClearSources();

    // Listener for 3D audio
    void SetListenerPosition(const Vector3D& pos) { listenerPos_ = pos; }
    void SetListenerForward(const Vector3D& fwd) { listenerFwd_ = fwd; }

    int GetSampleRate() const { return sampleRate_; }
    int GetChannels() const { return channels_; }

    // Called by audio callback on audio thread
    void MixFrames(float* output, unsigned int frameCount);

private:
    struct PlayingSource {
        std::shared_ptr<AudioSource> source;
        std::shared_ptr<AudioClip> clip;
        size_t readCursor = 0;  // byte offset into clip data

        KL_BEGIN_FIELDS(PlayingSource)
            KL_FIELD(PlayingSource, source, "Source", 0, 0),
            KL_FIELD(PlayingSource, clip, "Clip", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(PlayingSource)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(PlayingSource)
            /* No reflected ctors. */
        KL_END_DESCRIBE(PlayingSource)

    };

    // 3D audio helpers
    float CalculateAttenuation(const PlayingSource& ps) const;
    float CalculatePan(const PlayingSource& ps) const;

    bool initialized_ = false;
    int sampleRate_ = 44100;
    int channels_ = 2;
    void* audioStream_ = nullptr;  // SDL_AudioStream* (nullptr = no device)

    std::mutex sourceMutex_;
    std::vector<PlayingSource> playingSources_;
    Vector3D listenerPos_;
    Vector3D listenerFwd_{0, 0, -1};

    KL_BEGIN_FIELDS(AudioBackend)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(AudioBackend)
        KL_METHOD_AUTO(AudioBackend, Initialize, "Initialize"),
        KL_METHOD_AUTO(AudioBackend, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(AudioBackend, IsInitialized, "Is initialized"),
        KL_METHOD_AUTO(AudioBackend, AddSource, "Add source"),
        KL_METHOD_AUTO(AudioBackend, RemoveSource, "Remove source"),
        KL_METHOD_AUTO(AudioBackend, ClearSources, "Clear sources"),
        KL_METHOD_AUTO(AudioBackend, SetListenerForward, "Set listener forward"),
        KL_METHOD_AUTO(AudioBackend, GetSampleRate, "Get sample rate"),
        KL_METHOD_AUTO(AudioBackend, GetChannels, "Get channels"),
        KL_METHOD_AUTO(AudioBackend, MixFrames, "Mix frames")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AudioBackend)
        KL_CTOR0(AudioBackend)
    KL_END_DESCRIBE(AudioBackend)

};

} // namespace koilo
