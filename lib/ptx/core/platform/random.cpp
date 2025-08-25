#include "random.hpp"

#if defined(ARDUINO)
  #include <Arduino.h>   // For random() and randomSeed()
#else
  #include <cstdlib>     // For std::rand() and std::srand()
#endif

namespace ptx::Random {

void Seed(std::uint32_t seed) {
#if defined(ARDUINO)
    randomSeed(seed);
#else
    std::srand(static_cast<unsigned int>(seed));
#endif
}

int Int(int min, int max) {
#if defined(ARDUINO)
    // The Arduino random(min, max) function is exclusive of max, so we add 1.
    return static_cast<int>(random(static_cast<long>(min), static_cast<long>(max) + 1));
#else
    // Using modulo for random number generation can introduce bias,
    // but it is simple and often sufficient for non-cryptographic purposes.
    int span = max - min + 1;
    if (span <= 0) return min;
    return min + (std::rand() % span);
#endif
}

float Float(float min, float max) {
#if defined(ARDUINO)
    // Generate a float between 0.0 and 0.9999...
    float norm = static_cast<float>(random(0L, 10000L)) / 10000.0f;
#else
    // Generate a float between 0.0 and 1.0
    float norm = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
#endif
    return min + (max - min) * norm;
}

} // namespace ptx::Random