//
// Created by jandr on 29/10/2023.
//
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

#include "types.hpp"
#include "window.hpp"
#include "vkgfx/renderer.hpp"


namespace g_app {
    struct Time {
        double elapsed = 0.0;
        float  elapsedf = 0.0f;
        double delta = 0.0;
        float  deltaf = 0.0f;

        void update(double current_time){
            delta = current_time - elapsed;
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

        void main_loop(std::function<void()> f);
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

        AppInit& set_window_extent(const Extent2D<uint32_t>& extent){
            m_config.window_extent = extent;
            return *this;
        }
        AppInit& set_window_title(const std::string& title){
            m_config.window_title = title;
            return *this;
        }
        AppInit& set_window_mode(const WindowMode& mode){
            m_config.window_mode = mode;
            return *this;
        }
        AppInit& use_primary_monitor(){
            m_config.primary_monitor = true;
            return *this;
        }
        AppInit& use_other_monitor(std::function<Monitor(std::vector<Monitor>)> f){
            m_config.primary_monitor = false;
            m_config.choose_monitor = std::move(f);
            return *this;
        }
        AppInit& set_window_icon(const std::string& path){
            m_config.icon_path = path;
            return *this;
        }
        AppInit& set_resizable(bool resizable){
            m_config.resizable = resizable;
            return *this;
        }
        AppInit& configure_vulkan_renderer(const std::function<void(VulkanRendererInit&)>& f){
            if(f) f(m_config.renderer_init);
            return *this;
        }

        App init() {
            try {
                return App{m_config};
            } catch(const std::runtime_error& e){
                spdlog::error(e.what());
                std::abort();
            }
        }
    private:
        App::Config m_config = {};
    };
};
