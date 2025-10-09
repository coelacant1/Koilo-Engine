#pragma once

#if defined(ARDUINO)
    #include <Arduino.h>
#else
    #include <chrono>
    #include <cstdint>
#endif

#include "../../registry/reflect_macros.hpp"

namespace ptx {
namespace Time {

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
    struct Reflection {
        static uint32_t Millis() { return ::ptx::Time::Millis(); }
        static uint32_t Micros() { return ::ptx::Time::Micros(); }

        PTX_BEGIN_FIELDS(Reflection)
            /* No reflected fields. */
        PTX_END_FIELDS

        PTX_BEGIN_METHODS(Reflection)
            PTX_SMETHOD_OVLD0(Reflection, Millis, uint32_t),
            PTX_SMETHOD_OVLD0(Reflection, Micros, uint32_t)
        PTX_END_METHODS

        PTX_BEGIN_DESCRIBE(Reflection)
        PTX_END_DESCRIBE(Reflection)
    };
} // namespace Time 
} // namespace ptx
