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
  BYPRODUCTS ${KL_GEN_DIR}/.koilo_reflect_cache.json
  COMMAND ${CMAKE_COMMAND} -E make_directory ${KL_GEN_DIR}/koilo
  COMMAND Python3::Interpreter ${KL_SCRIPTS_DIR}/generatekoiloall.py --root ${CMAKE_SOURCE_DIR}/engine/koilo --output ${KL_GEN_DIR}/koilo/koiloall.hpp
  COMMAND Python3::Interpreter ${KL_SCRIPTS_DIR}/updatekoiloregistry.py --root ${CMAKE_SOURCE_DIR}/engine/koilo --write --cache-file ${KL_GEN_DIR}/.koilo_reflect_cache.json
  DEPENDS ${KL_SCRIPTS_DIR}/generatekoiloall.py ${KL_SCRIPTS_DIR}/updatekoiloregistry.py ${KL_SCRIPTS_DIR}/consoleoutput.py
  WORKING_DIRECTORY ${KL_SCRIPTS_DIR}
  COMMENT "Generating reflection umbrella header and Koilo registry cache"
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

# Gather core sources
file(GLOB_RECURSE KL_CORE_SOURCES
  CONFIGURE_DEPENDS
  engine/koilo/*.cpp
  engine/koilo/core/**/*.cpp
  engine/koilo/assets/**/*.cpp
  engine/koilo/registry/**/*.cpp
  engine/koilo/scripting/**/*.cpp
  engine/koilo/platform/**/*.cpp
  engine/koilo/systems/**/*.cpp
  engine/koilo/debug/**/*.cpp
  engine/koilo/ecs/**/*.cpp
  engine/koilo/resources/**/*.cpp
  engine/koilo/modules/**/*.cpp
)

list(FILTER KL_CORE_SOURCES EXCLUDE REGEX ".*/systems/display/.*")
list(FILTER KL_CORE_SOURCES EXCLUDE REGEX ".*/systems/render/gl/.*")

# Optional subsystem source exclusions
if(NOT KOILO_ENABLE_AI)
  list(FILTER KL_CORE_SOURCES EXCLUDE REGEX ".*/systems/ai/.*")
endif()
if(NOT KOILO_ENABLE_AUDIO)
  list(FILTER KL_CORE_SOURCES EXCLUDE REGEX ".*/systems/audio/.*")
endif()
if(NOT KOILO_ENABLE_PARTICLES)
  list(FILTER KL_CORE_SOURCES EXCLUDE REGEX ".*/systems/particles/.*")
endif()

# Add the core reflection auto-registration file (safe for headless builds)
list(APPEND KL_CORE_SOURCES ${KL_GEN_DIR}/reflection_core_gen.cpp)

add_library(koilo_core STATIC ${KL_CORE_SOURCES})
add_dependencies(koilo_core koilo_generate)

target_link_libraries(koilo_core PUBLIC koilo_headers)
target_compile_features(koilo_core PUBLIC cxx_std_17)

# Optional subsystem compile definitions
if(KOILO_ENABLE_AI)
  target_compile_definitions(koilo_core PUBLIC KOILO_ENABLE_AI)
endif()
if(KOILO_ENABLE_AUDIO)
  target_compile_definitions(koilo_core PUBLIC KOILO_ENABLE_AUDIO)
endif()
if(KOILO_ENABLE_PARTICLES)
  target_compile_definitions(koilo_core PUBLIC KOILO_ENABLE_PARTICLES)
endif()

# Audio backend
find_package(SDL3 QUIET)
if(SDL3_FOUND)
  message(STATUS "Audio backend: SDL3")
  target_link_libraries(koilo_core PRIVATE SDL3::SDL3)
  target_compile_definitions(koilo_core PRIVATE KL_AUDIO_SDL3)
else()
  message(STATUS "Audio backend: disabled (SDL3 not found)")
  target_compile_definitions(koilo_core PRIVATE KL_AUDIO_NONE)
endif()

if(KL_WARNING_FLAGS)
  target_compile_options(koilo_core PRIVATE ${KL_WARNING_FLAGS})
endif()

add_subdirectory(engine/koilo/systems/display)

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
