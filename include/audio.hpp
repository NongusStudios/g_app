//
// Created by jandr on 29/02/2024.
//
#pragma once

#include <string>

#include "miniaudio.h"

namespace g_app {
    class AudioEngine {
    public:
        AudioEngine();

        void play_sound(const std::string& path);
    private:
        ma_engine engine;
    };
}