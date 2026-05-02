// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file strict_fp.hpp
 * @brief Tier-2 (bit-exact) determinism support for the physics
 *        module.
 *
 * What this header provides
 * -------------------------
 *  - The `KOILO_PHYSICS_T2_STRICT` macro, defined to 1 by the build system
 *    when CMake option `KL_PHYSICS_T2_STRICT=ON` is selected. Headers may
 *    branch on this for any extra strict-build guards.
 *  - `koilo::ScopedFpEnv` - RAII helper that pins the calling
 *    thread's floating-point environment for the duration of its scope:
 *      * round-to-nearest-even,
 *      * denormals NOT flushed (FTZ/DAZ off on x86; FZ off on aarch64),
 *        so subnormal arithmetic is IEEE-754 conformant on every supported
 *        arch.
 *    Constructed at the top of `PhysicsWorld::Step()` to harden the
 *    physics step against host-application FP-mode pollution. Restores
 *    prior state on destruction.
 *
 * What this header does NOT provide
 * ---------------------------------
 *  - Cross-libc deterministic transcendentals. `std::sin`/`cos`/`atan2`/
 *    `pow`/`exp` are libc-implementation-defined; two machines using
 *    different libms can produce different bits even with FP-contract
 *    off and the FP environment pinned. Cross-libc bit-exactness requires
 *    bundling a vetted libm replacement (SLEEF, crlibm) - out of scope
 * for
 *  - Wall-clock independence. PhysicsWorld::Step() ignores the wall-clock
 *    substep budget when DeterminismTier::T2_BitExactCrossMachine is set;
 *    the integer substep cap is still honored.
 *
 * Current shipping bar
 * --------------------
 * ships strict build mode + FP env pinning + wall-clock exemption
 * + same-binary T2-build replay tests. Cross-arch (x86_64 ↔ aarch64)
 * bit-exactness is NOT yet validated in CI; documented as deferred
 * groundwork.
 *
 * T2-strict coverage audit
 * ---------------------------------------------------
 * Verified: every transcendental / fp-sensitive call exercised by the
 * fixed-step physics path lives in a translation unit that receives
 * KL_PHYSICS_T2_FLAGS when KL_PHYSICS_T2_STRICT=ON.
 *
 * koilo_foundation (engine/koilo/core, gets T2 flags):
 *   - core/math/vector3d.hpp        - std::sqrt in Magnitude/Distance (inline,
 *                                     inlined into callers' TU; covered when
 *                                     callers are in foundation/systems).
 *   - core/math/matrix4x4.cpp       - std::sqrt/sin/cos/tan in trig matrices.
 *   - core/math/mathematics.cpp     - Sin/Cos/Tan/ATan2 thin wrappers.
 *   - core/math/bezier.cpp          - std::sqrt for discriminant.
 *
 * koilo_systems (engine/koilo/systems, gets T2 flags):
 *   - systems/physics/physicsworld.cpp                 - std::sqrt (radii).
 *   - systems/physics/spherecollider.cpp               - std::sqrt (raycast).
 *   - systems/physics/capsulecollider.cpp              - std::sqrt (raycast).
 *   - systems/physics/meshcollider.cpp                 - std::sqrt (face norm).
 *   - systems/physics/narrowphase/manifoldgenerator.cpp- std::sqrt (×~14, dist
 *                                                       and inv-magnitudes).
 *   - systems/physics/solver/sequentialimpulsesolver.cpp - std::sqrt (friction
 *                                                       geometric mean).
 *   - systems/physics/joints/hingejoint.cpp            - std::atan2 (hinge angle).
 *
 * Excluded from T2 promise (documented at point of use):
 *   - systems/aerodynamics/atmosphere.cpp              - std::pow, std::exp,
 *                                                       std::sqrt (ISA model).
 *   - systems/aerodynamics/aerodynamicsworld.cpp       - std::atan2 (AoA).
 * ships T0/T1 only; cross-machine bit-exactness is explicitly
 *     NOT promised for the aerodynamics module - see atmosphere.hpp /
 *     aerodynamicsworld.hpp module-level comments. Same-binary replay still
 *     works (TestSameBinaryReplayBitExact passes under T2 flags).
 *
 * Conclusion: no transcendental call in the deterministic physics fixed-step
 * path falls outside the strict-fp envelope. No additional sqrt wrappers
 * required (FMA contraction is suppressed by `-ffp-contract=off` on the TU).
 */

#pragma once

// Platform detection for FP env pinning. Includes pulled in BEFORE the
// koilo namespace block so they emit declarations into the global /
// std namespace where they belong.
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#  define KOILO_PHYSICS_FP_ENV_X86 1
#  include <xmmintrin.h>
#  include <pmmintrin.h>
#elif defined(__aarch64__) || defined(_M_ARM64)
#  define KOILO_PHYSICS_FP_ENV_AARCH64 1
#endif

namespace koilo {

/**
 * @class ScopedFpEnv
 * @brief Pins the FP environment for the duration of its lifetime.
 *
 * On x86 (SSE): clears MXCSR FTZ + DAZ bits, sets rounding to
 * round-to-nearest. On aarch64: clears FPCR FZ bit, sets rounding to
 * round-to-nearest. Saves prior state and restores it on destruction.
 * On unsupported platforms it is a no-op (Tier-2 is not promised on
 * those platforms anyway). Header-only / inline.
 */
class ScopedFpEnv {
public:
    ScopedFpEnv() noexcept { Save(); Pin(); }
    ~ScopedFpEnv() noexcept { Restore(); }

    ScopedFpEnv(const ScopedFpEnv&) = delete;
    ScopedFpEnv& operator=(const ScopedFpEnv&) = delete;

private:
    inline void Save() noexcept;
    inline void Pin() noexcept;
    inline void Restore() noexcept;

    unsigned int       saved_x86_mxcsr_     = 0;
    unsigned long long saved_aarch64_fpcr_  = 0;
    bool               saved_               = false;
};

#if defined(KOILO_PHYSICS_FP_ENV_X86)

inline void ScopedFpEnv::Save() noexcept {
    saved_x86_mxcsr_ = _mm_getcsr();
    saved_ = true;
}
inline void ScopedFpEnv::Pin() noexcept {
    // Clear FTZ (bit 15) and DAZ (bit 6); force RC field (bits 13-14) to
    // 00 (round-to-nearest-even). Leave exception masks alone.
    unsigned int csr = saved_x86_mxcsr_;
    csr &= ~((1u << 15) | (1u << 6));   // FTZ off, DAZ off
    csr &= ~(3u << 13);                  // RC = nearest
    _mm_setcsr(csr);
}
inline void ScopedFpEnv::Restore() noexcept {
    if (saved_) _mm_setcsr(saved_x86_mxcsr_);
}

#elif defined(KOILO_PHYSICS_FP_ENV_AARCH64)

inline void ScopedFpEnv::Save() noexcept {
    unsigned long long fpcr;
    __asm__ volatile("mrs %0, fpcr" : "=r"(fpcr));
    saved_aarch64_fpcr_ = fpcr;
    saved_ = true;
}
inline void ScopedFpEnv::Pin() noexcept {
    // Clear FZ (bit 24, flush-to-zero) and force RMode (bits 22-23) to
    // 00 = round-to-nearest. Other bits left alone.
    unsigned long long fpcr = saved_aarch64_fpcr_;
    fpcr &= ~(1ull << 24);
    fpcr &= ~(3ull << 22);
    __asm__ volatile("msr fpcr, %0" : : "r"(fpcr));
}
inline void ScopedFpEnv::Restore() noexcept {
    if (saved_) {
        __asm__ volatile("msr fpcr, %0" : : "r"(saved_aarch64_fpcr_));
    }
}

#else

inline void ScopedFpEnv::Save() noexcept    { saved_ = true; }
inline void ScopedFpEnv::Pin() noexcept     {}
inline void ScopedFpEnv::Restore() noexcept {}

#endif

} // namespace koilo
