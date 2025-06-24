#pragma once
#if defined(ARDUINO)
  #include <Arduino.h>
  using string_t = String;
#else
  #include <string>
  #include <cstdint>
  using string_t = std::string;
#endif
