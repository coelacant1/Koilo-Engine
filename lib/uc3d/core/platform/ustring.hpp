// ustring.hpp
#pragma once

#include <cstdint>

namespace uc3d {

/**
 * @file ustring.hpp
 * @brief Platform-agnostic string wrapper with small, stable API.
 * @date 8/18/2025
 *
 * UString provides a thin, portable string abstraction that maps to Arduino's
 * String on embedded targets and std::string on native builds. Storage and
 * implementation live behind a PImpl to keep ABI stable and headers minimal.
 */

/**
 * @class UString
 * @brief A platform-agnostic string class for maximum portability.
 *
 * - On Arduino, wraps `String`.
 * - On native C++ platforms, wraps `std::string`.
 * - Small API: construction, assignment, concatenation, append, length/empty, clear, CStr().
 * - Use @ref FromFloat to format floating-point values without exposing iostreams.
 */
class UString {
public:
    // --- Constructors & Destructor ---

    /**
     * @brief Default-construct empty string.
     */
    UString();

    /**
     * @brief Construct from C string (null-safe).
     * @param c_str Null-terminated string; if null, results in empty string.
     */
    UString(const char* c_str);

    /**
     * @brief Copy-construct from another UString.
     * @param other Source instance.
     */
    UString(const UString& other);

    /**
     * @brief Move-construct, taking ownership of internal storage.
     * @param other Rvalue source; left in a valid but unspecified (empty) state.
     */
    UString(UString&& other) noexcept;

    /**
     * @brief Destroy and free internal storage.
     */
    ~UString();

    // --- Factory ---

    /**
     * @brief Create a UString from a float with fixed precision.
     * @param value Floating-point value to format.
     * @param precision Digits after decimal (default 2).
     * @return Newly constructed UString with formatted text.
     */
    static UString FromFloat(float value, int precision = 2);

    // --- Assignment Operators ---

    /**
     * @brief Copy-assign from another UString.
     */
    UString& operator=(const UString& other);

    /**
     * @brief Move-assign from another UString.
     */
    UString& operator=(UString&& other) noexcept;

    /**
     * @brief Assign from a C string (null-safe).
     */
    UString& operator=(const char* c_str);

    // --- Concatenation Operators ---

    /**
     * @brief Append a C string to this string.
     */
    UString& operator+=(const char* c_str);

    /**
     * @brief Append another UString to this string.
     */
    UString& operator+=(const UString& other);

    // --- Core API ---

    /**
     * @brief Append a C string (null is ignored).
     */
    void Append(const char* c_str);

    /**
     * @brief Append another UString.
     */
    void Append(const UString& other);

    /**
     * @brief Get string length in characters.
     */
    uint8_t Length() const;

    /**
     * @brief Check whether the string is empty.
     */
    bool IsEmpty() const;

    /**
     * @brief Clear contents to empty.
     */
    void Clear();

    /**
     * @brief Get a null-terminated C string view.
     * @return Pointer valid until next mutation of this UString.
     */
    const char* CStr() const;

private:
    struct PImpl; ///< Opaque implementation (Arduino String or std::string).
    PImpl* pimpl; ///< Owning pointer to implementation.
};

// --- Non-member concatenation helpers ---

/**
 * @brief Concatenate two UStrings (lhs + rhs).
 */
UString operator+(const UString& lhs, const UString& rhs);

/**
 * @brief Concatenate UString and C string (lhs + rhs).
 */
UString operator+(const UString& lhs, const char* rhs);

/**
 * @brief Concatenate C string and UString (lhs + rhs).
 */
UString operator+(const char* lhs, const UString& rhs);

} // namespace uc3d
