# Display System Library

# Find dependencies
find_package(SDL2 QUIET)
find_package(OpenGL QUIET)

if(UNIX AND NOT APPLE)
    find_package(GLEW QUIET)
endif()

# Source files
set(DISPLAY_SOURCES
    displaymanager.cpp
    backends/nullbackend.cpp
)

set(DISPLAY_HEADERS
    ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/pixelformat.hpp
    ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/framebuffer.hpp
    ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/displayinfo.hpp
    ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/idisplaybackend.hpp
    ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/displaymanager.hpp
    ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/backends/nullbackend.hpp
)

# Add SDL2 software backend if available
if(SDL2_FOUND)
    message(STATUS "SDL2 software backend enabled")
    list(APPEND DISPLAY_SOURCES backends/gpu/sdl2backend.cpp)
    list(APPEND DISPLAY_HEADERS ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/backends/gpu/sdl2backend.hpp)
    set(HAVE_SDL2_BACKEND ON)
else()
    message(STATUS "SDL2 software backend disabled (SDL2 not found)")
    set(HAVE_SDL2_BACKEND OFF)
endif()

# Add OpenGL backend if available
if(SDL2_FOUND AND OPENGL_FOUND)
    message(STATUS "OpenGL backend enabled")
    list(APPEND DISPLAY_SOURCES backends/gpu/openglbackend.cpp)
    list(APPEND DISPLAY_HEADERS ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/backends/gpu/openglbackend.hpp)
    # GL render backend (uses same GL context as display backend)
    list(APPEND DISPLAY_SOURCES ${CMAKE_SOURCE_DIR}/engine/koilo/systems/render/gl/opengl_render_backend.cpp)
    list(APPEND DISPLAY_HEADERS ${CMAKE_SOURCE_DIR}/engine/koilo/systems/render/gl/opengl_render_backend.hpp)
    # Render backend factory (auto-selects GPU or software)
    list(APPEND DISPLAY_SOURCES ${CMAKE_SOURCE_DIR}/engine/koilo/systems/render/gl/render_backend_factory.cpp)
    list(APPEND DISPLAY_HEADERS ${CMAKE_SOURCE_DIR}/engine/koilo/systems/render/gl/render_backend_factory.hpp)
    set(HAVE_OPENGL_BACKEND ON)
else()
    message(STATUS "OpenGL backend disabled (SDL2 or OpenGL not found)")
    set(HAVE_OPENGL_BACKEND OFF)
endif()

# Create library
add_library(koiloengine_display STATIC ${DISPLAY_SOURCES} ${DISPLAY_HEADERS})

# Add platform-specific reflection registration (display backends, KoiloApplication, renderer)
target_sources(koiloengine_display PRIVATE ${KL_GEN_DIR}/reflection_platform_gen.cpp)

# Ensure generated files exist before building
add_dependencies(koiloengine_display koilo_generate)

# Include directories
target_include_directories(koiloengine_display PUBLIC
    ${CMAKE_SOURCE_DIR}/engine
    ${KL_GEN_DIR}
)

# Link dependencies
target_link_libraries(koiloengine_display PUBLIC koilo_core)

if(HAVE_SDL2_BACKEND)
    target_include_directories(koiloengine_display PRIVATE
        ${SDL2_INCLUDE_DIRS}
    )
    target_link_libraries(koiloengine_display PUBLIC
        ${SDL2_LIBRARIES}
    )
    target_compile_definitions(koiloengine_display PUBLIC KL_HAVE_SDL2_BACKEND)
endif()

if(HAVE_OPENGL_BACKEND)
    target_include_directories(koiloengine_display PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIR}
    )
    
    if(UNIX AND NOT APPLE)
        target_link_libraries(koiloengine_display PUBLIC
            ${SDL2_LIBRARIES}
            ${OPENGL_LIBRARIES}
            GLEW::GLEW
        )
    else()
        target_link_libraries(koiloengine_display PUBLIC
            ${SDL2_LIBRARIES}
            ${OPENGL_LIBRARIES}
        )
    endif()
    
    target_compile_definitions(koiloengine_display PUBLIC KL_HAVE_OPENGL_BACKEND KL_HAVE_FILESYSTEM)

    # --- KSL Shader Build: .ksl.hpp -> .kso + .glsl at build time ---
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_FOUND)
        set(KSL_SHADER_DIR "${CMAKE_SOURCE_DIR}/shaders")
        set(KSL_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
        set(KSL_CODEGEN    "${CMAKE_SOURCE_DIR}/tools/ksl/ksl_codegen.py")
        set(KSL_COMPILER   "${CMAKE_SOURCE_DIR}/tools/ksl/ksl_compiler.py")

        file(GLOB KSL_SHADER_SOURCES "${KSL_SHADER_DIR}/*.ksl.hpp")
        file(MAKE_DIRECTORY ${KSL_OUTPUT_DIR})

        set(KSL_ALL_OUTPUTS "")
        foreach(KSL_SRC ${KSL_SHADER_SOURCES})
            get_filename_component(KSL_NAME ${KSL_SRC} NAME_WLE)  # e.g. spiral.ksl
            string(REPLACE ".ksl" "" KSL_STEM ${KSL_NAME})        # e.g. spiral
            set(KSL_GLSL "${KSL_OUTPUT_DIR}/${KSL_STEM}.glsl")
            set(KSL_KSO  "${KSL_OUTPUT_DIR}/${KSL_STEM}.kso")

            # Generate .glsl via ksl_codegen.py
            add_custom_command(
                OUTPUT ${KSL_GLSL}
                COMMAND ${Python3_EXECUTABLE} ${KSL_CODEGEN} ${KSL_SRC} -o ${KSL_GLSL}
                DEPENDS ${KSL_SRC} ${KSL_CODEGEN}
                COMMENT "KSL: ${KSL_STEM}.ksl.hpp -> ${KSL_STEM}.glsl"
            )

            # Compile .kso via ksl_compiler
            add_custom_command(
                OUTPUT ${KSL_KSO}
                COMMAND ${Python3_EXECUTABLE} ${KSL_COMPILER} ${KSL_SRC}
                    -o ${KSL_KSO} --no-glsl
                DEPENDS ${KSL_SRC} ${KSL_COMPILER}
                COMMENT "KSL: ${KSL_STEM}.ksl.hpp -> ${KSL_STEM}.kso"
            )

            list(APPEND KSL_ALL_OUTPUTS ${KSL_GLSL} ${KSL_KSO})
        endforeach()

        # Generate uber-shader (single GLSL containing all shade functions)
        set(KSL_UBER_GLSL "${KSL_OUTPUT_DIR}/uber.glsl")
        set(KSL_UBER_IDS  "${KSL_OUTPUT_DIR}/uber_ids.txt")
        add_custom_command(
            OUTPUT ${KSL_UBER_GLSL} ${KSL_UBER_IDS}
            COMMAND ${Python3_EXECUTABLE} ${KSL_CODEGEN} --uber ${KSL_SHADER_SOURCES}
                -o ${KSL_UBER_GLSL} --uber-ids ${KSL_UBER_IDS}
            DEPENDS ${KSL_SHADER_SOURCES} ${KSL_CODEGEN}
            COMMENT "KSL: uber-shader -> uber.glsl"
        )
        list(APPEND KSL_ALL_OUTPUTS ${KSL_UBER_GLSL} ${KSL_UBER_IDS})

        add_custom_target(ksl_shaders ALL DEPENDS ${KSL_ALL_OUTPUTS})
        add_dependencies(koiloengine_display ksl_shaders)

        message(STATUS "KSL shaders: ${KSL_OUTPUT_DIR}/ (.glsl + .kso + uber.glsl)")
    else()
        message(WARNING "Python3 not found - KSL shader generation disabled")
    endif()
endif()

# Set C++ standard
set_target_properties(koiloengine_display PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Install
install(TARGETS koiloengine_display
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display
    DESTINATION include/koilo/systems
    FILES_MATCHING PATTERN "*.hpp"
)
