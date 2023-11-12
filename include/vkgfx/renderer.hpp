//
// Created by jandr on 6/11/2023.
//

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <memory>
#include <vector>

#include "types.hpp"

namespace g_app {
    class VulkanRendererInit;

    class VulkanRenderer {
    public:

        VulkanRenderer(const VulkanRenderer&) = default;
        VulkanRenderer& operator = (const VulkanRenderer&) = default;
    private:
        struct Config {
            uint32_t api_version = VK_API_VERSION_1_3;
            uint32_t app_version = VK_MAKE_VERSION(1, 0, 0);
            uint32_t engine_version = VK_MAKE_VERSION(1, 0, 0);
            std::string app_name = "app";
            std::string engine_name = "engine";
            std::vector<const char*> enabled_layers = {};
            std::vector<const char*> enabled_device_extensions = {};

            DisplayMode display_mode = DisplayMode::VSYNC;
            uint32_t    frame_rate_limit = 0; // Leave 0 for unlimited
        };

        struct Inner {
            VkInstance instance;
            VkPhysicalDevice physical_device;
            VkDevice device;

            ~Inner(){
                vkDestroyInstance(instance, nullptr);
            }
        };

        std::shared_ptr<Inner> self;

        VulkanRenderer(GLFWwindow* window, const Config& config);
        void init_instance(const Config& config);
        void pick_physical_device();
        void init_device(const Config& config);

        friend class VulkanRendererInit;
    };

    class VulkanRendererInit {
    public:
        VulkanRendererInit(): m_config{} {}

        VulkanRendererInit& set_api_version(uint32_t version){
            m_config.api_version = version;
            return *this;
        }
        VulkanRendererInit& set_app_version(uint32_t version){
            m_config.app_version = version;
            return *this;
        }
        VulkanRendererInit& set_engine_version(uint32_t version){
            m_config.engine_version = version;
            return *this;
        }
        VulkanRendererInit& set_app_name(const std::string& name){
            m_config.app_name = name;
            return *this;
        }
        VulkanRendererInit& set_engine_name(const std::string& name){
            m_config.engine_name = name;
            return *this;
        }
        VulkanRendererInit& set_display_mode(const DisplayMode& mode){
            m_config.display_mode = mode;
        }
        VulkanRendererInit& set_frame_rate_limit(uint32_t limit){
            m_config.frame_rate_limit = limit;
        }

        VulkanRenderer init(GLFWwindow* window) const { return {window, m_config}; }
    private:
        VulkanRenderer::Config m_config;
    };

} // g_app
