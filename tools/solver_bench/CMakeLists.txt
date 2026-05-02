# SPDX-License-Identifier: GPL-3.0-or-later
#
# Solver benchmark harness.
#
# Standalone executable. Headless. Emits CSV to stdout (per-step + summary),
# so results can be diffed across solver variants. NOT wired into the default
# test runner - this is an on-demand A/B tool.
#
# Build target: koilo_solver_bench
# Run:          ./koilo_solver_bench [--scene <name>] [--csv per_step|summary|both]

add_executable(koilo_solver_bench
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/scenes.cpp
)
target_link_libraries(koilo_solver_bench PRIVATE koilo_engine)
target_include_directories(koilo_solver_bench PRIVATE
  ${CMAKE_SOURCE_DIR}/engine
  ${CMAKE_SOURCE_DIR}/engine/koilo
  ${CMAKE_BINARY_DIR}/generated
)
target_compile_features(koilo_solver_bench PRIVATE cxx_std_17)
if(KL_WARNING_FLAGS)
  target_compile_options(koilo_solver_bench PRIVATE ${KL_WARNING_FLAGS})
endif()
