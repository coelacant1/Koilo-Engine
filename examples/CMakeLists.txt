# Examples build configuration
find_package(SDL3 QUIET)

if(SDL3_FOUND OR TARGET SDL3::SDL3)
  # Example scripts are now run via: koilo --script examples/<name>/<name>.ks
  # Only module_demo needs its own executable (demonstrates dynamic module loading).
  set(KL_EXAMPLE_INCLUDES
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/engine/koilo
    ${CMAKE_SOURCE_DIR}/engine/koilo/modules
    ${CMAKE_BINARY_DIR}/generated
  )
  set(KL_EXAMPLE_LIBS koilo_core koiloengine_display SDL3::SDL3)

  add_executable(module_demo ${CMAKE_CURRENT_SOURCE_DIR}/modules/main.cpp)
  target_link_libraries(module_demo PRIVATE ${KL_EXAMPLE_LIBS})
  target_include_directories(module_demo PRIVATE ${KL_EXAMPLE_INCLUDES})
  if(TARGET sensor_module)
    add_dependencies(module_demo sensor_module game_mode_module fx_module)
  endif()
else()
  message(STATUS "SDL3 not found - skipping example builds")
endif()

# Demo ELF modules (no SDL3 dependency - standalone shared libs)
add_subdirectory(modules)
