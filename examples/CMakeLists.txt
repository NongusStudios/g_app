if(WIN32)
    set(PATH_TO_VULKANSDK C:/VulkanSDK/1.3.275.0)
    set(VULKANSDK_INC ${PATH_TO_VULKANSDK}/Include)
    if(WIN32)
        set(VULKANSDK_LIB ${PATH_TO_VULKANSDK}/Lib/vulkan-1.lib)
    else()
        set(VULKANSDK_LIB vulkan)
    endif()
else()
    set(VULKANSDK_LIB vulkan)
endif()

include_directories(glm)

file(GLOB SRCS
    **/*.cpp
)

foreach(SRC ${SRCS})
    get_filename_component(EXEC ${SRC} NAME_WLE)
    add_executable(${EXEC} ${SRC})
    target_link_libraries(${EXEC}
            g_app glfw spdlog::spdlog ${VULKANSDK_LIB} imgui
    )
endforeach ()

file(GLOB GLSL_FILES
        **/*.vert
        **/*.frag
        **/*.comp
)

foreach(GLSL_FILE ${GLSL_FILES})
    get_filename_component(FILE_NAME ${GLSL_FILE} NAME)
    get_filename_component(FILE_PATH ${GLSL_FILE} DIRECTORY)
    set(SPIRV "${FILE_PATH}/${FILE_NAME}.spv")
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND glslc -o ${SPIRV} ${GLSL_FILE}
            DEPENDS ${GLSL_FILE}
    )
    list(APPEND SPIRV_FILES ${SPIRV})
endforeach(GLSL_FILE)

add_custom_target(shaders ALL
        DEPENDS ${SPIRV_FILES})
