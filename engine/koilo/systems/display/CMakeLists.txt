# Display System Library

# Find dependencies
find_package(SDL3 QUIET)
find_package(OpenGL QUIET)

# glad (GL loader, replaces GLEW - works on both GLX and EGL/Wayland)
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/external/glad)

# Source files
set(DISPLAY_SOURCES
    displaymanager.cpp
    backends/nullbackend.cpp
)

set(DISPLAY_HEADERS
    ${CMAKE_SOURCE_DIR}/engine/koilo/core/color/pixelformat.hpp
    ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/framebuffer.hpp
    ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/displayinfo.hpp
    ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/idisplaybackend.hpp
    ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/displaymanager.hpp
    ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/backends/nullbackend.hpp
)

# Add SDL3 software backend if available
if(SDL3_FOUND)
    message(STATUS "SDL3 software backend enabled")
    list(APPEND DISPLAY_SOURCES backends/gpu/sdl3backend.cpp)
    list(APPEND DISPLAY_HEADERS ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/backends/gpu/sdl3backend.hpp)
    set(HAVE_SDL3_BACKEND ON)
else()
    message(STATUS "SDL3 software backend disabled (SDL3 not found)")
    set(HAVE_SDL3_BACKEND OFF)
endif()

# Add OpenGL backend if available and requested
if(KOILO_USE_OPENGL AND SDL3_FOUND AND OPENGL_FOUND)
    message(STATUS "OpenGL backend enabled")
    list(APPEND DISPLAY_SOURCES backends/gpu/openglbackend.cpp)
    list(APPEND DISPLAY_HEADERS ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/backends/gpu/openglbackend.hpp)
    # OpenGL RHI device
    list(APPEND DISPLAY_SOURCES ${CMAKE_SOURCE_DIR}/engine/koilo/systems/render/rhi/opengl/opengl_rhi_device.cpp)
    list(APPEND DISPLAY_HEADERS ${CMAKE_SOURCE_DIR}/engine/koilo/systems/render/rhi/opengl/opengl_rhi_device.hpp)
    set(HAVE_OPENGL_BACKEND ON)
else()
    if(KOILO_USE_OPENGL)
        message(STATUS "OpenGL backend disabled (SDL3 or OpenGL not found)")
    else()
        message(STATUS "OpenGL backend disabled (KOILO_USE_OPENGL=OFF)")
    endif()
    set(HAVE_OPENGL_BACKEND OFF)
endif()

# Render backend factory is always needed (handles backend selection at runtime)
list(APPEND DISPLAY_SOURCES ${CMAKE_SOURCE_DIR}/engine/koilo/systems/render/gl/render_backend_factory.cpp)
list(APPEND DISPLAY_HEADERS ${CMAKE_SOURCE_DIR}/engine/koilo/systems/render/gl/render_backend_factory.hpp)

# Unified RHI UI renderer (works with any RHI backend)
list(APPEND DISPLAY_SOURCES ${CMAKE_SOURCE_DIR}/engine/koilo/systems/ui/render/ui_rhi_renderer.cpp)
list(APPEND DISPLAY_HEADERS ${CMAKE_SOURCE_DIR}/engine/koilo/systems/ui/render/ui_rhi_renderer.hpp)

# Add Vulkan backend if available and requested
set(HAVE_VULKAN_BACKEND OFF)
if(KOILO_USE_VULKAN AND SDL3_FOUND)
    find_package(Vulkan QUIET)
    if(Vulkan_FOUND)
        message(STATUS "Vulkan backend enabled")
        list(APPEND DISPLAY_SOURCES backends/gpu/vulkanbackend.cpp)
        list(APPEND DISPLAY_SOURCES ../render/rhi/vulkan/vulkan_rhi_device.cpp)
        list(APPEND DISPLAY_HEADERS
            ${CMAKE_SOURCE_DIR}/engine/koilo/systems/display/backends/gpu/vulkanbackend.hpp)
        set(HAVE_VULKAN_BACKEND ON)
    else()
        message(STATUS "Vulkan backend disabled (Vulkan SDK not found)")
    endif()
else()
    if(KOILO_USE_VULKAN AND NOT SDL3_FOUND)
        message(STATUS "Vulkan backend disabled (SDL3 not found)")
    endif()
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

if(HAVE_SDL3_BACKEND)
    target_link_libraries(koiloengine_display PUBLIC
        SDL3::SDL3
    )
    target_compile_definitions(koiloengine_display PUBLIC KL_HAVE_SDL3_BACKEND)
endif()

if(HAVE_OPENGL_BACKEND)
    target_include_directories(koiloengine_display PRIVATE
        ${OPENGL_INCLUDE_DIR}
        ${GLAD_DIR}/include
    )
    target_sources(koiloengine_display PRIVATE ${GLAD_DIR}/src/glad.c)

    target_link_libraries(koiloengine_display PUBLIC
        SDL3::SDL3
        ${OPENGL_LIBRARIES}
    )
    
    target_compile_definitions(koiloengine_display PUBLIC KL_HAVE_OPENGL_BACKEND)
endif()

# std::filesystem is always available (C++17)
target_compile_definitions(koiloengine_display PUBLIC KL_HAVE_FILESYSTEM)

if(HAVE_VULKAN_BACKEND)
    target_include_directories(koiloengine_display PRIVATE ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(koiloengine_display PUBLIC Vulkan::Vulkan SDL3::SDL3)
    target_compile_definitions(koiloengine_display PUBLIC KL_HAVE_VULKAN_BACKEND)
endif()

# --- KSL Shader Build: .ksl.hpp -> .kso + .glsl at build time ---
if(HAVE_OPENGL_BACKEND OR HAVE_VULKAN_BACKEND)
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

        # --- SPIR-V compilation for Vulkan backend ---
        if(KOILO_USE_VULKAN)
            find_program(GLSLANG_VALIDATOR glslangValidator)
            if(NOT GLSLANG_VALIDATOR)
                find_program(GLSLANG_VALIDATOR glslc)
            endif()

            if(GLSLANG_VALIDATOR)
                set(KSL_SPV_DIR "${KSL_OUTPUT_DIR}/spirv")
                file(MAKE_DIRECTORY ${KSL_SPV_DIR})

                set(KSL_SPV_OUTPUTS "")

                # Compile shared vertex shader
                set(KSL_VERT_SRC "${KSL_SHADER_DIR}/scene.vert")
                set(KSL_VERT_SPV "${KSL_SPV_DIR}/scene.vert.spv")
                add_custom_command(
                    OUTPUT ${KSL_VERT_SPV}
                    COMMAND ${GLSLANG_VALIDATOR} --target-env vulkan1.0
                        -S vert ${KSL_VERT_SRC} -o ${KSL_VERT_SPV}
                    DEPENDS ${KSL_VERT_SRC}
                    COMMENT "SPIR-V: scene.vert -> scene.vert.spv"
                )
                list(APPEND KSL_SPV_OUTPUTS ${KSL_VERT_SPV})

                # Compile line shaders (debug draw)
                foreach(LINE_SHADER line.vert line.frag)
                    string(REPLACE "." "_" LINE_VAR ${LINE_SHADER})
                    set(LINE_SRC "${KSL_SHADER_DIR}/${LINE_SHADER}")
                    set(LINE_SPV "${KSL_SPV_DIR}/${LINE_SHADER}.spv")
                    string(REGEX REPLACE ".*\\." "" LINE_STAGE ${LINE_SHADER})
                    add_custom_command(
                        OUTPUT ${LINE_SPV}
                        COMMAND ${GLSLANG_VALIDATOR} --target-env vulkan1.0
                            -S ${LINE_STAGE} ${LINE_SRC} -o ${LINE_SPV}
                        DEPENDS ${LINE_SRC}
                        COMMENT "SPIR-V: ${LINE_SHADER} -> ${LINE_SHADER}.spv"
                    )
                    list(APPEND KSL_SPV_OUTPUTS ${LINE_SPV})
                endforeach()

                # Compile blit shaders (fullscreen composite)
                foreach(BLIT_SHADER blit.vert blit.frag)
                    set(BLIT_SRC "${KSL_SHADER_DIR}/${BLIT_SHADER}")
                    set(BLIT_SPV "${KSL_SPV_DIR}/${BLIT_SHADER}.spv")
                    string(REGEX REPLACE ".*\\." "" BLIT_STAGE ${BLIT_SHADER})
                    add_custom_command(
                        OUTPUT ${BLIT_SPV}
                        COMMAND ${GLSLANG_VALIDATOR} --target-env vulkan1.0
                            -S ${BLIT_STAGE} ${BLIT_SRC} -o ${BLIT_SPV}
                        DEPENDS ${BLIT_SRC}
                        COMMENT "SPIR-V: ${BLIT_SHADER} -> ${BLIT_SHADER}.spv"
                    )
                    list(APPEND KSL_SPV_OUTPUTS ${BLIT_SPV})
                endforeach()

                # Compile UI shaders
                foreach(UI_SHADER ui.vert ui.frag)
                    set(UI_SRC "${KSL_SHADER_DIR}/${UI_SHADER}")
                    set(UI_SPV "${KSL_SPV_DIR}/${UI_SHADER}.spv")
                    string(REGEX REPLACE ".*\\." "" UI_STAGE ${UI_SHADER})
                    add_custom_command(
                        OUTPUT ${UI_SPV}
                        COMMAND ${GLSLANG_VALIDATOR} --target-env vulkan1.0
                            -S ${UI_STAGE} ${UI_SRC} -o ${UI_SPV}
                        DEPENDS ${UI_SRC}
                        COMMENT "SPIR-V: ${UI_SHADER} -> ${UI_SHADER}.spv"
                    )
                    list(APPEND KSL_SPV_OUTPUTS ${UI_SPV})
                endforeach()

                # Generate Vulkan GLSL and compile each to SPIR-V
                foreach(KSL_SRC ${KSL_SHADER_SOURCES})
                    get_filename_component(KSL_NAME ${KSL_SRC} NAME_WLE)
                    string(REPLACE ".ksl" "" KSL_STEM ${KSL_NAME})
                    set(KSL_VK_FRAG "${KSL_SPV_DIR}/${KSL_STEM}.frag")
                    set(KSL_FRAG_SPV "${KSL_SPV_DIR}/${KSL_STEM}.frag.spv")

                    # Generate Vulkan-compatible GLSL
                    add_custom_command(
                        OUTPUT ${KSL_VK_FRAG}
                        COMMAND ${Python3_EXECUTABLE} ${KSL_CODEGEN}
                            --vulkan ${KSL_SRC} -o ${KSL_VK_FRAG}
                        DEPENDS ${KSL_SRC} ${KSL_CODEGEN}
                        COMMENT "KSL: ${KSL_STEM}.ksl.hpp -> ${KSL_STEM}.frag (Vulkan)"
                    )

                    # Compile to SPIR-V
                    add_custom_command(
                        OUTPUT ${KSL_FRAG_SPV}
                        COMMAND ${GLSLANG_VALIDATOR} --target-env vulkan1.0
                            -S frag ${KSL_VK_FRAG} -o ${KSL_FRAG_SPV}
                        DEPENDS ${KSL_VK_FRAG}
                        COMMENT "SPIR-V: ${KSL_STEM}.frag -> ${KSL_STEM}.frag.spv"
                    )

                    list(APPEND KSL_SPV_OUTPUTS ${KSL_FRAG_SPV})
                endforeach()

                add_custom_target(ksl_spirv ALL DEPENDS ${KSL_SPV_OUTPUTS})
                add_dependencies(koiloengine_display ksl_spirv)

                target_compile_definitions(koiloengine_display PUBLIC KL_HAVE_VULKAN_SHADERS)
                message(STATUS "SPIR-V shaders: ${KSL_SPV_DIR}/ (${GLSLANG_VALIDATOR})")
            else()
                message(WARNING "KOILO_USE_VULKAN=ON but no SPIR-V compiler found "
                                "(install glslangValidator or glslc)")
            endif()
        endif()
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
