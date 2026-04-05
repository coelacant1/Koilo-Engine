// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file module_fault.hpp
 * @brief Per-module fault tracking and isolation policy.
 *
 * Tracks consecutive faults per module. When a module throws during
 * Update/Render/Tick, the kernel catches the exception, records the fault,
 * and either attempts a restart or marks the module as permanently failed.
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

#include <cstdint>
#include <string>

namespace koilo {

/// Policy constants for module fault isolation.
struct ModuleFaultPolicy {
    static constexpr uint32_t kMaxConsecutiveFaults = 3;
    static constexpr float    kRestartCooldownSec   = 2.0f;
};

/// Per-module fault record. Stored alongside module entries.
struct ModuleFaultRecord {
    uint32_t    consecutiveFaults = 0;
    uint32_t    totalFaults       = 0;
    float       faultCooldown     = 0.0f;   ///< Seconds remaining before restart.
    std::string lastError;                   ///< what() from the last caught exception.

    void RecordFault(const char* error) {
        ++consecutiveFaults;
        ++totalFaults;
        lastError = error ? error : "unknown";
        faultCooldown = ModuleFaultPolicy::kRestartCooldownSec;
    }

    void ClearFault() {
        consecutiveFaults = 0;
        faultCooldown = 0.0f;
    }

    bool ShouldRestart() const {
        return consecutiveFaults > 0 &&
               consecutiveFaults < ModuleFaultPolicy::kMaxConsecutiveFaults;
    }

    bool IsPermanentlyFailed() const {
        return consecutiveFaults >= ModuleFaultPolicy::kMaxConsecutiveFaults;
    }

    void TickCooldown(float dt) {
        if (faultCooldown > 0.0f) faultCooldown -= dt;
    }

    bool CooldownExpired() const {
        return faultCooldown <= 0.0f;
    }
};

} // namespace koilo
