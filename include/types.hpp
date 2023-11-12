//
// Created by jandr on 29/10/2023.
//

#pragma once

#include <cstdint>

namespace g_app {
    template<typename T>
    struct Extent2D {
        T width, height;
    };
    template<typename T>
    struct Pos2D {
        T xpos, ypos;
    };
}
