#pragma once

#if defined(ARDUINO)
    #include <Arduino.h>
#else
    #include <chrono>
    #include <cstdint>
#endif

namespace uc3d::Time {

inline uint32_t Millis() {
#if defined(ARDUINO)
    return millis();
#else
    using namespace std::chrono;
    return static_cast<std::uint32_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()
    );
#endif
}

inline uint32_t Micros() {
#if defined(ARDUINO)
    return micros();
#else
    using namespace std::chrono;
    return static_cast<std::uint32_t>(
        duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count()
    );
#endif
}
} // namespace uc3d::Time 
