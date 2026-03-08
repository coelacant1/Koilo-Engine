// SPDX-License-Identifier: GPL-3.0-or-later
// ustring.cpp
#include <koilo/core/platform/ustring.hpp>

#if defined(ARDUINO)
  #include <Arduino.h>
#else
  #include <string>
  #include <utility>   // std::move
  #include <sstream>   // std::ostringstream
  #include <iomanip>   // std::fixed, std::setprecision
#endif

namespace koilo {

// Implementation container (Arduino String or std::string).
struct koilo::UString::PImpl {
#if defined(ARDUINO)
    String internal_string;
#else
    std::string internal_string;
#endif
};

koilo::UString::UString() : pimpl(new PImpl()) {}

koilo::UString::UString(const char* c_str) : pimpl(new PImpl()) {
    if (c_str) pimpl->internal_string = c_str;
}

koilo::UString::UString(const UString& other) : pimpl(new PImpl()) {
    if (other.pimpl) pimpl->internal_string = other.pimpl->internal_string;
}

koilo::UString::UString(UString&& other) noexcept : pimpl(other.pimpl) {
    other.pimpl = nullptr;
}

koilo::UString::~UString() {
    delete pimpl;
}

UString koilo::UString::FromFloat(float value, int precision) {
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

UString& koilo::UString::operator=(const UString& other) {
    if (this != &other) {
        if (other.pimpl) pimpl->internal_string = other.pimpl->internal_string;
        else pimpl->internal_string = "";
    }
    return *this;
}

UString& koilo::UString::operator=(UString&& other) noexcept {
    if (this != &other) {
        delete pimpl;
        pimpl = other.pimpl;
        other.pimpl = nullptr;
    }
    return *this;
}

UString& koilo::UString::operator=(const char* c_str) {
    pimpl->internal_string = c_str ? c_str : "";
    return *this;
}

UString& koilo::UString::operator+=(const char* c_str) {
    Append(c_str);
    return *this;
}

UString& koilo::UString::operator+=(const UString& other) {
    Append(other);
    return *this;
}

void koilo::UString::Append(const char* c_str) {
    if (!c_str || !pimpl) return;
#if defined(ARDUINO)
    pimpl->internal_string.concat(c_str);
#else
    pimpl->internal_string.append(c_str);
#endif
}

void koilo::UString::Append(const UString& other) {
    if (!pimpl || !other.pimpl) return;
#if defined(ARDUINO)
    pimpl->internal_string.concat(other.pimpl->internal_string);
#else
    pimpl->internal_string.append(other.pimpl->internal_string);
#endif
}

uint8_t koilo::UString::Length() const {
    return pimpl ? pimpl->internal_string.length() : 0;
}

bool koilo::UString::IsEmpty() const {
    return (pimpl == nullptr) || (pimpl->internal_string.length() == 0);
}

void koilo::UString::Clear() {
    if (!pimpl) return;
#if defined(ARDUINO)
    pimpl->internal_string = "";
#else
    pimpl->internal_string.clear();
#endif
}

const char* koilo::UString::CStr() const {
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

} // namespace koilo
