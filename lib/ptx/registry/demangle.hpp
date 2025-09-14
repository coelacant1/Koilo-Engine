#pragma once
#include <string>
#if defined(__GNUG__)
  #include <cxxabi.h>
  #include <memory>
#endif

inline std::string Demangle(const char* name) {
#if defined(__GNUG__)
    int status = 0;
    std::unique_ptr<char, void(*)(void*)> uptr(
        abi::__cxa_demangle(name, nullptr, nullptr, &status), std::free);
    return status == 0 && uptr ? std::string(uptr.get()) : std::string(name);
#else
    return std::string(name);
#endif
}
