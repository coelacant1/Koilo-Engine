cmake_minimum_required(VERSION 3.21)
project(KoiloEngine LANGUAGES C CXX)

include(GNUInstallDirs)

# Parallel build optimizations
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clangd/IDE support
set_property(GLOBAL PROPERTY JOB_POOLS compile_pool=20) # Limit parallel compile jobs
set(CMAKE_JOB_POOL_COMPILE compile_pool)

# Use ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  message(STATUS "ccache found: ${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# Options
option(KL_BUILD_TESTS "Build C++ tests" ON)
option(KL_BUILD_PYTHON "Enable Python code generation helpers" ON)
option(KL_ENABLE_WARNINGS "Enable project warning flags" ON)
option(KL_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(KL_USE_SYSTEM_UNITY "Use an existing Unity test framework checkout" OFF)
option(KOILO_ENABLE_AI "Build AI subsystem (pathfinding, behavior trees)" ON)
option(KOILO_ENABLE_AUDIO "Build audio subsystem (SDL3 audio)" ON)
option(KOILO_ENABLE_PARTICLES "Build particle subsystem" ON)
option(KOILO_USE_VULKAN "Build Vulkan render backend and compile SPIR-V shaders" ON)
option(KOILO_USE_OPENGL "Build OpenGL render backend" ON)
set(KL_UNITY_DIR ${CMAKE_SOURCE_DIR}/external/unity CACHE PATH "Path to Unity test framework")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(default_build_type "RelWithDebInfo")
  message(STATUS "Setting build type to '${default_build_type}' as none was specified")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
endif()

set(KL_WARNING_FLAGS)
if(KL_ENABLE_WARNINGS)
  list(APPEND KL_WARNING_FLAGS -Wall -Wextra)
  # Suppress common warnings that don't indicate bugs
  list(APPEND KL_WARNING_FLAGS -Wno-unused-parameter)
  if(KL_WARNINGS_AS_ERRORS)
    list(APPEND KL_WARNING_FLAGS -Werror)
  endif()
endif()

# Directories
set(KL_SOURCE_ROOT ${CMAKE_SOURCE_DIR})
set(KL_SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/scripts)
set(KL_GEN_DIR ${CMAKE_BINARY_DIR}/generated)

# Python (for generation)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Generation custom commands / targets
add_custom_command(
  OUTPUT ${KL_GEN_DIR}/koilo/koiloall.hpp
  COMMAND ${CMAKE_COMMAND} -E make_directory ${KL_GEN_DIR}/koilo
  COMMAND Python3::Interpreter ${KL_SCRIPTS_DIR}/generatekoiloall.py --root ${CMAKE_SOURCE_DIR}/engine/koilo --output ${KL_GEN_DIR}/koilo/koiloall.hpp
  DEPENDS ${KL_SCRIPTS_DIR}/generatekoiloall.py ${KL_SCRIPTS_DIR}/consoleoutput.py
  WORKING_DIRECTORY ${KL_SCRIPTS_DIR}
  COMMENT "Generating reflection umbrella header (koiloall.hpp)"
  VERBATIM
)

# Test skeleton generation (runs after reflection generation)
add_custom_command(
  OUTPUT ${KL_GEN_DIR}/.test_skeleton_cache.json
  COMMAND Python3::Interpreter ${KL_SCRIPTS_DIR}/generatetestskeletons.py
          --all
          --cache-file ${KL_GEN_DIR}/.test_skeleton_cache.json
  DEPENDS
    ${KL_SCRIPTS_DIR}/generatetestskeletons.py
    ${KL_SCRIPTS_DIR}/consoleoutput.py
    ${KL_GEN_DIR}/reflection_entry_gen.cpp
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Generating test skeletons for untested classes"
  VERBATIM
)

add_custom_target(koilo_update_registry ALL
  DEPENDS ${KL_GEN_DIR}/koilo/koiloall.hpp
)

add_custom_command(
  OUTPUT ${KL_GEN_DIR}/reflection_entry_gen.cpp
         ${KL_GEN_DIR}/reflection_core_gen.cpp
         ${KL_GEN_DIR}/reflection_platform_gen.cpp
  COMMAND ${CMAKE_COMMAND} -E make_directory ${KL_GEN_DIR}
  COMMAND Python3::Interpreter ${KL_SCRIPTS_DIR}/generatereflectionentry.py --root ${CMAKE_SOURCE_DIR}/engine/koilo --output ${KL_GEN_DIR}/reflection_entry_gen.cpp
  DEPENDS ${KL_SCRIPTS_DIR}/generatereflectionentry.py ${KL_SCRIPTS_DIR}/consoleoutput.py ${KL_GEN_DIR}/koilo/koiloall.hpp
  WORKING_DIRECTORY ${KL_SCRIPTS_DIR}
  COMMENT "Generating reflection registry translation units (core + platform)"
  VERBATIM
)

add_custom_target(koilo_generate ALL DEPENDS ${KL_GEN_DIR}/reflection_entry_gen.cpp)
add_dependencies(koilo_generate koilo_update_registry)

# Test skeleton generation target
add_custom_target(koilo_generate_test_skeletons
  DEPENDS ${KL_GEN_DIR}/.test_skeleton_cache.json
)
add_dependencies(koilo_generate_test_skeletons koilo_generate)

add_library(koilo_headers INTERFACE)
target_include_directories(koilo_headers INTERFACE
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/engine>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/engine/koilo>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external/glad/include>
  $<BUILD_INTERFACE:${KL_GEN_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Optional subsystem compile definitions (project-wide via koilo_headers)
if(KOILO_ENABLE_AI)
  target_compile_definitions(koilo_headers INTERFACE KOILO_ENABLE_AI)
endif()
if(KOILO_ENABLE_AUDIO)
  target_compile_definitions(koilo_headers INTERFACE KOILO_ENABLE_AUDIO)
endif()
if(KOILO_ENABLE_PARTICLES)
  target_compile_definitions(koilo_headers INTERFACE KOILO_ENABLE_PARTICLES)
endif()

# -- koilo_foundation ----------------------------------------------
# Pure math, geometry, color, time, reflection, shader types, interfaces.
# No dependencies on kernel, scripting, or systems.
file(GLOB_RECURSE KL_FOUNDATION_SOURCES
  CONFIGURE_DEPENDS
  engine/koilo/core/*.cpp
)

add_library(koilo_foundation STATIC ${KL_FOUNDATION_SOURCES})
add_dependencies(koilo_foundation koilo_generate)
target_link_libraries(koilo_foundation PUBLIC koilo_headers)
target_compile_features(koilo_foundation PUBLIC cxx_std_17)

if(KL_WARNING_FLAGS)
  target_compile_options(koilo_foundation PRIVATE ${KL_WARNING_FLAGS})
endif()

# -- Sub-targets (OBJECT libraries for compilation isolation) -----
# Each compiles independently for incremental build benefit.
# All objects are collected into the final koilo_engine archive.

# koilo_kernel: microkernel (module lifecycle, memory, console, services)
file(GLOB_RECURSE KL_KERNEL_SOURCES CONFIGURE_DEPENDS engine/koilo/kernel/*.cpp)
add_library(koilo_kernel OBJECT ${KL_KERNEL_SOURCES})
add_dependencies(koilo_kernel koilo_generate)
target_link_libraries(koilo_kernel PUBLIC koilo_foundation)
target_compile_features(koilo_kernel PUBLIC cxx_std_17)
if(KL_WARNING_FLAGS)
  target_compile_options(koilo_kernel PRIVATE ${KL_WARNING_FLAGS})
endif()

# koilo_assets: asset loaders (mesh, image, font, model formats)
file(GLOB_RECURSE KL_ASSETS_SOURCES CONFIGURE_DEPENDS engine/koilo/assets/*.cpp)
add_library(koilo_assets OBJECT ${KL_ASSETS_SOURCES})
add_dependencies(koilo_assets koilo_generate)
target_link_libraries(koilo_assets PUBLIC koilo_foundation)
target_compile_features(koilo_assets PUBLIC cxx_std_17)
if(KL_WARNING_FLAGS)
  target_compile_options(koilo_assets PRIVATE ${KL_WARNING_FLAGS})
endif()

# koilo_debug: debug draw, debug renderer, profiler
file(GLOB_RECURSE KL_DEBUG_SOURCES CONFIGURE_DEPENDS engine/koilo/debug/*.cpp)
add_library(koilo_debug OBJECT ${KL_DEBUG_SOURCES})
add_dependencies(koilo_debug koilo_generate)
target_link_libraries(koilo_debug PUBLIC koilo_foundation)
target_compile_features(koilo_debug PUBLIC cxx_std_17)
if(KL_WARNING_FLAGS)
  target_compile_options(koilo_debug PRIVATE ${KL_WARNING_FLAGS})
endif()

# koilo_scripting: KoiloScript language (lexer, parser, AST, bytecode VM, engine)
file(GLOB_RECURSE KL_SCRIPTING_SOURCES CONFIGURE_DEPENDS engine/koilo/scripting/*.cpp)
add_library(koilo_scripting OBJECT ${KL_SCRIPTING_SOURCES})
add_dependencies(koilo_scripting koilo_generate)
target_link_libraries(koilo_scripting PUBLIC koilo_kernel koilo_foundation)
target_compile_features(koilo_scripting PUBLIC cxx_std_17)
if(KL_WARNING_FLAGS)
  target_compile_options(koilo_scripting PRIVATE ${KL_WARNING_FLAGS})
endif()

# koilo_systems: all runtime systems (render, scene, physics, UI, input, etc.)
file(GLOB_RECURSE KL_SYSTEMS_SOURCES
  CONFIGURE_DEPENDS
  engine/koilo/systems/*.cpp
  engine/koilo/ecs/*.cpp
)
list(FILTER KL_SYSTEMS_SOURCES EXCLUDE REGEX ".*/systems/display/.*")
list(FILTER KL_SYSTEMS_SOURCES EXCLUDE REGEX ".*/systems/render/gl/.*")
list(FILTER KL_SYSTEMS_SOURCES EXCLUDE REGEX ".*/systems/render/vk/.*")
list(FILTER KL_SYSTEMS_SOURCES EXCLUDE REGEX ".*/systems/render/rhi/vulkan/.*")
list(FILTER KL_SYSTEMS_SOURCES EXCLUDE REGEX ".*/systems/render/rhi/opengl/.*")
list(FILTER KL_SYSTEMS_SOURCES EXCLUDE REGEX ".*/systems/ui/render/ui_gl_renderer\\.cpp")
list(FILTER KL_SYSTEMS_SOURCES EXCLUDE REGEX ".*/systems/ui/render/ui_vk_renderer\\.cpp")
if(NOT KOILO_ENABLE_AI)
  list(FILTER KL_SYSTEMS_SOURCES EXCLUDE REGEX ".*/systems/ai/.*")
endif()
if(NOT KOILO_ENABLE_AUDIO)
  list(FILTER KL_SYSTEMS_SOURCES EXCLUDE REGEX ".*/systems/audio/.*")
endif()
if(NOT KOILO_ENABLE_PARTICLES)
  list(FILTER KL_SYSTEMS_SOURCES EXCLUDE REGEX ".*/systems/particles/.*")
endif()

add_library(koilo_systems OBJECT ${KL_SYSTEMS_SOURCES})
add_dependencies(koilo_systems koilo_generate)
target_link_libraries(koilo_systems PUBLIC
  koilo_scripting koilo_kernel koilo_debug koilo_assets koilo_foundation
)
target_compile_features(koilo_systems PUBLIC cxx_std_17)

# Audio backend (SDL3 for audio subsystem)
find_package(SDL3 QUIET)
if(SDL3_FOUND)
  message(STATUS "Audio backend: SDL3")
  target_link_libraries(koilo_systems PRIVATE SDL3::SDL3)
  target_compile_definitions(koilo_systems PRIVATE KL_AUDIO_SDL3)
else()
  message(STATUS "Audio backend: disabled (SDL3 not found)")
  target_compile_definitions(koilo_systems PRIVATE KL_AUDIO_NONE)
endif()

if(KL_WARNING_FLAGS)
  target_compile_options(koilo_systems PRIVATE ${KL_WARNING_FLAGS})
endif()

# koilo_platform: platform abstraction (file readers, thread affinity)
file(GLOB_RECURSE KL_PLATFORM_SOURCES CONFIGURE_DEPENDS engine/koilo/platform/*.cpp)
add_library(koilo_platform OBJECT ${KL_PLATFORM_SOURCES})
add_dependencies(koilo_platform koilo_generate)
target_link_libraries(koilo_platform PUBLIC koilo_foundation)
target_compile_features(koilo_platform PUBLIC cxx_std_17)
if(KL_WARNING_FLAGS)
  target_compile_options(koilo_platform PRIVATE ${KL_WARNING_FLAGS})
endif()

# koilo_app: application framework (KoiloApplication)
file(GLOB_RECURSE KL_APP_SOURCES CONFIGURE_DEPENDS engine/koilo/app/*.cpp)
add_library(koilo_app OBJECT ${KL_APP_SOURCES})
add_dependencies(koilo_app koilo_generate)
target_link_libraries(koilo_app PUBLIC koilo_kernel koilo_foundation)
target_compile_features(koilo_app PUBLIC cxx_std_17)
if(KL_WARNING_FLAGS)
  target_compile_options(koilo_app PRIVATE ${KL_WARNING_FLAGS})
endif()

# Propagate Vulkan backend define to object libraries that need it
if(KOILO_USE_VULKAN)
  find_package(Vulkan QUIET)
  if(Vulkan_FOUND)
    target_compile_definitions(koilo_scripting PRIVATE KL_HAVE_VULKAN_BACKEND)
    target_compile_definitions(koilo_systems PRIVATE KL_HAVE_VULKAN_BACKEND)
    target_include_directories(koilo_scripting PRIVATE ${Vulkan_INCLUDE_DIRS})
    target_include_directories(koilo_systems PRIVATE ${Vulkan_INCLUDE_DIRS})
    set(KL_VULKAN_FOUND TRUE)
  endif()
endif()

# -- koilo_engine (aggregate archive) ----------------------------
# Collects all OBJECT library outputs + generated reflection code
# into a single static library. Avoids circular link ordering issues.
add_library(koilo_engine STATIC
  ${KL_GEN_DIR}/reflection_core_gen.cpp
  $<TARGET_OBJECTS:koilo_kernel>
  $<TARGET_OBJECTS:koilo_assets>
  $<TARGET_OBJECTS:koilo_debug>
  $<TARGET_OBJECTS:koilo_scripting>
  $<TARGET_OBJECTS:koilo_systems>
  $<TARGET_OBJECTS:koilo_platform>
  $<TARGET_OBJECTS:koilo_app>
)
add_dependencies(koilo_engine koilo_generate)
target_link_libraries(koilo_engine PUBLIC koilo_foundation)
target_compile_features(koilo_engine PUBLIC cxx_std_17)

if(KL_WARNING_FLAGS)
  target_compile_options(koilo_engine PRIVATE ${KL_WARNING_FLAGS})
endif()

if(KL_VULKAN_FOUND)
  target_compile_definitions(koilo_engine PRIVATE KL_HAVE_VULKAN_BACKEND)
  target_include_directories(koilo_engine PRIVATE ${Vulkan_INCLUDE_DIRS})
endif()

# Backward-compat alias: koilo_core -> koilo_engine
add_library(koilo_core ALIAS koilo_engine)

add_subdirectory(engine/koilo/systems/display)

# -- Generic koilo runner ------------------------------------------
# Single binary: koilo --script examples/space_shooter/space_shooter.ks --software --vsync
if(SDL3_FOUND OR TARGET SDL3::SDL3)
  add_executable(koilo engine/koilo/app/koilo_runner.cpp)
  target_link_libraries(koilo PRIVATE koilo_core koiloengine_display SDL3::SDL3)
  target_include_directories(koilo PRIVATE
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/engine/koilo
    ${CMAKE_SOURCE_DIR}/engine/koilo/modules
    ${CMAKE_BINARY_DIR}/generated
  )
endif()

# Examples
option(KL_BUILD_EXAMPLES "Build example programs" ON)
if(KL_BUILD_EXAMPLES AND EXISTS "${CMAKE_SOURCE_DIR}/examples/CMakeLists.txt")
  add_subdirectory(examples)
endif()

# Tests
if(KL_BUILD_TESTS)
  enable_testing()
  file(GLOB_RECURSE KL_TEST_SOURCES CONFIGURE_DEPENDS
    tests/engine/*.cpp
    tests/engine/**/*.cpp
  )
  set(KL_UNITY_SOURCES)
  set(KL_UNITY_INCLUDE)

  if(KL_USE_SYSTEM_UNITY)
    if(NOT EXISTS ${KL_UNITY_DIR}/unity.c)
      message(FATAL_ERROR "KL_USE_SYSTEM_UNITY=ON but no unity.c found at ${KL_UNITY_DIR}. Set KL_UNITY_DIR to a valid Unity checkout.")
    endif()
    message(STATUS "Using Unity test framework from system path ${KL_UNITY_DIR}")
    set(KL_UNITY_SOURCES ${KL_UNITY_DIR}/unity.c)
    set(KL_UNITY_INCLUDE ${KL_UNITY_DIR})
  elseif(EXISTS ${KL_UNITY_DIR}/unity.c)
    message(STATUS "Using vendored Unity test framework at ${KL_UNITY_DIR}")
    set(KL_UNITY_SOURCES ${KL_UNITY_DIR}/unity.c)
    set(KL_UNITY_INCLUDE ${KL_UNITY_DIR})
  else()
    include(FetchContent)
    FetchContent_Declare(
      unity
      GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
      GIT_TAG v2.6.0 # update as needed
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(unity)
    set(KL_UNITY_SOURCES ${unity_SOURCE_DIR}/src/unity.c)
    set(KL_UNITY_INCLUDE ${unity_SOURCE_DIR}/src)
  endif()

  list(FILTER KL_TEST_SOURCES EXCLUDE REGEX ".*/language_tests/.*")

  # Exclude tests for disabled subsystems
  if(NOT KOILO_ENABLE_AI)
    list(FILTER KL_TEST_SOURCES EXCLUDE REGEX ".*/systems/ai/.*")
  endif()
  if(NOT KOILO_ENABLE_AUDIO)
    list(FILTER KL_TEST_SOURCES EXCLUDE REGEX ".*/systems/audio/.*")
  endif()
  if(NOT KOILO_ENABLE_PARTICLES)
    list(FILTER KL_TEST_SOURCES EXCLUDE REGEX ".*/systems/particles/.*")
  endif()

  add_executable(koilo_tests ${KL_TEST_SOURCES} ${KL_UNITY_SOURCES})
  add_dependencies(koilo_tests koilo_generate_test_skeletons)
  target_link_libraries(koilo_tests PRIVATE koilo_core koiloengine_display)
  target_include_directories(koilo_tests PRIVATE
    ${KL_UNITY_INCLUDE}
    ${CMAKE_SOURCE_DIR}/tests/engine
  )
  target_compile_features(koilo_tests PRIVATE cxx_std_17)
  if(KL_WARNING_FLAGS)
    target_compile_options(koilo_tests PRIVATE ${KL_WARNING_FLAGS})
  endif()
  add_test(NAME koilo_core_tests COMMAND koilo_tests)

  # KoiloScript language integration tests (custom TEST/PASS/FAIL runner, not Unity)
  file(GLOB_RECURSE KL_LANG_TEST_SOURCES CONFIGURE_DEPENDS
    tests/engine/scripting/language_tests/*.cpp
  )
  add_executable(koilo_script_language_tests ${KL_LANG_TEST_SOURCES})
  add_dependencies(koilo_script_language_tests koilo_generate)
  target_link_libraries(koilo_script_language_tests PRIVATE koilo_core koiloengine_display)
  target_include_directories(koilo_script_language_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/engine/scripting/language_tests
    ${CMAKE_SOURCE_DIR}/tests/engine
  )
  target_compile_features(koilo_script_language_tests PRIVATE cxx_std_17)
  if(KL_WARNING_FLAGS)
    target_compile_options(koilo_script_language_tests PRIVATE ${KL_WARNING_FLAGS})
  endif()
  add_test(NAME koilo_script_language_tests COMMAND koilo_script_language_tests)
endif()

# Header smoke-test: compile every Koilo header as its own translation unit
file(GLOB_RECURSE KL_HEADERCHECK_HEADERS
  CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/engine/koilo/*.hpp
)

set(KL_HEADERCHECK_STUBS)
foreach(header IN LISTS KL_HEADERCHECK_HEADERS)
  # Filter out template and virtual headers (same as GenerateKoiloAll.py)
  file(READ ${header} header_content)
  string(REGEX MATCH "template[ \t\n]*<" has_template "${header_content}")
  string(REGEX MATCH "virtual" has_virtual "${header_content}")
  if(has_template OR has_virtual)
    continue()
  endif()

  file(RELATIVE_PATH rel_path ${CMAKE_SOURCE_DIR}/engine ${header})
  set(stub "${CMAKE_BINARY_DIR}/headercheck/${rel_path}.cpp")
  cmake_path(GET stub PARENT_PATH stub_dir)
  file(MAKE_DIRECTORY ${stub_dir})
  file(WRITE ${stub} "#include \"${rel_path}\"\n")
  list(APPEND KL_HEADERCHECK_STUBS ${stub})
endforeach()

if(KL_HEADERCHECK_STUBS)
  add_library(koilo_headercheck_objs OBJECT EXCLUDE_FROM_ALL ${KL_HEADERCHECK_STUBS})
  add_dependencies(koilo_headercheck_objs koilo_generate)
  target_link_libraries(koilo_headercheck_objs PRIVATE koilo_headers)
  target_compile_features(koilo_headercheck_objs PRIVATE cxx_std_17)
  target_compile_options(koilo_headercheck_objs PRIVATE -Wno-pragmas)
  add_custom_target(koilo_headercheck DEPENDS koilo_headercheck_objs)
endif()

if(KL_BUILD_PYTHON)
  message(STATUS "Koilo Python helper targets enabled (no native build steps)")
endif()

message(STATUS "KoiloEngine configuration complete")
