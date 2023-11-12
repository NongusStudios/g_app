set(PATH_TO_VULKANSDK C:/VulkanSDK/1.3.268.0)
set(VULKANSDK_LIB ${PATH_TO_VULKANSDK}/Lib/vulkan-1.lib)

file(GLOB EXAMPLES **/*.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
include_directories(${CMAKE_SOURCE_DIR}/include)



#foreach (EXAMPLE ${EXAMPLES})
#    get_filename_component(FILE_NAME ${EXAMPLE} NAME)
#    add_executable(${FILE_NAME} ${EXAMPLE})

#    target_link_libraries(${FILE_NAME} glfw g_app ${VULKANSDK_LIB} spdlog)
#endforeach (EXAMPLE)