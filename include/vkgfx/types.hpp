//
// Created by jandr on 6/11/2023.
//

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace g_app {
    enum class DisplayMode {
        FIFO,
        VSYNC,
    };
}
