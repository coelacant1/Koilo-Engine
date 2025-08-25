// ustring.cpp
#include "ustring.hpp"

#if defined(ARDUINO)
  #include <Arduino.h>
#else
  #include <string>
  #include <utility>   // std::move
  #include <sstream>   // std::ostringstream
  #include <iomanip>   // std::fixed, std::setprecision
#endif

namespace ptx {

// Implementation container (Arduino String or std::string).
struct UString::PImpl {
#if defined(ARDUINO)
    String internal_string;
#else
    std::string internal_string;
#endif
};

UString::UString() : pimpl(new PImpl()) {}

UString::UString(const char* c_str) : pimpl(new PImpl()) {
    if (c_str) pimpl->internal_string = c_str;
}

UString::UString(const UString& other) : pimpl(new PImpl()) {
    if (other.pimpl) pimpl->internal_string = other.pimpl->internal_string;
}

UString::UString(UString&& other) noexcept : pimpl(other.pimpl) {
    other.pimpl = nullptr;
}

UString::~UString() {
    delete pimpl;
}

UString UString::FromFloat(float value, int precision) {
    UString result;
#if defined(ARDUINO)
    result.pimpl->internal_string = String(value, precision);
#else
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    result.pimpl->internal_string = ss.str();
#endif
    return result;
}

UString& UString::operator=(const UString& other) {
    if (this != &other) {
        if (other.pimpl) pimpl->internal_string = other.pimpl->internal_string;
        else pimpl->internal_string = "";
    }
    return *this;
}

UString& UString::operator=(UString&& other) noexcept {
    if (this != &other) {
        delete pimpl;
        pimpl = other.pimpl;
        other.pimpl = nullptr;
    }
    return *this;
}

UString& UString::operator=(const char* c_str) {
    pimpl->internal_string = c_str ? c_str : "";
    return *this;
}

UString& UString::operator+=(const char* c_str) {
    Append(c_str);
    return *this;
}

UString& UString::operator+=(const UString& other) {
    Append(other);
    return *this;
}

void UString::Append(const char* c_str) {
    if (!c_str || !pimpl) return;
#if defined(ARDUINO)
    pimpl->internal_string.concat(c_str);
#else
    pimpl->internal_string.append(c_str);
#endif
}

void UString::Append(const UString& other) {
    if (!pimpl || !other.pimpl) return;
#if defined(ARDUINO)
    pimpl->internal_string.concat(other.pimpl->internal_string);
#else
    pimpl->internal_string.append(other.pimpl->internal_string);
#endif
}

uint8_t UString::Length() const {
    return pimpl ? pimpl->internal_string.length() : 0;
}

bool UString::IsEmpty() const {
    return (pimpl == nullptr) || (pimpl->internal_string.length() == 0);
}

void UString::Clear() {
    if (!pimpl) return;
#if defined(ARDUINO)
    pimpl->internal_string = "";
#else
    pimpl->internal_string.clear();
#endif
}

const char* UString::CStr() const {
    if (!pimpl) return "";
    return pimpl->internal_string.c_str();
}

// --- Non-member operators ---

UString operator+(const UString& lhs, const UString& rhs) {
    UString result(lhs);
    result.Append(rhs);
    return result;
}

UString operator+(const UString& lhs, const char* rhs) {
    UString result(lhs);
    result.Append(rhs);
    return result;
}

UString operator+(const char* lhs, const UString& rhs) {
    UString result(lhs);
    result.Append(rhs);
    return result;
}

} // namespace ptx
