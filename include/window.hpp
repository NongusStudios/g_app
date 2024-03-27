//
// Created by jandr on 1/11/2023.
//

#pragma once

#include <string>
#include <functional>
#include <cassert>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "../include/types.hpp"

namespace g_app {
    enum class WindowMode {
        WINDOWED,
        BORDERLESS,
        FULLSCREEN,
    };

    class Monitor {
    public:
        Monitor(): m_monitor{nullptr}, m_idx{0} {}
        explicit Monitor(GLFWmonitor* monitor, size_t idx): m_monitor{monitor}, m_idx{idx}, m_vidmode{glfwGetVideoMode(monitor)} {}

        Monitor(const Monitor& monitor) = default;
        Monitor& operator = (const Monitor& monitor) = default;

        GLFWmonitor* glfw_monitor() const { return m_monitor; }
        const GLFWvidmode* glfw_vidmode() const { return m_vidmode; }

        bool is_primary() const {
            return m_monitor == glfwGetPrimaryMonitor();
        }
        std::string name() const {
            assert(m_monitor && "g_app ERR: m_monitor = null");
            return {glfwGetMonitorName(m_monitor)};
        }
        size_t index() const {
            return m_idx;
        }
        Extent2D<uint32_t> size_mm() const {
            assert(m_monitor && "g_app ERR: m_monitor = null");
            int w, h;
            glfwGetMonitorPhysicalSize(m_monitor, &w, &h);
            return {
                    static_cast<uint32_t>(w),
                    static_cast<uint32_t>(h),
            };
        }
        Extent2D<float> content_scale() const {
            assert(m_monitor && "g_app ERR: m_monitor = null");
            Extent2D<float> scale = {};
            glfwGetMonitorContentScale(m_monitor, &scale.width, &scale.height);
            return scale;
        }
        std::pair<Pos2D<int>, Extent2D<uint32_t>> workarea() const {
            assert(m_monitor && "g_app ERR: m_monitor = null");
            int x,y,w,h;
            glfwGetMonitorWorkarea(m_monitor, &x, &y, &w, &h);
            return {
                    {x, y},
                    {static_cast<uint32_t>(w), static_cast<uint32_t>(h)},
            };
        }
        Pos2D<int> pos() const {
            assert(m_monitor && "g_app ERR: m_monitor = null");
            Pos2D<int> pos = {};
            glfwGetMonitorPos(m_monitor, &pos.xpos, &pos.ypos);
            return pos;
        }

    private:
        GLFWmonitor* m_monitor;
        size_t       m_idx;
        const GLFWvidmode* m_vidmode;
    };

    struct Mods {
        bool ctrl;
        bool shift;
        bool alt;
        bool super;
        bool capslock;
        bool numlock;
    };

    enum class EventType {
        NONE,
        KEY,
        CHAR,
        CURSOR_POSITION,
        CURSOR_ENTER,
        MBUTTON,
        SCROLL,
        FILE_DROP,
    };

    struct KeyEvent {
        Key key;
        Action action;
        Mods mods;
    };
    struct CharEvent {
        uint32_t codepoint;
    };
    struct CursorPositionEvent {
        Pos2D<double> pos;
        Pos2D<float>  posf;
    };
    struct CursorEnterEvent {
        bool entered;
    };
    struct MButtonEvent {
        MouseButton button;
        Action action;
        Mods mods;
    };
    struct ScrollEvent {
        Pos2D<double> offset;
        Pos2D<float>  offsetf;
    };
    struct FileDropEvent {
        const char** paths;
        uint32_t     count;
    };

    struct Event {
        static Event empty() {
            return {EventType::NONE};
        }

        EventType type;
        union {
            KeyEvent key;
            CharEvent character;
            CursorPositionEvent cursor_position;
            CursorEnterEvent cursor_enter;
            MButtonEvent mbutton;
            ScrollEvent scroll;
            FileDropEvent file_drop;
        };
    };

    void g_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void g_char_callback(GLFWwindow* window, uint32_t codepoint);
    void g_cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    void g_cursor_enter_callback(GLFWwindow* window, int entered);
    void g_mbutton_callback(GLFWwindow* window, int button, int action, int mods);
    void g_scroll_callback(GLFWwindow* window, double xoff, double yoff);
    void g_file_drop_callback(GLFWwindow* window, int count, const char** paths);

    class Window {
    public:
        Window(): self{}, m_monitors{}, m_current_events{} {}

        Window(
            Extent2D<uint32_t> window_extent,
            const std::string& window_title,
            WindowMode         window_mode,
            bool               resizable,
            bool               primary_monitor,
            const std::function<Monitor(std::vector<Monitor>)>& choose_monitor
        );

        void set_window_size(const Extent2D<uint32_t>& extent) {
            glfwSetWindowSize(self->window, static_cast<int>(extent.width), static_cast<int>(extent.height));
        }
        void set_window_title(const std::string& title) {
            glfwSetWindowTitle(self->window, title.c_str());
        }

        void quit(){
            glfwSetWindowShouldClose(self->window, true);
        }

        bool is_open() const {
            return !glfwWindowShouldClose(self->window);
        }

        std::vector<Event> poll_events() {
            m_current_events.clear();
            glfwPollEvents();
            return m_current_events;
        }

        std::vector<Monitor> monitors() const {
            return m_monitors;
        }

        Extent2D<uint32_t> extent() const {
            int width, height;
            glfwGetFramebufferSize(self->window, &width, &height);
            return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        }
        Pos2D<double> cursor_position() const {
            double xpos, ypos;
            glfwGetCursorPos(self->window, &xpos, &ypos);
            return {xpos, ypos};
        }
        Pos2D<float> cursor_positionf() const {
            auto curpos = cursor_position();
            return {static_cast<float>(curpos.xpos), static_cast<float>(curpos.ypos)};
        }

        Action key(Key key) const {
            return Action(glfwGetKey(self->window, int(key)));
        }
        Action button(MouseButton button) const {
            return Action(glfwGetMouseButton(self->window, int(button)));
        }

        bool lctrl() const {
            return key(Key::LCTRL) == Action::PRESS;
        }
        bool rctrl() const {
            return key(Key::RCTRL) == Action::PRESS;
        }

        bool lshift() const {
            return key(Key::LSHIFT) == Action::PRESS;
        }
        bool rshift() const {
            return key(Key::RSHIFT) == Action::PRESS;
        }

        bool lalt() const {
            return key(Key::LALT) == Action::PRESS;
        }
        bool ralt() const {
            return key(Key::RALT) == Action::PRESS;
        }

        bool lsuper() const {
            return key(Key::LSUPER) == Action::PRESS;
        }
        bool rsuper() const {
            return key(Key::RSUPER) == Action::PRESS;
        }

        Mods mods() const {
            return {lctrl() || rctrl(), lshift() || rshift(), lalt() || ralt(), lsuper() || rsuper(), false, false };
        }

        GLFWwindow* glfw_window() const { return self->window; }

        static constexpr size_t M_CURRENT_EVENTS_CAPACITY = 128;

        static bool m_instance_exists;
    private:
        struct Inner {
            ~Inner(){
                glfwDestroyWindow(window);
                glfwTerminate();
            }

            GLFWwindow* window;
        };
        std::shared_ptr<Inner> self;        

        std::vector<Monitor> m_monitors;
        std::vector<Event> m_current_events;

        friend void g_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
        friend void g_char_callback(GLFWwindow* window, uint32_t codepoint);
        friend void g_cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
        friend void g_cursor_enter_callback(GLFWwindow* window, int entered);
        friend void g_mbutton_callback(GLFWwindow* window, int button, int action, int mods);
        friend void g_scroll_callback(GLFWwindow* window, double xoff, double yoff);
        friend void g_file_drop_callback(GLFWwindow* window, int count, const char** paths);

        void key_callback(int key, int scancode, int action, int mods);
        void char_callback(uint32_t codepoint);
        void cursor_position_callback(double xpos, double ypos);
        void cursor_enter_callback(int entered);
        void mbutton_callback(int button, int action, int mods);
        void scroll_callback(double xoff, double yoff);
        void file_drop_callback(int count, const char** paths);
    };

} // g_app
