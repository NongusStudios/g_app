//
// Created by jandr on 6/11/2023.
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

#pragma once

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

#include <spdlog/spdlog.h>

#include <string>
#include <memory>
#include <vector>

#include "types.hpp"

namespace g_app {
    enum class Queue {
        TRANSFER = 0,
        COMPUTE = 1,
        GRAPHICS = 2,
        MAX,
    };

    struct QueueFamilyInfo {
        uint32_t index = 0;
        uint32_t count = 0;
    };

    class VulkanRendererInit;
    class RenderPass;

    class VulkanRenderer {
    public:
        static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
        static constexpr VkFormat TARGET_SWAPCHAIN_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;

        struct SwapchainDepthResources {
            std::vector<VkImage> images = {};
            std::vector<VmaAllocation> allocations = {};
            std::vector<VkImageView> image_views = {};
            VkFormat format;
        };

        struct Swapchain {
            VkSwapchainKHR swapchain = VK_NULL_HANDLE;
            std::vector<VkImage> images = {};
            std::vector<VkImageView> image_views = {};
            SwapchainDepthResources depth_resources = {};
            std::vector<VkFramebuffer> framebuffers = {};

            VkFormat format;
            VkExtent2D extent;
            uint32_t min_image_count;

            void destroy(VkDevice device, VmaAllocator allocator){
                for(uint32_t i = 0; i < images.size(); i++){
                    vkDestroyFramebuffer(device, framebuffers[i], nullptr);
                    vmaDestroyImage(allocator, depth_resources.images[i], depth_resources.allocations[i]);
                    vkDestroyImageView(device, depth_resources.image_views[i], nullptr);
                    vkDestroyImageView(device, image_views[i], nullptr);
                }
                vkDestroySwapchainKHR(device, swapchain, nullptr);
            }
        };

        struct Inner {
            GLFWwindow* window = nullptr;
            VkInstance instance = VK_NULL_HANDLE;
            VkSurfaceKHR surface = VK_NULL_HANDLE;
            VkPhysicalDevice physical_device = VK_NULL_HANDLE;
            VkDevice device = VK_NULL_HANDLE;
            std::vector<VkQueue> queues = {};
            QueueFamilyInfo queue_family_info = {};
            VmaAllocator allocator = VK_NULL_HANDLE;
            VkCommandPool command_pool = VK_NULL_HANDLE;
            Swapchain swapchain = {};
            VkRenderPass default_render_pass = VK_NULL_HANDLE;
            std::vector<VkSemaphore> image_available_semaphores = {};
            std::vector<VkSemaphore> render_finished_semaphores = {};
            std::vector<VkFence> in_flight_fences = {};
            VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
            bool cleanup_imgui = false;
            std::unordered_map<std::string, PFN_vkVoidFunction> ext_pfn = {};

            uint32_t current_frame = 0;
            uint32_t current_image = 0;

            ~Inner(){
                if(cleanup_imgui){
                    ImGui_ImplVulkan_Shutdown();
                    ImGui_ImplGlfw_Shutdown();
                    ImGui::DestroyContext();
                    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
                }


                for(uint32_t i = 0; i < image_available_semaphores.size(); i++) {
                    vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
                    vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
                    vkDestroyFence(device, in_flight_fences[i], nullptr);
                }
                vkDestroyRenderPass(device, default_render_pass, nullptr);
                swapchain.destroy(device, allocator);
                vkDestroyCommandPool(device, command_pool, nullptr);
                vmaDestroyAllocator(allocator);
                vkDestroyDevice(device, nullptr);
                vkDestroySurfaceKHR(instance, surface, nullptr);
                vkDestroyInstance(instance, nullptr);
            }
        };

        VulkanRenderer() = default;
        VulkanRenderer(const VulkanRenderer&) = default;
        VulkanRenderer& operator = (const VulkanRenderer&) = default;

        bool acquire_next_swapchain_image();
        void begin_default_render_pass(VkCommandBuffer cmd, float r, float g, float b, float a);
        void present();
        
        template<typename T>
        T get_extpfn(const std::string& name){
            assert(self->ext_pfn.contains(name));
            return reinterpret_cast<T>(self->ext_pfn[name]);
        }

        ImGuiIO& init_imgui();
        void render_imgui(VkCommandBuffer cmd){
            ImGui::Render();
            ImDrawData* draw_data = ImGui::GetDrawData();
            ImGui_ImplVulkan_RenderDrawData(draw_data, cmd);
        }

        VkFormat chosen_swapchain_format() const { return self->swapchain.format; }

        uint32_t current_frame() const { return self->current_frame; }
        uint32_t current_image() const { return self->current_image; }

        bool is_valid() const { return self != nullptr; }

        VkSemaphore current_image_available_semaphore() const { return self->image_available_semaphores[current_frame()]; }
        VkSemaphore current_render_finished_semaphore() const { return self->render_finished_semaphores[current_frame()]; }
        VkFence current_in_flight_fence() const { return self->in_flight_fences[current_frame()]; }

        VkRenderPass default_render_pass() const { return self->default_render_pass; }

        VkPhysicalDeviceProperties physical_device_properties() const {
            VkPhysicalDeviceProperties properties = {};
            vkGetPhysicalDeviceProperties(self->physical_device, &properties);
            return properties;
        }
        VkPhysicalDeviceFeatures physical_device_features() const {
            VkPhysicalDeviceFeatures features = {};
            vkGetPhysicalDeviceFeatures(self->physical_device, &features);
            return features;
        }
        VkQueue get_queue(Queue queue) const {
            if(static_cast<size_t>(queue) >= self->queues.size()){
                return self->queues[std::clamp<size_t>(static_cast<size_t>(queue), 0, self->queues.size()-1)];
            }
            return self->queues[static_cast<uint32_t>(queue)];
        }

        void device_wait_idle(){
            vkDeviceWaitIdle(self->device);
        }

        std::shared_ptr<Inner> inner() { return self; }

        static constexpr uint32_t MAX_QUEUE_COUNT = 3;
    private:
        struct Config {
            uint32_t api_version = VK_API_VERSION_1_3;
            uint32_t app_version = VK_MAKE_VERSION(1, 0, 0);
            uint32_t engine_version = VK_MAKE_VERSION(1, 0, 0);
            std::string app_name = "app";
            std::string engine_name = "engine";
            std::vector<const char*> enabled_layers = {};
            std::vector<const char*> enabled_device_extensions = {};
            VkPhysicalDeviceFeatures enabled_features = {};
            uint32_t    frame_rate_limit = 0; // Leave 0 for unlimited
            std::vector<const char*> pfnload = {};
        };

        /* All vulkan object abstractions are contained within a shared_ptr to allow for easy copying without worrying about
         * resources being deallocated when they shouldn't be. */
        std::shared_ptr<Inner> self;

        VulkanRenderer(GLFWwindow* window, const Config& config);
        void init_instance(const Config& config);
        void init_surface();
        void pick_physical_device(const VulkanRenderer::Config &config);
        void init_device(const Config& config);
        void load_extensions(const Config& config);
        void init_allocator(const Config& config);
        void init_command_pool();
        void init_swapchain(VkSwapchainKHR old_swapchain=VK_NULL_HANDLE);
        void init_default_render_pass();
        void init_framebuffers();
        void init_sync_objects();
        void init_descriptor_pool();
        void recreate_swapchain();

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
        VulkanRendererInit& set_enabled_layers(const std::vector<const char*>& layers){
            m_config.enabled_layers = layers;
            return *this;
        }
        VulkanRendererInit& set_enabled_device_extensions(const std::vector<const char*>& extensions){
            m_config.enabled_device_extensions = extensions;
            return *this;
        }
        VulkanRendererInit& set_enabled_features(const VkPhysicalDeviceFeatures& features){
            m_config.enabled_features = features;
            return *this;
        }
        VulkanRendererInit& set_frame_rate_limit(uint32_t limit){
            m_config.frame_rate_limit = limit;
            return *this;
        }
        VulkanRendererInit& load_vkpfn(const std::vector<const char*>& names){
            m_config.pfnload = names;
            return *this;
        }

        VulkanRenderer init(GLFWwindow* window) const {
            try {
                return {window, m_config};
            } catch(const std::runtime_error& e) {
                spdlog::error(e.what());
                std::exit(EXIT_FAILURE);
            }
        }
    private:
        VulkanRenderer::Config m_config;
    };
} // g_app
