cmake_minimum_required(VERSION 3.26)
project(g_app)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_subdirectory(external/glfw)
add_subdirectory(external/spdlog)

find_package(Vulkan REQUIRED)

# Build ImGui
file(GLOB IMGUI_SRC
    external/imgui/*.h
    external/imgui/*.cpp
)

add_library(imgui
    ${IMGUI_SRC}
    external/imgui/backends/imgui_impl_glfw.h
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_vulkan.h
    external/imgui/backends/imgui_impl_vulkan.cpp
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(
        external/glfw/include
        external/vma/include
        external/spdlog/include
        external/imgui
        external/stb_image
        external/miniaudio
        include
)

file(GLOB SRC include/*.hpp include/**/*.hpp src/*.cpp src/**/*.cpp)
set(CMAKE_CONFIGURE_DEPENDS ${SRC})
add_library(g_app
        ${SRC}
        src/vma.cpp
        external/stb_image/stb_image.cpp
        include/audio.hpp
        external/miniaudio/miniaudio.cpp)

set(BUILD_EXAMPLES OFF)

if(${BUILD_EXAMPLES})
    add_subdirectory(examples)
endif ()
