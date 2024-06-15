include_directories(glm)

file(GLOB SRCS
    **/*.cpp
)

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

foreach(SRC ${SRCS})
    get_filename_component(EXEC ${SRC} NAME_WLE)
    add_executable(${EXEC} ${SRC})
    target_link_libraries(${EXEC}
            g_app glfw spdlog::spdlog Vulkan::Vulkan imgui
    )
endforeach ()
