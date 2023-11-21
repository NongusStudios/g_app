//
// Created by jandr on 29/10/2023.
//

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
    void App::main_loop(std::function<void(const Time &)> f){
        try {
            while (m_window.is_open()) {
                auto events = m_window.poll_events();
                m_time.update(glfwGetTime());
                f(m_time);
            }
        } catch(const std::runtime_error& e) {
            spdlog::error(e.what());
            std::exit(EXIT_FAILURE);
        }
    }
}
