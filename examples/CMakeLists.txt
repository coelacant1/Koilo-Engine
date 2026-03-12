# Examples build configuration
find_package(SDL3 QUIET)

if(SDL3_FOUND OR TARGET SDL3::SDL3)
  # Space Shooter example
  add_executable(space_shooter ${CMAKE_CURRENT_SOURCE_DIR}/space_shooter/main.cpp)
  target_link_libraries(space_shooter PRIVATE koilo_core koiloengine_display SDL3::SDL3)
  target_include_directories(space_shooter PRIVATE
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/engine/koilo
    ${CMAKE_SOURCE_DIR}/engine/koilo/modules
    ${CMAKE_BINARY_DIR}/generated
  )

  # 3D scene example
  add_executable(scene_3d ${CMAKE_CURRENT_SOURCE_DIR}/scene_3d/main.cpp)
  target_link_libraries(scene_3d PRIVATE koilo_core koiloengine_display SDL3::SDL3)
  target_include_directories(scene_3d PRIVATE
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/engine/koilo
    ${CMAKE_SOURCE_DIR}/engine/koilo/modules
    ${CMAKE_BINARY_DIR}/generated
  )

  # Stress test example
  add_executable(stress_test ${CMAKE_CURRENT_SOURCE_DIR}/stress_test/main.cpp)
  target_link_libraries(stress_test PRIVATE koilo_core koiloengine_display SDL3::SDL3)
  target_include_directories(stress_test PRIVATE
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/engine/koilo
    ${CMAKE_SOURCE_DIR}/engine/koilo/modules
    ${CMAKE_BINARY_DIR}/generated
  )

  # Syntax demo example
  add_executable(syntax_demo ${CMAKE_CURRENT_SOURCE_DIR}/syntax_demo/main.cpp)
  target_link_libraries(syntax_demo PRIVATE koilo_core koiloengine_display SDL3::SDL3)
  target_include_directories(syntax_demo PRIVATE
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/engine/koilo
    ${CMAKE_SOURCE_DIR}/engine/koilo/modules
    ${CMAKE_BINARY_DIR}/generated
  )

  # OBJ scene example
  add_executable(obj_scene ${CMAKE_CURRENT_SOURCE_DIR}/obj_scene/main.cpp)
  target_link_libraries(obj_scene PRIVATE koilo_core koiloengine_display SDL3::SDL3)
  target_include_directories(obj_scene PRIVATE
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/engine/koilo
    ${CMAKE_SOURCE_DIR}/engine/koilo/modules
    ${CMAKE_BINARY_DIR}/generated
  )

  # Module demo example
  add_executable(module_demo ${CMAKE_CURRENT_SOURCE_DIR}/modules/main.cpp)
  target_link_libraries(module_demo PRIVATE koilo_core koiloengine_display SDL3::SDL3)
  target_include_directories(module_demo PRIVATE
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/engine/koilo
    ${CMAKE_SOURCE_DIR}/engine/koilo/modules
    ${CMAKE_BINARY_DIR}/generated
  )
  if(TARGET sensor_module)
    add_dependencies(module_demo sensor_module game_mode_module fx_module)
  endif()

  # UI demo example
  add_executable(ui_demo ${CMAKE_CURRENT_SOURCE_DIR}/ui_demo/main.cpp)
  target_link_libraries(ui_demo PRIVATE koilo_core koiloengine_display SDL3::SDL3)
  target_include_directories(ui_demo PRIVATE
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/engine/koilo
    ${CMAKE_SOURCE_DIR}/engine/koilo/modules
    ${CMAKE_BINARY_DIR}/generated
  )
else()
  message(STATUS "SDL3 not found  skipping example builds")
endif()

# Demo ELF modules (no SDL3 dependency - standalone shared libs)
add_subdirectory(modules)
