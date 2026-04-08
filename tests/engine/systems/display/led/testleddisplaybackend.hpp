// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testleddisplaybackend.hpp
 * @brief Test declarations for the LED display backend.
 *
 * Tests are compiled unconditionally but guarded at runtime/compile
 * depending on KL_HAVE_LED_VOLUME. When the feature is disabled,
 * the test class still exists but RunAllTests is a no-op.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#pragma once

class TestLEDDisplayBackend {
public:
    static void RunAllTests();
};
