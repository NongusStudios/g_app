//
// Created by jandr on 1/11/2023.
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

#include "../include/window.hpp"

#include <spdlog/spdlog.h>

#include <cassert>

namespace g_app {
    bool Window::m_instance_exists = false;

    void g_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->key_callback(key, scancode, action, mods);
    }
    void g_char_callback(GLFWwindow* window, uint32_t codepoint){
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->char_callback(codepoint);
    }
    void g_cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->cursor_position_callback(xpos, ypos);
    }
    void g_cursor_enter_callback(GLFWwindow* window, int entered){
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->cursor_enter_callback(entered);
    }
    void g_mbutton_callback(GLFWwindow* window, int button, int action, int mods){
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->mbutton_callback(button, action, mods);
    }
    void g_scroll_callback(GLFWwindow* window, double xoff, double yoff){
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->scroll_callback(xoff, yoff);
    }
    void g_file_drop_callback(GLFWwindow* window, int count, const char** paths){
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->file_drop_callback(count, paths);
    }

    Window::Window(
            Extent2D<uint32_t> window_extent,
            const std::string& window_title,
            WindowMode         window_mode,
            bool               resizable,
            bool               primary_monitor,
            const std::function<Monitor(std::vector<Monitor>)>& choose_monitor
    ): m_window{nullptr}, m_monitors{}, m_current_events{} {
        if(m_instance_exists) {
            spdlog::warn("Trying to create another window, when one already exists!");
            return;
        }

        if(!glfwInit()){
            throw std::runtime_error("Failed to load GLFW3.");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, resizable);

        // Retrieve Monitors
        int mon_count = 0;
        GLFWmonitor** mons = glfwGetMonitors(&mon_count);

        m_monitors.reserve(mon_count);
        for(int i = 0; i < mon_count; i++){
            m_monitors.emplace_back(mons[i], i);
        }

        Monitor monitor = {};
        if(primary_monitor || !choose_monitor)
            monitor = Monitor(glfwGetPrimaryMonitor(), 0);
        else if(choose_monitor)
            monitor = choose_monitor(m_monitors);

        switch(window_mode){
            case WindowMode::WINDOWED:
                m_window = glfwCreateWindow(
                    static_cast<int>(window_extent.width), static_cast<int>(window_extent.height),
                    window_title.c_str(), nullptr, nullptr
                );
                break;
            case WindowMode::BORDERLESS: {
                const GLFWvidmode* vidmode = monitor.glfw_vidmode();
                glfwWindowHint(GLFW_RED_BITS, vidmode->redBits);
                glfwWindowHint(GLFW_GREEN_BITS, vidmode->greenBits);
                glfwWindowHint(GLFW_BLUE_BITS, vidmode->blueBits);
                glfwWindowHint(GLFW_REFRESH_RATE, vidmode->refreshRate);

                m_window = glfwCreateWindow(
                    vidmode->width, vidmode->height,
                    window_title.c_str(), monitor.glfw_monitor(),
                    nullptr
                );
                break;
            }
            case WindowMode::FULLSCREEN:
                m_window = glfwCreateWindow(
                    static_cast<int>(window_extent.width), static_cast<int>(window_extent.height),
                    window_title.c_str(), monitor.glfw_monitor(), nullptr
                );
                break;
        }

        if(m_window == nullptr){
            throw std::runtime_error("Window creation failed!");
        }

        glfwSetWindowUserPointer(m_window, this);

        m_current_events.reserve(M_CURRENT_EVENTS_CAPACITY);
        glfwSetKeyCallback(m_window, g_key_callback);
        glfwSetCharCallback(m_window, g_char_callback);
        glfwSetCursorPosCallback(m_window, g_cursor_position_callback);
        glfwSetCursorEnterCallback(m_window, g_cursor_enter_callback);
        glfwSetMouseButtonCallback(m_window, g_mbutton_callback);
        glfwSetScrollCallback(m_window, g_scroll_callback);
        glfwSetDropCallback(m_window, g_file_drop_callback);

        m_instance_exists = true;
    }

    Mods glfw_mods2mods(int mods){
        Mods m = {};
        if(mods & GLFW_MOD_SHIFT) m.shift = true;
        if(mods & GLFW_MOD_CONTROL) m.ctrl = true;
        if(mods & GLFW_MOD_ALT) m.alt = true;
        if(mods & GLFW_MOD_SUPER) m.super = true;
        if(mods & GLFW_MOD_CAPS_LOCK) m.capslock = true;
        if(mods & GLFW_MOD_NUM_LOCK) m.numlock = true;
        return m;
    }

    void Window::key_callback(int key, int scancode, int action, int mods){
        Event e = {EventType::KEY};
        e.key.key = key;
        e.key.action = action;
        e.key.mods = glfw_mods2mods(mods);
        m_current_events.push_back(e);
    }
    void Window::char_callback(uint32_t codepoint){
        Event e = {EventType::CHAR};
        e.character.codepoint = codepoint;
        m_current_events.push_back(e);
    }
    void Window::cursor_position_callback(double xpos, double ypos){
        Event e = {EventType::CURSOR_POSITION};
        e.cursor_position.pos = {xpos, ypos};
        e.cursor_position.posf = {static_cast<float>(xpos), static_cast<float>(ypos)};
        m_current_events.push_back(e);
    }
    void Window::cursor_enter_callback(int entered){
        Event e = {EventType::CURSOR_ENTER};
        e.cursor_enter.entered = entered;
        m_current_events.push_back(e);
    }
    void Window::mbutton_callback(int button, int action, int mods){
        Event e = {EventType::MBUTTON};
        e.mbutton.button = button;
        e.mbutton.action = action;
        e.mbutton.mods = glfw_mods2mods(mods);
        m_current_events.push_back(e);
    }
    void Window::scroll_callback(double xoff, double yoff){
        Event e = {EventType::SCROLL};
        e.scroll.offset =  {xoff, yoff};
        e.scroll.offsetf = {static_cast<float>(xoff), static_cast<float>(yoff)};
        m_current_events.push_back(e);
    }
    void Window::file_drop_callback(int count, const char** paths){
        Event e = {EventType::FILE_DROP};
        e.file_drop.paths = paths;
        e.file_drop.count = count;
        m_current_events.push_back(e);
    }
} // g_app