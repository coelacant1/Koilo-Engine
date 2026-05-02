# CMake toolchain file for cross-compiling to Raspberry Pi (aarch64)
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-pi-aarch64.cmake ..
#
# Strategy: Do NOT set CMAKE_SYSROOT. The Arch cross-compiler (GCC 15)
# ships its own aarch64 glibc + C++ stdlib under /usr/aarch64-linux-gnu/.
# Setting --sysroot to a Debian Pi sysroot causes __pthread_cond_s struct
# mismatches between GCC 15's C++ headers and Debian's glibc headers.
#
# Instead rely on the cross-compiler's own libc for linking, and add
# -isystem paths so the compiler finds Pi-specific headers (SDL3, libdrm).
# CMAKE_FIND_ROOT_PATH lets find_package() discover Pi libs in the sysroot.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Cross compiler (Arch: aarch64-linux-gnu-gcc / g++)
set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Pi sysroot (rsync'd from device) -- used for find_package() only
set(PI_SYSROOT "$ENV{HOME}/pi-sysroot")
set(CMAKE_FIND_ROOT_PATH ${PI_SYSROOT})

# find_package() searches sysroot for libs/includes/packages, host for programs
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Debian multiarch paths inside the Pi sysroot
set(PI_LIB     "${PI_SYSROOT}/lib/aarch64-linux-gnu")
set(PI_USR_LIB "${PI_SYSROOT}/usr/lib/aarch64-linux-gnu")
set(PI_USR_INC "${PI_SYSROOT}/usr/include")
set(PI_MA_INC  "${PI_SYSROOT}/usr/include/aarch64-linux-gnu")

# RPi-specific compile flags (CM4 = Cortex-A72)
# -isystem adds Pi sysroot headers AFTER the cross-compiler's own stdlib
# so SDL3/libdrm/etc. are found without conflicting with GCC's glibc.
set(CMAKE_C_FLAGS_INIT   "-march=armv8-a+crc -mtune=cortex-a72 -isystem ${PI_USR_INC} -isystem ${PI_MA_INC}")
set(CMAKE_CXX_FLAGS_INIT "-march=armv8-a+crc -mtune=cortex-a72 -isystem ${PI_USR_INC} -isystem ${PI_MA_INC}")

# Linker: rpath-link helps resolve transitive .so dependencies in the sysroot.
# --allow-shlib-undefined: standard for cross-compilation -- shared library
# transitive deps (e.g. SDL3 -> libpulse -> libpulsecommon) resolve on the
# target at runtime, not at cross-link time.
# Do NOT add -L here -- it would cause the linker to prefer the sysroot's
# libc.so over the cross-compiler's own. CMake's find_package() adds -L
# paths automatically for libraries it discovers (SDL3, etc.).
set(CMAKE_EXE_LINKER_FLAGS_INIT    "-Wl,-rpath-link,${PI_LIB}:${PI_USR_LIB} -Wl,--allow-shlib-undefined")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-Wl,-rpath-link,${PI_LIB}:${PI_USR_LIB} -Wl,--allow-shlib-undefined")

# Help find_package() locate cmake configs and libraries in Debian multiarch tree
list(APPEND CMAKE_PREFIX_PATH "${PI_USR_LIB}/cmake")
list(APPEND CMAKE_SYSTEM_LIBRARY_PATH "${PI_USR_LIB}" "${PI_LIB}")

# Disable ccache for cross builds (host ccache may not match)
set(CMAKE_C_COMPILER_LAUNCHER "")
set(CMAKE_CXX_COMPILER_LAUNCHER "")
