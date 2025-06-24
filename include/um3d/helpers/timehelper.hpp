#if defined(ARDUINO)
    #include <Arduino.h>
#else
    #include <chrono>

    static std::uint32_t millis(){
        using namespace std::chrono;

        return static_cast<std::uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
    }

    static std::uint32_t micros(){
        using namespace std::chrono;

        return static_cast<std::uint32_t>(duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
    }
#endif
