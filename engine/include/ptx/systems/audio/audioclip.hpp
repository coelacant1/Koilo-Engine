/**
 * @file audioclip.hpp
 * @brief Audio clip/buffer for loaded audio data.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "../../../registry/reflect_macros.hpp"

namespace ptx {

/**
 * @enum AudioFormat
 * @brief Supported audio formats.
 */
enum class AudioFormat : uint8_t {
    Mono8 = 0,      // 8-bit mono
    Mono16 = 1,     // 16-bit mono
    Stereo8 = 2,    // 8-bit stereo
    Stereo16 = 3    // 16-bit stereo
};

/**
 * @class AudioClip
 * @brief Represents a loaded audio clip with sample data.
 */
class AudioClip {
private:
    std::string name;
    std::vector<uint8_t> data;
    AudioFormat format;
    int sampleRate;
    float duration;
    bool loaded;

public:
    /**
     * @brief Default constructor.
     */
    AudioClip();

    /**
     * @brief Constructor with name.
     * @param name The name of the audio clip.
     */
    explicit AudioClip(const std::string& name);

    /**
     * @brief Loads audio data from file.
     * @param filepath Path to the audio file (WAV, OGG, MP3).
     * @return True if loaded successfully.
     */
    bool LoadFromFile(const std::string& filepath);

    /**
     * @brief Loads audio data from memory.
     * @param data Raw audio data.
     * @param dataSize Size of data in bytes.
     * @param format Audio format.
     * @param sampleRate Sample rate in Hz.
     * @return True if loaded successfully.
     */
    bool LoadFromMemory(const void* data, size_t dataSize, AudioFormat format, int sampleRate);

    /**
     * @brief Unloads audio data from memory.
     */
    void Unload();

    /**
     * @brief Gets the audio clip name.
     */
    std::string GetName() const { return name; }

    /**
     * @brief Sets the audio clip name.
     */
    void SetName(const std::string& name) { this->name = name; }

    /**
     * @brief Gets the raw audio data.
     */
    const std::vector<uint8_t>& GetData() const { return data; }

    /**
     * @brief Gets the audio format.
     */
    AudioFormat GetFormat() const { return format; }

    /**
     * @brief Gets the sample rate in Hz.
     */
    int GetSampleRate() const { return sampleRate; }

    /**
     * @brief Gets the duration in seconds.
     */
    float GetDuration() const { return duration; }

    /**
     * @brief Checks if the audio clip is loaded.
     */
    bool IsLoaded() const { return loaded; }

    /**
     * @brief Gets the size of the audio data in bytes.
     */
    size_t GetDataSize() const { return data.size(); }

    PTX_BEGIN_FIELDS(AudioClip)
        PTX_FIELD(AudioClip, name, "Name", 0, 0),
        PTX_FIELD(AudioClip, sampleRate, "Sample rate", 0, 192000),
        PTX_FIELD(AudioClip, duration, "Duration", 0.0f, 3600.0f),
        PTX_FIELD(AudioClip, loaded, "Loaded", 0, 1)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(AudioClip)
        PTX_METHOD_AUTO(AudioClip, LoadFromFile, "Load from file"),
        PTX_METHOD_AUTO(AudioClip, Unload, "Unload"),
        PTX_METHOD_AUTO(AudioClip, GetName, "Get name"),
        PTX_METHOD_AUTO(AudioClip, GetSampleRate, "Get sample rate"),
        PTX_METHOD_AUTO(AudioClip, GetDuration, "Get duration"),
        PTX_METHOD_AUTO(AudioClip, IsLoaded, "Is loaded")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(AudioClip)
        PTX_CTOR0(AudioClip),
        PTX_CTOR(AudioClip, const std::string&)
    PTX_END_DESCRIBE(AudioClip)
};

} // namespace ptx
