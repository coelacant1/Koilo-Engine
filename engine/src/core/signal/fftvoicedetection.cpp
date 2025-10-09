#include <ptx/core/signal/fftvoicedetection.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

FFTVoiceDetection::FFTVoiceDetection(size_t peakCountValue, uint8_t bandwidthValue)
    : peakCount(std::max<size_t>(1, peakCountValue)),
      bandwidth(std::max<uint8_t>(1, bandwidthValue)),
      peakDetection(peakCount, 8, 2.0f, 0.5f),
      peakSmoothing(10, 0.1f),
      peaksBinary(peakCount, false),
      peakDensity(peakCount, 0.0f) {}

void FFTVoiceDetection::SetThreshold(float thresholdValue) {
    threshold = thresholdValue;
}

float FFTVoiceDetection::GetViseme(MouthShape viseme) const {
    const size_t index = static_cast<size_t>(viseme);
    if (index >= visRatios.size()) {
        return 0.0f;
    }

    return *visRatios[index];
}

ptx::UString FFTVoiceDetection::ToString() const {
    float maxRatio = 0.0f;
    size_t winningIndex = std::numeric_limits<size_t>::max();
    ptx::UString output;

    for (size_t i = 0; i < kVisemeCount; ++i) {
        if (maxRatio < *visRatios[i]) {
            maxRatio = *visRatios[i];
            winningIndex = i;
        }
    }

    if (winningIndex < kVisemeCount) {
        output += ptx::UString::FromFloat(f1);
        output += ",";
        output += ptx::UString::FromFloat(f2);
        output += ",";
    }

    switch (winningIndex) {
        case EE:
            output += "EE";
            break;
        case AE:
            output += "AE";
            break;
        case UH:
            output += "UH";
            break;
        case AR:
            output += "AR";
            break;
        case ER:
            output += "ER";
            break;
        case AH:
            output += "AH";
            break;
        case OO:
            output += "OO";
            break;
        case SS:
            output += "SS";
            break;
        default:
            break;
    }

    return output;
}

void FFTVoiceDetection::ResetVisemes() {
    for (auto* ratio : visRatios) {
        *ratio = 0.0f;
    }

    std::fill(peaksBinary.begin(), peaksBinary.end(), false);
    std::fill(peakDensity.begin(), peakDensity.end(), 0.0f);
    f1 = 0.0f;
    f2 = 0.0f;
    peakSmoothing.Reset();
    peakDetection.Reset();
}

void FFTVoiceDetection::Update(const float* peaks, float maxFrequency) {
    if (peaks == nullptr || peakCount == 0) {
        return;
    }

    CalculateFormants(peaks);

    const float scale = (maxFrequency / 2.0f) / static_cast<float>(peakCount);
    f1 *= scale;
    f2 *= scale;

    CalculateVisemeGroup();
}

void FFTVoiceDetection::CalculateFormants(const float* peaks) {
    if (peaks == nullptr || peakCount == 0) {
        f1 = 0.0f;
        f2 = 0.0f;
        return;
    }

    const int total = static_cast<int>(peakCount);
    const int bw = std::max<int>(1, static_cast<int>(bandwidth));

    for (int i = 0; i < total; ++i) {
        float density = 0.0f;

        for (int j = 0; j < bw; ++j) {
            const int forwardIndex = i + j;
            const int backwardIndex = i - j;

            if (i < bw) {
                if (forwardIndex < total) {
                    density += peaks[forwardIndex];
                }
                if (backwardIndex > 0) {
                    density += peaks[backwardIndex];
                }
            } else if (i > total - bw) {
                if (backwardIndex >= 0) {
                    density += peaks[backwardIndex];
                }
                if (forwardIndex < total) {
                    density += peaks[forwardIndex];
                }
            } else {
                if (forwardIndex < total) {
                    density += peaks[forwardIndex];
                }
                if (backwardIndex >= 0) {
                    density += peaks[backwardIndex];
                }
            }
        }

        peakDensity[static_cast<size_t>(i)] = peakSmoothing.Filter(density);
    }

    peakDetection.Calculate(peakDensity.data(), peaksBinary);

    size_t tempCount = 0;
    size_t firstStart = 0;
    size_t firstCount = 0;
    size_t secondStart = 0;
    size_t secondCount = 0;

    auto finalizeRun = [&](size_t index) {
        if (tempCount == 0) {
            return;
        }

        const size_t start = index >= tempCount ? index - tempCount : 0;

        if (firstCount < tempCount) {
            secondCount = firstCount;
            secondStart = firstStart;
            firstCount = tempCount;
            firstStart = start;
        } else if (secondCount < tempCount) {
            secondCount = tempCount;
            secondStart = start;
        }

        tempCount = 0;
    };

    for (size_t i = 0; i < peakCount; ++i) {
        if (peaksBinary[i]) {
            ++tempCount;
        } else {
            finalizeRun(i);
        }
    }
    finalizeRun(peakCount);

    if (firstCount < 8) {
        secondStart = firstStart;
        secondCount = firstCount;
    }

    if (secondCount < 8) {
        secondStart = firstStart;
        secondCount = firstCount;
    }

    if (firstCount == 0 && secondCount == 0) {
        f1 = 0.0f;
        f2 = 0.0f;
        return;
    }

    if (firstStart <= secondStart) {
        f1 = static_cast<float>(firstStart + firstCount / 2);
        f2 = static_cast<float>(secondStart + secondCount / 2);
    } else {
        f1 = static_cast<float>(secondStart + secondCount / 2);
        f2 = static_cast<float>(firstStart + firstCount / 2);
    }
}

void FFTVoiceDetection::CalculateVisemeGroup() {
    for (auto* ratio : visRatios) {
        *ratio = 0.0f;
    }

    if (f1 > threshold || f2 > threshold) {
        Vector2D formant(f1, f2);
        size_t firstClosest = 0;
        float firstDistance = 1'000'000.0f;

        for (size_t i = 0; i < kVisemeCount; ++i) {
            const float distance = formant.CalculateEuclideanDistance(*coordinates[i]);
            if (distance < firstDistance) {
                firstClosest = i;
                firstDistance = distance;
            }
        }

        *visRatios[firstClosest] = 1.0f;
    }
}
