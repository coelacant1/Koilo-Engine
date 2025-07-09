#include "ustring.hpp"

#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <string>
#include <utility> // For std::move
#endif

// Define the actual implementation structure inside the .cpp file.
struct uc3d::UString::PImpl {
#if defined(ARDUINO)
    String internal_string;
#else
    std::string internal_string;
#endif
};

namespace uc3d {

UString::UString() : pimpl(new PImpl()) {}

UString::UString(const char* c_str) : pimpl(new PImpl()) {
    if (c_str) {
        pimpl->internal_string = c_str;
    }
}

UString::UString(const UString& other) : pimpl(new PImpl()) {
    if (other.pimpl) { // Guard against copying from a moved-from object
        pimpl->internal_string = other.pimpl->internal_string;
    }
}

UString::UString(UString&& other) noexcept : pimpl(other.pimpl) {
    other.pimpl = nullptr;
}

UString::~UString() {
    delete pimpl;
}

UString UString::FromFloat(float value, int precision) {
    UString result; // Create a new UString
#if defined(ARDUINO)
    // Arduino's String class has a convenient constructor for this
    result.pimpl->internal_string = String(value, precision);
#else
    // Native C++ uses a string stream for formatted output
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    result.pimpl->internal_string = ss.str();
#endif
    return result;
}

UString& UString::operator=(const UString& other) {
    if (this != &other) {
        if (other.pimpl) {
            pimpl->internal_string = other.pimpl->internal_string;
        } else {
            pimpl->internal_string = "";
        }
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
    if (c_str) {
        pimpl->internal_string = c_str;
    } else {
        pimpl->internal_string = "";
    }
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

size_t UString::Length() const {
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
#if defined(ARDUINO)
    return pimpl->internal_string.c_str();
#else
    return pimpl->internal_string.c_str();
#endif
}

UString operator+(const UString& lhs, const UString& rhs) {
    UString result(lhs); // Start with the left-hand side
    result.Append(rhs);  // Append the right-hand side
    return result;
}

UString operator+(const UString& lhs, const char* rhs) {
    UString result(lhs);
    result.Append(rhs);
    return result;
}

// This is the specific overload that fixes your problem.
UString operator+(const char* lhs, const UString& rhs) {
    UString result(lhs); // Create a new UString from the C-style string
    result.Append(rhs);  // Append the UString to it
    return result;
}

} // namespace uc3d
