/**
 * @file FFTVoiceDetection.h
 * @brief Declares the FFTVoiceDetection class for real-time viseme detection based on FFT data.
 *
 * This file defines the FFTVoiceDetection class, which extends the Viseme class to provide functionality
 * for detecting mouth shapes (visemes) based on formant frequencies extracted from FFT analysis of voice signals.
 *
 * @date 22/12/2024
 * @author Coela Can't
 */

#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "filter/peakdetection.hpp"
#include "filter/runningaveragefilter.hpp"
#include "../math/vector2d.hpp"
#include "../platform/ustring.hpp"
#include "../../registry/reflect_macros.hpp"

/**
 * @class Viseme
 * @brief Defines the available mouth shapes (visemes).
 */
class Viseme {
public:
    /**
     * @enum MouthShape
     * @brief Enumerates the possible mouth shapes for viseme detection.
     */
    enum MouthShape {
        EE, ///< Mouth shape corresponding to the "EE" sound.
        AE, ///< Mouth shape corresponding to the "AE" sound.
        UH, ///< Mouth shape corresponding to the "UH" sound.
        AR, ///< Mouth shape corresponding to the "AR" sound.
        ER, ///< Mouth shape corresponding to the "ER" sound.
        AH, ///< Mouth shape corresponding to the "AH" sound.
        OO, ///< Mouth shape corresponding to the "OO" sound.
        SS  ///< Mouth shape corresponding to the "SS" sound (optional).
    };

    PTX_BEGIN_FIELDS(Viseme)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Viseme)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Viseme)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(Viseme)

};

/**
 * @class FFTVoiceDetection
 * @brief Detects visemes based on FFT voice analysis.
 *
 * The FFTVoiceDetection class uses formant frequencies (F1 and F2) derived from FFT peaks
 * to detect and assign probabilities to various mouth shapes (visemes). It employs peak detection,
 * smoothing filters, and threshold-based calculations to determine the most probable viseme.
 *
 */
class FFTVoiceDetection : public Viseme {
private:
    static constexpr uint8_t kVisemeCount = 7; ///< Number of supported visemes.

    // Formant frequency coordinates for each viseme.
    Vector2D visEE = Vector2D(350.0f, 3200.0f); ///< Coordinates for "EE".
    Vector2D visAE = Vector2D(500.0f, 2700.0f); ///< Coordinates for "AE".
    Vector2D visUH = Vector2D(1100.0f, 2700.0f); ///< Coordinates for "UH".
    Vector2D visAR = Vector2D(850.0f, 850.0f); ///< Coordinates for "AR".
    Vector2D visER = Vector2D(1000.0f, 1000.0f); ///< Coordinates for "ER".
    Vector2D visAH = Vector2D(900.0f, 2400.0f); ///< Coordinates for "AH".
    Vector2D visOO = Vector2D(600.0f, 600.0f); ///< Coordinates for "OO".

    std::array<Vector2D*, kVisemeCount> coordinates { &visEE, &visAE, &visUH, &visAR, &visER, &visAH, &visOO }; ///< Array of viseme coordinates.

    // Viseme probabilities.
    float visRatioEE = 0.0f; ///< Probability for "EE".
    float visRatioAE = 0.0f; ///< Probability for "AE".
    float visRatioUH = 0.0f; ///< Probability for "UH".
    float visRatioAR = 0.0f; ///< Probability for "AR".
    float visRatioER = 0.0f; ///< Probability for "ER".
    float visRatioAH = 0.0f; ///< Probability for "AH".
    float visRatioOO = 0.0f; ///< Probability for "OO".

    std::array<float*, kVisemeCount> visRatios { &visRatioEE, &visRatioAE, &visRatioUH, &visRatioAR, &visRatioER, &visRatioAH, &visRatioOO }; ///< Array of viseme probabilities.

    size_t peakCount; ///< Number of FFT peaks analysed per update.
    uint8_t bandwidth; ///< Bandwidth used for formant smoothing.

    PeakDetection peakDetection; ///< Peak detection instance.
    RunningAverageFilter peakSmoothing; ///< Smoothing filter for peak data.

    std::vector<bool> peaksBinary; ///< Binary array indicating peak presence.
    std::vector<float> peakDensity; ///< Array of peak densities.

    float f1 = 0.0f; ///< Formant frequency F1.
    float f2 = 0.0f; ///< Formant frequency F2.

    float threshold = 400.0f; ///< Threshold for formant calculations.

    /**
     * @brief Calculates formant frequencies (F1 and F2) from FFT peaks.
     *
     * @param peaks Pointer to the FFT peak magnitudes used for analysis.
     */
    void CalculateFormants(const float* peaks);

    /**
     * @brief Calculates the viseme group probabilities based on formants.
     */
    void CalculateVisemeGroup();

public:
    /**
     * @brief Constructs a new FFTVoiceDetection instance.
     */
    explicit FFTVoiceDetection(size_t peakCount = 64, uint8_t bandwidth = 5);

    /**
     * @brief Sets the threshold for formant calculations.
     *
     * @param threshold The new threshold value.
     */
    void SetThreshold(float threshold);

    /**
     * @brief Retrieves the probability of a specific viseme.
     *
     * @param viseme The viseme to query.
     * @return The probability of the specified viseme (0.0 - 1.0).
     */
    float GetViseme(MouthShape viseme) const;

    /**
     * @brief Prints the probabilities of all visemes to the serial console.
     */
    ptx::UString ToString() const;

    /**
     * @brief Resets all viseme probabilities to zero.
     */
    void ResetVisemes();

    /**
     * @brief Updates the viseme probabilities based on new FFT data.
     *
     * @param peaks Array of FFT peak values.
     * @param maxFrequency Maximum frequency represented in the FFT data.
     */
    void Update(const float* peaks, float maxFrequency);

    /**
     * @brief Returns the configured number of peaks evaluated each frame.
     */
    size_t GetPeakCount() const { return peakCount; }

    PTX_BEGIN_FIELDS(FFTVoiceDetection)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(FFTVoiceDetection)
        PTX_METHOD_AUTO(FFTVoiceDetection, SetThreshold, "Set threshold"),
        PTX_METHOD_AUTO(FFTVoiceDetection, GetViseme, "Get viseme"),
        PTX_METHOD_AUTO(FFTVoiceDetection, ToString, "To string"),
        PTX_METHOD_AUTO(FFTVoiceDetection, ResetVisemes, "Reset visemes"),
        PTX_METHOD_AUTO(FFTVoiceDetection, Update, "Update"),
        PTX_METHOD_AUTO(FFTVoiceDetection, GetPeakCount, "Get peak count")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(FFTVoiceDetection)
        PTX_CTOR(FFTVoiceDetection, int, uint8_t)
    PTX_END_DESCRIBE(FFTVoiceDetection)

};
