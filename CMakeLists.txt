cmake_minimum_required(VERSION 3.26)
project(g_app)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_subdirectory(external/glfw)
add_subdirectory(external/spdlog)

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

set(PATH_TO_VULKANSDK C:/VulkanSDK/1.3.275.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(VULKANSDK_INC ${PATH_TO_VULKANSDK}/Include)
set(VULKANSDK_LIB ${PATH_TO_VULKANSDK}/Lib/vulkan-1.lib)

include_directories(
        external/glfw/include
        external/vma/include
        external/spdlog/include
        external/imgui
        external/stb_image
        external/miniaudio
        include
        ${VULKANSDK_INC}
)

file(GLOB SRC include/*.hpp include/**/*.hpp src/*.cpp src/**/*.cpp)
add_library(g_app
        ${SRC}
        src/vma.cpp
        external/stb_image/stb_image.cpp
        include/audio.hpp
        external/miniaudio/miniaudio.cpp
        src/audio.cpp)

    set(BUILD_EXAMPLES ON)

if(${BUILD_EXAMPLES})
    add_subdirectory(examples)
endif ()
