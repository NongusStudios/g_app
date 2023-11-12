cmake_minimum_required(VERSION 3.26)
project(g_app)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_subdirectory(external/glfw)
add_subdirectory(external/spdlog)

set(PATH_TO_VULKANSDK C:/VulkanSDK/1.3.268.0)
set(VULKANSDK_INC ${PATH_TO_VULKANSDK}/Include)

include_directories(
        external/glfw/include
        external/vma/include
        external/spdlog/include
        include
        ${VULKANSDK_INC}
)

add_library(g_app
        src/g_app.cpp
        include/g_app.hpp
        include/types.hpp
        src/vma.cpp
        src/window.cpp
        include/window.hpp
        src/vkgfx/renderer.cpp
        include/vkgfx/renderer.hpp
        include/vkgfx/types.hpp)

add_executable(1_triangle examples/triangle/1_triangle.cpp)
target_link_libraries(1_triangle
        glfw g_app spdlog::spdlog ${VULKANSDK_LIB}
)
