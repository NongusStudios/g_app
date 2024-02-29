//
// Created by jandr on 29/10/2023.
//

/*
 * This file is a part of the g_app open-source project.
 *
 *  repo: https://github.com/NongusStudios/g_app.git
 *  license: MIT
 *
 *  Copyright (c) 2023 Nongus Studios
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#pragma  once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <string>
#include <utility>
#include <vector>
#include <functional>
#include <stdexcept>
#include <cassert>
#include <algorithm>

#include "types.hpp"
#include "window.hpp"
#include "vkgfx/all.hpp"

namespace g_app {
    struct Time {
        static constexpr double MAX_DELTA = 0.032f;
        double elapsed = 0.0; /* Elapsed time since application start in seconds. */
        float  elapsedf = 0.0f; /* Same as 'elapsed' but as a 32-bit floating point. */
        double delta = 0.0; /* Time between frames */
        float  deltaf = 0.0f; /* Same as 'delta' but as a 32-but floating point. */

        void update(double current_time){
            delta = std::clamp(current_time - elapsed, 0.0, MAX_DELTA);
            elapsed = current_time;
            elapsedf = static_cast<float>(elapsed);
            deltaf = static_cast<float>(delta);
        }
    };

    class AppInit;

    class App {
    public:
        Window& window() {
            return m_window;
        }
        const Window& window() const {
            return m_window;
        }
        Time time() const {
            return m_time;
        }
        VulkanRenderer& renderer() {
            return m_renderer;
        }

        void main_loop(std::function<void(const std::vector<Event>&, const Time &)> f);
    private:
        struct Config {
            Extent2D<uint32_t> window_extent   = {800, 600}; // Size of the window in pixels
            std::string        window_title    = "g_app";                 // Title of the window
            WindowMode         window_mode     = WindowMode::WINDOWED;    // Mode of the window (Windowed, Borderless, Fullscreen)
            bool               resizable       = true;
            bool               primary_monitor = true;                    // Defaults to primary monitor when true
            std::function<Monitor(std::vector<Monitor>)> choose_monitor;
            std::string icon_path; // TODO
            uint32_t sample_count = 1;

            VulkanRendererInit renderer_init = {};
        };

        explicit App(const Config& config);

        friend class AppInit;

        Window         m_window;
        Time           m_time;
        VulkanRenderer m_renderer;
    };

    class AppInit {
    public:
        AppInit(): m_config{} {}

        /* Sets the width and height of the window */
        AppInit& set_window_extent(const Extent2D<uint32_t>& extent){
            m_config.window_extent = extent;
            return *this;
        }

        /* Sets the window title */
        AppInit& set_window_title(const std::string& title){
            m_config.window_title = title;
            return *this;
        }

        /* Sets the window display mode
         * enum class WindowMode options:
         *  - WINDOWED
         *  - BORDERLESS
         *  - EXCLUSIVE */
        AppInit& set_window_mode(const WindowMode& mode){
            m_config.window_mode = mode;
            return *this;
        }

        /* Puts the window on the primary monitor */
        AppInit& use_primary_monitor(){
            m_config.primary_monitor = true;
            return *this;
        }

        /* Puts the window on a monitor of your choice
         * 'f' is called and passed through all the connected monitors.
         * The monitor returned from 'f' will be used. */
        AppInit& use_other_monitor(std::function<Monitor(std::vector<Monitor>)> f){
            m_config.primary_monitor = false;
            m_config.choose_monitor = std::move(f);
            return *this;
        }

        /* Specifies a file path to an image to be used for the window icon. */
        AppInit& set_window_icon(const std::string& path){
            m_config.icon_path = path;
            return *this;
        }

        /* If the window will be resizable or not. */
        AppInit& set_resizable(bool resizable){
            m_config.resizable = resizable;
            return *this;
        }

        /* Used to configure the vulkan renderer. VulkanRendererInit& is edited through a function pointer. */
        AppInit& configure_vulkan_renderer(const std::function<void(VulkanRendererInit&)>& f){
            if(f) f(m_config.renderer_init);
            return *this;
        }

        /* Initialises the app. Should be called once all configurations have been completed. */
        App init() {
            try {
                return App{m_config};
            } catch(const std::runtime_error& e) {
                spdlog::error(e.what());
                std::exit(EXIT_FAILURE);
            }
        }
    private:
        App::Config m_config = {};
    };
};
