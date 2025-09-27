cmake_minimum_required(VERSION 3.21)
project(PTXEngine LANGUAGES C CXX)

# Options
option(PTX_BUILD_LUA "Build Lua binding" ON)
option(PTX_BUILD_TESTS "Build C++ tests" ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Directories
set(PTX_SOURCE_ROOT ${CMAKE_SOURCE_DIR})
set(PTX_SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/scripts)
set(PTX_GEN_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PTX_GEN_DIR})
file(MAKE_DIRECTORY ${PTX_GEN_DIR}/ptx)

# Python (for generation)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Generation custom command
add_custom_command(
  OUTPUT ${PTX_GEN_DIR}/ptx/ptxall.hpp ${PTX_GEN_DIR}/reflection_entry_gen.cpp
  COMMAND Python3::Interpreter ${PTX_SCRIPTS_DIR}/GeneratePTXAll.py --root ${CMAKE_SOURCE_DIR}/engine/include/ptx --output ${PTX_GEN_DIR}/ptx/ptxall.hpp
  COMMAND Python3::Interpreter ${PTX_SCRIPTS_DIR}/UpdatePTXRegistry.py --root ${CMAKE_SOURCE_DIR}/engine/include/ptx --write --cache-file ${PTX_GEN_DIR}/.ptx_reflect_cache.json
  COMMAND ${CMAKE_COMMAND} -E echo "// Auto-generated registry translation unit" > ${PTX_GEN_DIR}/reflection_entry_gen.cpp
  COMMAND ${CMAKE_COMMAND} -E echo "#include <ptx/ptxall.hpp>" >> ${PTX_GEN_DIR}/reflection_entry_gen.cpp
  COMMAND ${CMAKE_COMMAND} -E echo "// Force registration by referencing Describe() on each class (filled by generator in future)" >> ${PTX_GEN_DIR}/reflection_entry_gen.cpp
  DEPENDS ${PTX_SCRIPTS_DIR}/GeneratePTXAll.py ${PTX_SCRIPTS_DIR}/UpdatePTXRegistry.py
  WORKING_DIRECTORY ${PTX_SCRIPTS_DIR}
  COMMENT "Generating reflection umbrella + minimal registry translation unit"
  VERBATIM
)

add_custom_target(ptx_generate ALL DEPENDS ${PTX_GEN_DIR}/ptx/ptxall.hpp ${PTX_GEN_DIR}/reflection_entry_gen.cpp)

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

target_include_directories(ptx_core PUBLIC
  ${CMAKE_SOURCE_DIR}/engine/include
  ${CMAKE_SOURCE_DIR}/engine/include/ptx
  ${PTX_GEN_DIR}
)

# Reflection C API + generated TU
add_library(ptx_reflect SHARED
  ${PTX_GEN_DIR}/reflection_entry_gen.cpp
  bindings/c_api/reflect_capi.cpp
)
add_dependencies(ptx_reflect ptx_generate)

# Link core publicly so consumers of ptx_reflect (tests, bindings) also link ptx_core
# This ensures symbols defined only in ptx_core (math, vectors, etc.) are available
# without forcing each downstream target to specify ptx_core explicitly.
target_link_libraries(ptx_reflect PUBLIC ptx_core)

target_include_directories(ptx_reflect PUBLIC
  ${CMAKE_SOURCE_DIR}/engine/include
  ${CMAKE_SOURCE_DIR}/engine/include/ptx
  ${PTX_GEN_DIR}
  ${CMAKE_SOURCE_DIR}/bindings/c_api
)

# Lua module
if(PTX_BUILD_LUA)
  add_library(ptx_lua MODULE bindings/lua/src/ptx_lua.c)
  set_property(TARGET ptx_lua PROPERTY LINKER_LANGUAGE C)
  target_link_libraries(ptx_lua PRIVATE ptx_reflect)
  # Remove "lib" prefix and ensure extension is .so on *nix
  set_target_properties(ptx_lua PROPERTIES
    PREFIX ""
    OUTPUT_NAME "ptx" # so Lua 'require("ptx")' finds ptx.so
    BUILD_WITH_INSTALL_RPATH ON
    SKIP_BUILD_RPATH OFF
    INSTALL_RPATH "\$ORIGIN"
    BUILD_RPATH "\$ORIGIN"
  )
  target_include_directories(ptx_lua PRIVATE
    ${CMAKE_SOURCE_DIR}/bindings/c_api
    ${CMAKE_SOURCE_DIR}/engine/include
    ${CMAKE_SOURCE_DIR}/engine/include/ptx
    ${PTX_GEN_DIR}
  )
  # Copy the dependent shared library next to the Lua module for local 'lua test_reflection.lua'
  add_custom_command(TARGET ptx_lua POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ptx_reflect> $<TARGET_FILE_DIR:ptx_lua>/ptx_reflect.so)
endif()

# Tests
if(PTX_BUILD_TESTS)
  include(FetchContent)
  enable_testing()
  file(GLOB PTX_TEST_SOURCES CONFIGURE_DEPENDS tests/engine/*.cpp)

  # Unity test framework fetch (if external/unity not already vendored)
  set(PTX_UNITY_DIR ${CMAKE_SOURCE_DIR}/external/unity)
  if(NOT EXISTS ${PTX_UNITY_DIR}/unity.c)
    message(STATUS "Fetching Unity test framework...")
    FetchContent_Declare(
      unity
      GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
      GIT_TAG v2.6.0 # update as needed
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(unity)
    # Unity repository places sources under src/ (unity.c, unity.h, unity_internals.h)
    set(PTX_UNITY_SOURCES ${unity_SOURCE_DIR}/src/unity.c)
    set(PTX_UNITY_INCLUDE ${unity_SOURCE_DIR}/src)
    # Optional: copy minimal required files into external/unity for IDE browsing / offline reuse
    file(MAKE_DIRECTORY ${PTX_UNITY_DIR})
    foreach(_f unity.c unity.h unity_internals.h)
      if(EXISTS ${unity_SOURCE_DIR}/src/${_f})
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${unity_SOURCE_DIR}/src/${_f} ${PTX_UNITY_DIR}/${_f})
      endif()
    endforeach()
  else()
    set(PTX_UNITY_SOURCES ${PTX_UNITY_DIR}/unity.c)
    set(PTX_UNITY_INCLUDE ${PTX_UNITY_DIR})
  endif()

  add_executable(ptx_tests ${PTX_TEST_SOURCES} ${PTX_UNITY_SOURCES})
  # ptx_reflect already propagates ptx_core publicly; we still list explicitly for clarity
  target_link_libraries(ptx_tests PRIVATE ptx_reflect ptx_core)
  target_include_directories(ptx_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/engine/include
    ${CMAKE_SOURCE_DIR}/engine/include/ptx
    ${PTX_GEN_DIR}
    ${PTX_UNITY_INCLUDE}
  )
  add_test(NAME ptx_core_tests COMMAND ptx_tests)
endif()

message(STATUS "PTXEngine configuration complete")
