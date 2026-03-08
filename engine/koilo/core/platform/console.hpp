// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint> // For std::uint32_t

#include <koilo/registry/reflect_macros.hpp>

/**
 * @file Console.hpp
 * @brief Provides a platform-agnostic API for console/serial output.
 * @date 29/06/2025
 * @author Coela Can't
 */

/**
 * @namespace koilo::Console
 * @brief Unifies console output between Arduino (Serial) and native C++ (std::cout).
 */
namespace koilo {
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
        static void Begin(uint32_t baud = 9600) { ::koilo::Console::Begin(baud); }

    static void Print(const char* msg) { ::koilo::Console::Print(msg); }
    static void Print(int value) { ::koilo::Console::Print(value); }
    static void Print(float value, int precision = 2) { ::koilo::Console::Print(value, precision); }

    static void Println() { ::koilo::Console::Println(); }
    static void Println(const char* msg) { ::koilo::Console::Println(msg); }
    static void Println(int value) { ::koilo::Console::Println(value); }
    static void Println(float value, int precision = 2) { ::koilo::Console::Println(value, precision); }

        KL_BEGIN_FIELDS(Reflection)
            /* No reflected fields. */
        KL_END_FIELDS

        KL_BEGIN_METHODS(Reflection)
            KL_SMETHOD_OVLD(Reflection, Begin, void, uint32_t),
            KL_SMETHOD_OVLD(Reflection, Print, void, const char *),
            KL_SMETHOD_OVLD(Reflection, Print, void, int),
            KL_SMETHOD_OVLD(Reflection, Print, void, float, int),
            KL_SMETHOD_OVLD0(Reflection, Println, void),
            KL_SMETHOD_OVLD(Reflection, Println, void, const char *),
            KL_SMETHOD_OVLD(Reflection, Println, void, int),
            KL_SMETHOD_OVLD(Reflection, Println, void, float, int)
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(Reflection)
        KL_END_DESCRIBE(Reflection)
    };
} // namespace Console
} // namespace koilo
