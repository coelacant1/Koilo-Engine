# Demo ELF modules - built as shared libraries loadable at runtime.
# These demonstrate the Koilo Engine external module C ABI.
#
# Each module compiles to a standalone .so (or .dll on Windows) with no
# engine library linkage - interaction is entirely through the C ABI
# callbacks provided by EngineServices.

option(KL_BUILD_DEMO_MODULES "Build demo ELF module plugins" ON)

if(NOT KL_BUILD_DEMO_MODULES)
  return()
endif()

set(MODULE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/modules"
    CACHE PATH "Directory for built module .so files")
file(MAKE_DIRECTORY ${MODULE_OUTPUT_DIR})

# Common compile settings for all modules
function(koilo_add_module target source)
  add_library(${target} SHARED ${source})
  target_include_directories(${target} PRIVATE
    ${CMAKE_SOURCE_DIR}/engine
  )
  set_target_properties(${target} PROPERTIES
    PREFIX ""                          # no "lib" prefix
    SUFFIX ".so"                       # explicit .so suffix on all platforms
    LIBRARY_OUTPUT_DIRECTORY ${MODULE_OUTPUT_DIR}
    POSITION_INDEPENDENT_CODE ON
  )
  target_compile_features(${target} PRIVATE cxx_std_17)
  target_compile_options(${target} PRIVATE -fno-rtti -fno-exceptions -O2)
endfunction()

# --- Sensor module: simulated hardware sensors ---
koilo_add_module(sensor_module
  ${CMAKE_CURRENT_SOURCE_DIR}/sensor_module.cpp)

# --- Game mode module: score/lives/timer state ---
koilo_add_module(game_mode_module
  ${CMAKE_CURRENT_SOURCE_DIR}/game_mode_module.cpp)

# --- FX module: pixel buffer post-processing ---
koilo_add_module(fx_module
  ${CMAKE_CURRENT_SOURCE_DIR}/fx_module.cpp)

message(STATUS "Demo modules will be built to: ${MODULE_OUTPUT_DIR}")
