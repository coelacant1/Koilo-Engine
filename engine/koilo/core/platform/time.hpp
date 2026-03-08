// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#if defined(ARDUINO)
    #include <Arduino.h>
#else
    #include <chrono>
    #include <cstdint>
#endif

#include <koilo/registry/reflect_macros.hpp>

namespace koilo {
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
        static uint32_t Millis() { return ::koilo::Time::Millis(); }
        static uint32_t Micros() { return ::koilo::Time::Micros(); }

        KL_BEGIN_FIELDS(Reflection)
            /* No reflected fields. */
        KL_END_FIELDS

        KL_BEGIN_METHODS(Reflection)
            KL_SMETHOD_OVLD0(Reflection, Millis, uint32_t),
            KL_SMETHOD_OVLD0(Reflection, Micros, uint32_t)
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(Reflection)
        KL_END_DESCRIBE(Reflection)
    };
} // namespace Time 
} // namespace koilo
