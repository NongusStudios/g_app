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

#include "../include/g_app.hpp"

namespace g_app {
    App::App(const Config& config):
        m_window{config.window_extent,
                 config.window_title,
                 config.window_mode,
                 config.resizable,
                 config.primary_monitor,
                 config.choose_monitor
        },
        m_renderer{config.renderer_init.init(m_window.glfw_window())}
    {

    }
    void App::main_loop(std::function<void(const std::vector<Event>&, const Time &)> f){
        try {
            while (m_window.is_open()) {
                auto events = m_window.poll_events();
                m_time.update(glfwGetTime());
                f(events, m_time);
            }
        } catch(const std::runtime_error& e) {
            spdlog::error(e.what());
            std::exit(EXIT_FAILURE);
        }
    }
}
