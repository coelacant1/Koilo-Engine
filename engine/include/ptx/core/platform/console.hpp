#pragma once

#include <cstdint> // For std::uint32_t

#include "../../registry/reflect_macros.hpp"

/**
 * @file Console.hpp
 * @brief Provides a platform-agnostic API for console/serial output.
 * @date 29/06/2025
 * @author Coela Can't
 */

/**
 * @namespace ptx::Console
 * @brief Unifies console output between Arduino (Serial) and native C++ (std::cout).
 */
namespace ptx {
namespace Console {

    /**
     * @brief Initializes the console or serial port.
     * @param baud The serial baud rate (ignored on non-Arduino platforms).
     */
    void Begin(uint32_t baud = 9600);

    /**
     * @brief Prints a null-terminated string without a new line.
     */
    void Print(const char* msg);

    /**
     * @brief Prints an integer value without a new line.
     */
    void Print(int value);

    /**
     * @brief Prints a float value with a specific precision, without a new line.
     * @param value The floating-point number to print.
     * @param precision The number of digits to show after the decimal point.
     */
    void Print(float value, int precision = 2);

    /**
     * @brief Prints a new line character to the console.
     */
    void Println();
    
    /**
     * @brief Prints a null-terminated string followed by a new line.
     */
    void Println(const char* msg);

    /**
     * @brief Prints an integer value followed by a new line.
     */
    void Println(int value);

    /**
     * @brief Prints a float value with a specific precision, followed by a new line.
     * @param value The floating-point number to print.
     * @param precision The number of digits to show after the decimal point.
     */
    void Println(float value, int precision = 2);

    struct Reflection {
        static void Begin(uint32_t baud = 9600) { ::ptx::Console::Begin(baud); }

    static void Print(const char* msg) { ::ptx::Console::Print(msg); }
    static void Print(int value) { ::ptx::Console::Print(value); }
    static void Print(float value, int precision = 2) { ::ptx::Console::Print(value, precision); }

    static void Println() { ::ptx::Console::Println(); }
    static void Println(const char* msg) { ::ptx::Console::Println(msg); }
    static void Println(int value) { ::ptx::Console::Println(value); }
    static void Println(float value, int precision = 2) { ::ptx::Console::Println(value, precision); }

        PTX_BEGIN_FIELDS(Reflection)
            /* No reflected fields. */
        PTX_END_FIELDS

        PTX_BEGIN_METHODS(Reflection)
            PTX_SMETHOD_OVLD(Reflection, Begin, void, uint32_t),
            PTX_SMETHOD_OVLD(Reflection, Print, void, const char *),
            PTX_SMETHOD_OVLD(Reflection, Print, void, int),
            PTX_SMETHOD_OVLD(Reflection, Print, void, float, int),
            PTX_SMETHOD_OVLD0(Reflection, Println, void),
            PTX_SMETHOD_OVLD(Reflection, Println, void, const char *),
            PTX_SMETHOD_OVLD(Reflection, Println, void, int),
            PTX_SMETHOD_OVLD(Reflection, Println, void, float, int)
        PTX_END_METHODS

        PTX_BEGIN_DESCRIBE(Reflection)
        PTX_END_DESCRIBE(Reflection)
    };
} // namespace Console
} // namespace ptx
