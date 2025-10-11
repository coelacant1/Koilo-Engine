cmake_minimum_required(VERSION 3.21)
project(PTXEngine LANGUAGES C CXX)

include(GNUInstallDirs)

# Options
option(PTX_BUILD_LUA "Build Lua binding" ON)
option(PTX_BUILD_TESTS "Build C++ tests" ON)
option(PTX_BUILD_PYTHON "Enable Python binding helpers" ON)
option(PTX_ENABLE_WARNINGS "Enable project warning flags" ON)
option(PTX_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(PTX_USE_SYSTEM_UNITY "Use an existing Unity test framework checkout" OFF)
set(PTX_UNITY_DIR ${CMAKE_SOURCE_DIR}/external/unity CACHE PATH "Path to Unity test framework")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(default_build_type "RelWithDebInfo")
  message(STATUS "Setting build type to '${default_build_type}' as none was specified")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
endif()

set(PTX_WARNING_FLAGS)
if(PTX_ENABLE_WARNINGS)
  list(APPEND PTX_WARNING_FLAGS -Wall -Wextra)
  if(PTX_WARNINGS_AS_ERRORS)
    list(APPEND PTX_WARNING_FLAGS -Werror)
  endif()
endif()

# Directories
set(PTX_SOURCE_ROOT ${CMAKE_SOURCE_DIR})
set(PTX_SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/scripts)
set(PTX_GEN_DIR ${CMAKE_BINARY_DIR}/generated)

# Python (for generation)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Generation custom commands / targets
add_custom_command(
  OUTPUT ${PTX_GEN_DIR}/ptx/ptxall.hpp
  BYPRODUCTS ${PTX_GEN_DIR}/.ptx_reflect_cache.json
  COMMAND ${CMAKE_COMMAND} -E make_directory ${PTX_GEN_DIR}/ptx
  COMMAND Python3::Interpreter ${PTX_SCRIPTS_DIR}/generateptxall.py --root ${CMAKE_SOURCE_DIR}/engine/include/ptx --output ${PTX_GEN_DIR}/ptx/ptxall.hpp
  COMMAND Python3::Interpreter ${PTX_SCRIPTS_DIR}/updateptxregistry.py --root ${CMAKE_SOURCE_DIR}/engine/include/ptx --write --cache-file ${PTX_GEN_DIR}/.ptx_reflect_cache.json
  DEPENDS ${PTX_SCRIPTS_DIR}/generateptxall.py ${PTX_SCRIPTS_DIR}/updateptxregistry.py ${PTX_SCRIPTS_DIR}/consoleoutput.py
  WORKING_DIRECTORY ${PTX_SCRIPTS_DIR}
  COMMENT "Generating reflection umbrella header and PTX registry cache"
  VERBATIM
)

# Test skeleton generation (runs after reflection generation)
add_custom_command(
  OUTPUT ${PTX_GEN_DIR}/.test_skeleton_cache.json
  COMMAND Python3::Interpreter ${PTX_SCRIPTS_DIR}/generatetestskeletons.py
          --all
          --cache-file ${PTX_GEN_DIR}/.test_skeleton_cache.json
  DEPENDS
    ${PTX_SCRIPTS_DIR}/generatetestskeletons.py
    ${PTX_SCRIPTS_DIR}/consoleoutput.py
    ${PTX_GEN_DIR}/reflection_entry_gen.cpp
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Generating test skeletons for untested classes"
  VERBATIM
)

add_custom_target(ptx_update_registry ALL
  DEPENDS ${PTX_GEN_DIR}/ptx/ptxall.hpp
)

add_custom_command(
  OUTPUT ${PTX_GEN_DIR}/reflection_entry_gen.cpp
  COMMAND ${CMAKE_COMMAND} -E make_directory ${PTX_GEN_DIR}
  # Generate a translation unit that explicitly calls <Class>::Describe() for each
  # reflected class so their static registration objects are emitted at link/run
  # time. The helper script scans headers for PTX_BEGIN_DESCRIBE and writes a TU
  # containing (void)Class::Describe(); lines.
  COMMAND Python3::Interpreter ${PTX_SCRIPTS_DIR}/generatereflectionentry.py --root ${CMAKE_SOURCE_DIR}/engine/include/ptx --output ${PTX_GEN_DIR}/reflection_entry_gen.cpp
  DEPENDS ${PTX_SCRIPTS_DIR}/generatereflectionentry.py ${PTX_SCRIPTS_DIR}/consoleoutput.py ${PTX_GEN_DIR}/ptx/ptxall.hpp
  WORKING_DIRECTORY ${PTX_SCRIPTS_DIR}
  COMMENT "Generating minimal registry translation unit"
  VERBATIM
)

add_custom_target(ptx_generate ALL DEPENDS ${PTX_GEN_DIR}/reflection_entry_gen.cpp)
add_dependencies(ptx_generate ptx_update_registry)

# Test skeleton generation target
add_custom_target(ptx_generate_test_skeletons
  DEPENDS ${PTX_GEN_DIR}/.test_skeleton_cache.json
)
add_dependencies(ptx_generate_test_skeletons ptx_generate)

add_library(ptx_headers INTERFACE)
target_include_directories(ptx_headers INTERFACE
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/engine/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/engine/include/ptx>
  $<BUILD_INTERFACE:${PTX_GEN_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Gather core sources (initial explicit list; can later auto-generate with a helper script)
file(GLOB_RECURSE PTX_CORE_SOURCES
  CONFIGURE_DEPENDS
  engine/src/core/**/*.cpp
  engine/src/assets/**/*.cpp
  engine/src/systems/**/*.cpp
  engine/src/project/**/*.cpp
)

# Exclude reflection generated file from core (it will go into reflect lib)
list(REMOVE_ITEM PTX_CORE_SOURCES ${PTX_GEN_DIR}/reflection_entry_gen.cpp)

add_library(ptx_core STATIC ${PTX_CORE_SOURCES})
add_dependencies(ptx_core ptx_generate)
target_link_libraries(ptx_core PUBLIC ptx_headers)
target_compile_features(ptx_core PUBLIC cxx_std_17)
if(PTX_WARNING_FLAGS)
  target_compile_options(ptx_core PRIVATE ${PTX_WARNING_FLAGS})
endif()

# Reflection C API + generated TU
add_library(ptx_reflect SHARED
  ${PTX_GEN_DIR}/reflection_entry_gen.cpp
  bindings/c_api/reflect_capi.cpp
)
add_dependencies(ptx_reflect ptx_generate)
if(TARGET ptx_headercheck)
  add_dependencies(ptx_reflect ptx_headercheck)
endif()

# Link core publicly so consumers of ptx_reflect (tests, bindings) also link ptx_core
# This ensures symbols defined only in ptx_core (math, vectors, etc.) are available
# without forcing each downstream target to specify ptx_core explicitly.
target_link_libraries(ptx_reflect PUBLIC ptx_core)
target_compile_features(ptx_reflect PUBLIC cxx_std_17)
if(PTX_WARNING_FLAGS)
  target_compile_options(ptx_reflect PRIVATE ${PTX_WARNING_FLAGS})
endif()

target_include_directories(ptx_reflect PUBLIC
  ${CMAKE_SOURCE_DIR}/bindings/c_api
)

# Lua module
if(PTX_BUILD_LUA)
  find_package(Lua 5.3 REQUIRED)
  add_library(ptx_lua MODULE bindings/lua/src/ptx_lua.c)
  set_property(TARGET ptx_lua PROPERTY LINKER_LANGUAGE C)
  target_link_libraries(ptx_lua PRIVATE ptx_reflect ${LUA_LIBRARIES})
  # Remove "lib" prefix and ensure extension is .so on *nix
  set_target_properties(ptx_lua PROPERTIES
    PREFIX ""
    OUTPUT_NAME "ptx" # so Lua 'require("ptx")' finds ptx.so
    BUILD_WITH_INSTALL_RPATH ON
    SKIP_BUILD_RPATH OFF
    INSTALL_RPATH "\$ORIGIN"
    BUILD_RPATH "\$ORIGIN"
  )
  target_include_directories(ptx_lua PRIVATE ${LUA_INCLUDE_DIR})
  if(PTX_WARNING_FLAGS)
    target_compile_options(ptx_lua PRIVATE ${PTX_WARNING_FLAGS})
  endif()
  # Copy the dependent shared library next to the Lua module for local 'lua test_reflection.lua'
  add_custom_command(TARGET ptx_lua POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ptx_reflect> $<TARGET_FILE_DIR:ptx_lua>/ptx_reflect.so)
endif()

# Tests
if(PTX_BUILD_TESTS)
  enable_testing()
  # Recursively gather all test sources (including test.cpp in root and subdirectories)
  file(GLOB_RECURSE PTX_TEST_SOURCES CONFIGURE_DEPENDS
    tests/engine/*.cpp
    tests/engine/**/*.cpp
  )
  set(PTX_UNITY_SOURCES)
  set(PTX_UNITY_INCLUDE)

  if(PTX_USE_SYSTEM_UNITY)
    if(NOT EXISTS ${PTX_UNITY_DIR}/unity.c)
      message(FATAL_ERROR "PTX_USE_SYSTEM_UNITY=ON but no unity.c found at ${PTX_UNITY_DIR}. Set PTX_UNITY_DIR to a valid Unity checkout.")
    endif()
    message(STATUS "Using Unity test framework from system path ${PTX_UNITY_DIR}")
    set(PTX_UNITY_SOURCES ${PTX_UNITY_DIR}/unity.c)
    set(PTX_UNITY_INCLUDE ${PTX_UNITY_DIR})
  elseif(EXISTS ${PTX_UNITY_DIR}/unity.c)
    message(STATUS "Using vendored Unity test framework at ${PTX_UNITY_DIR}")
    set(PTX_UNITY_SOURCES ${PTX_UNITY_DIR}/unity.c)
    set(PTX_UNITY_INCLUDE ${PTX_UNITY_DIR})
  else()
    include(FetchContent)
    FetchContent_Declare(
      unity
      GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
      GIT_TAG v2.6.0 # update as needed
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(unity)
    set(PTX_UNITY_SOURCES ${unity_SOURCE_DIR}/src/unity.c)
    set(PTX_UNITY_INCLUDE ${unity_SOURCE_DIR}/src)
  endif()

  add_executable(ptx_tests ${PTX_TEST_SOURCES} ${PTX_UNITY_SOURCES})
  # Ensure test skeletons are generated before building tests
  add_dependencies(ptx_tests ptx_generate_test_skeletons)
  # ptx_reflect already propagates ptx_core publicly; but list explicitly for clarity
  target_link_libraries(ptx_tests PRIVATE ptx_reflect ptx_core)
  target_include_directories(ptx_tests PRIVATE
    ${PTX_UNITY_INCLUDE}
    ${CMAKE_SOURCE_DIR}/tests/engine
  )
  target_compile_features(ptx_tests PRIVATE cxx_std_17)
  if(PTX_WARNING_FLAGS)
    target_compile_options(ptx_tests PRIVATE ${PTX_WARNING_FLAGS})
  endif()
  add_test(NAME ptx_core_tests COMMAND ptx_tests)
endif()

# Header smoke-test: compile every PTX header as its own translation unit
file(GLOB_RECURSE PTX_HEADERCHECK_HEADERS
  CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/engine/include/ptx/*.hpp
)

set(PTX_HEADERCHECK_STUBS)
foreach(header IN LISTS PTX_HEADERCHECK_HEADERS)
  # Filter out template and virtual headers (same as GeneratePTXAll.py)
  file(READ ${header} header_content)
  string(REGEX MATCH "template[ \t\n]*<" has_template "${header_content}")
  string(REGEX MATCH "virtual" has_virtual "${header_content}")
  if(has_template OR has_virtual)
    continue()
  endif()

  file(RELATIVE_PATH rel_path ${CMAKE_SOURCE_DIR}/engine/include ${header})
  set(stub "${CMAKE_BINARY_DIR}/headercheck/${rel_path}.cpp")
  cmake_path(GET stub PARENT_PATH stub_dir)
  file(MAKE_DIRECTORY ${stub_dir})
  file(WRITE ${stub} "#include \"${rel_path}\"\n")
  list(APPEND PTX_HEADERCHECK_STUBS ${stub})
endforeach()

if(PTX_HEADERCHECK_STUBS)
  add_library(ptx_headercheck_objs OBJECT ${PTX_HEADERCHECK_STUBS})
  add_dependencies(ptx_headercheck_objs ptx_generate)
  target_link_libraries(ptx_headercheck_objs PRIVATE ptx_headers)
  target_compile_features(ptx_headercheck_objs PRIVATE cxx_std_17)
  target_compile_options(ptx_headercheck_objs PRIVATE -Wno-pragmas)
  add_custom_target(ptx_headercheck DEPENDS ptx_headercheck_objs)
endif()

if(PTX_BUILD_PYTHON)
  message(STATUS "PTX Python helper targets enabled (no native build steps)")
endif()

message(STATUS "PTXEngine configuration complete")
