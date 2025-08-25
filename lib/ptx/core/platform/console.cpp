#include "console.hpp"

#if defined(ARDUINO)
  #include <Arduino.h>
#else
  #include <iostream>
#endif

namespace ptx::Console {

void Begin(std::uint32_t baud) {
#if defined(ARDUINO)
    Serial.begin(baud);
    // Optional: wait for serial port to connect. Needed for native USB on some boards.
    // while (!Serial) { ; }
#else
    // This is a no-op on native platforms, but we acknowledge the parameter
    // to prevent "unused parameter" warnings.
    (void)baud;
#endif
}

// --- Print Implementations ---

void Print(const char* msg) {
#if defined(ARDUINO)
    Serial.print(msg);
#else
    std::cout << msg;
#endif
}

void Print(int value) {
#if defined(ARDUINO)
    Serial.print(value);
#else
    std::cout << value;
#endif
}

void Print(float value, int precision) {
#if defined(ARDUINO)
    Serial.print(value, precision);
#else
    // NOTE: This sets the state of std::cout for subsequent prints.
    // A more advanced implementation might save and restore the previous state.
    std::cout << std::fixed;
    std::cout.precision(precision);
    std::cout << value;
#endif
}

// --- Println Implementations (re-use Print for simplicity) ---

void Println() {
#if defined(ARDUINO)
    Serial.println();
#else
    std::cout << std::endl;
#endif
}

void Println(const char* msg) {
    Print(msg);
    Println();
}

void Println(int value) {
    Print(value);
    Println();
}

void Println(float value, int precision) {
    Print(value, precision);
    Println();
}

} // namespace ptx::Console
