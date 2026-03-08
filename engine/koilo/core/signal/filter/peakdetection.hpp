/**
 * @file PeakDetection.h
 * @brief Implements peak detection for time-series data using a sliding window approach.
 *
 * The `PeakDetection` class identifies peaks in a data stream by maintaining a moving
 * average and standard deviation and comparing incoming values against a dynamic threshold.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "../../../registry/reflect_macros.hpp"

/**
 * @class PeakDetection
 * @brief Detects peaks in time-series data using statistical methods.
 *
 * The runtime version of `PeakDetection` retains the original behaviour of the
 * template implementation while allowing the caller to choose the sample window
 * size at runtime. Incoming values are compared against a moving average and
 * standard deviation to determine whether they exceed a configurable threshold.
 */

class PeakDetection {
private:
    size_t sampleSize;              ///< Size of the sliding window used for peak detection.
    uint8_t lag;                    ///< Number of samples considered when updating the statistics.
    float threshold;                ///< Standard deviation multiplier that triggers a peak.
    float influence;                ///< Influence of a detected peak on subsequent calculations.
    std::vector<float> filteredData; ///< Filtered data used to blend peak influence.
    std::vector<float> averages;     ///< Cached moving averages of the window.
    std::vector<float> deviations;   ///< Cached moving standard deviations of the window.

    void GetStdDev(size_t start, size_t length, const float* data, float& avgRet, float& stdRet) const;

public:
    /**
     * @brief Constructs a peak detector for a specific sample window size.
     *
     * @param sampleSize   Number of elements considered when evaluating peaks.
     * @param lag          Number of samples used to compute the moving statistics (default: 12).
     * @param threshold    Standard deviation multiplier that triggers a peak (default: 0.75).
     * @param influence    Influence of a detected peak on subsequent statistics (default: 0.5).
     */
    PeakDetection(size_t sampleSize, uint8_t lag = 12, float threshold = 0.75f, float influence = 0.5f);

    /**
     * @brief Identifies peaks within the provided data buffer.
     *
     * @param data   Pointer to the input data array with `sampleSize` elements.
     * @param peaks  Output vector (resized to `sampleSize`) marked `true` for detected peaks.
     */
    void Calculate(const float* data, std::vector<bool>& peaks);

    /**
     * @brief Clears cached statistics, preparing the detector for a new sequence.
     */
    void Reset();

    /**
     * @brief Retrieves the configured sample window size.
     */
    size_t GetSampleSize() const { return sampleSize; }

    /**
     * @brief Returns the configured lag (statistics window) length.
     */
    uint8_t GetLag() const { return lag; }

    /**
     * @brief Returns the current peak detection threshold multiplier.
     */
    float GetThreshold() const { return threshold; }

    /**
     * @brief Returns the influence factor applied to detected peaks.
     */
    float GetInfluence() const { return influence; }

    PTX_BEGIN_FIELDS(PeakDetection)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(PeakDetection)
        PTX_METHOD_AUTO(PeakDetection, Calculate, "Calculate"),
        PTX_METHOD_AUTO(PeakDetection, Reset, "Reset"),
        PTX_METHOD_AUTO(PeakDetection, GetSampleSize, "Get sample size"),
        PTX_METHOD_AUTO(PeakDetection, GetLag, "Get lag"),
        PTX_METHOD_AUTO(PeakDetection, GetThreshold, "Get threshold"),
        PTX_METHOD_AUTO(PeakDetection, GetInfluence, "Get influence")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(PeakDetection)
        PTX_CTOR(PeakDetection, int, uint8_t, float, float)
    PTX_END_DESCRIBE(PeakDetection)

};
