#pragma once

#include <cstdint> // For std::uint32_t

/**
 * @file Console.hpp
 * @brief Provides a platform-agnostic API for console/serial output.
 * @date 29/06/2025
 * @author Coela Can't
 */

/**
 * @namespace um3d::Console
 * @brief Unifies console output between Arduino (Serial) and native C++ (std::cout).
 */
namespace um3d::Console {

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

} // namespace um3d::Console
