#pragma once

#include <cstddef> // For size_t

// Forward-declare the underlying implementation types to keep the header clean.
#if defined(ARDUINO)
class String;
#else
#include <string>
//#include <utility>
#include <iomanip>
#endif

namespace um3d {

/**
 * @class UString
 * @brief A platform-agnostic string class for maximum portability.
 */
class UString {
public:
    // --- Constructors & Destructor ---
    UString();
    UString(const char* c_str);
    UString(const UString& other);
    UString(UString&& other) noexcept;
    ~UString();

    // --- Assignment Operators ---
    /**
     * @brief Creates a UString from a float value with a specified precision.
     * @param value The floating-point number to convert.
     * @param precision The number of digits after the decimal point.
     * @return A new UString representing the formatted float.
     */

    static UString FromFloat(float value, int precision = 2);

    UString& operator=(const UString& other);
    UString& operator=(UString&& other) noexcept;
    UString& operator=(const char* c_str);

    // --- Concatenation Operators ---
    UString& operator+=(const char* c_str);
    UString& operator+=(const UString& other);

    // --- Core API ---
    void Append(const char* c_str);
    void Append(const UString& other);
    size_t Length() const;
    bool IsEmpty() const;
    void Clear();
    const char* CStr() const;

private:
    struct PImpl;
    PImpl* pimpl;
};

// --- Non-Member Operator Overloads for Concatenation ---
// These functions allow for natural string addition like "hello" + myUString;

/**
 * @brief Concatenates two UString objects.
 */
UString operator+(const UString& lhs, const UString& rhs);

/**
 * @brief Concatenates a UString with a C-style string.
 */
UString operator+(const UString& lhs, const char* rhs);

/**
 * @brief Concatenates a C-style string with a UString.
 */
UString operator+(const char* lhs, const UString& rhs);


} // namespace um3d
