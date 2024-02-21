//
// Created by jeol on 21/02/24.
//
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>
#include <string>

#include "renderer.hpp"

namespace g_app {
    class PipelineCache {
    public:
        PipelineCache() = default;

        VkPipelineCache vk_pipeline_cache() const { return self->cache; }

        bool serialize(const std::string& path);
        static PipelineCache load(VulkanRenderer renderer, const std::string& path);
    private:
        PipelineCache(VulkanRenderer renderer, void* prev_data, size_t size, const std::string& label="unnamed pipeline cache");
        PipelineCache(VulkanRenderer renderer, const std::string& label="unnamed pipeline cache");

        struct Inner {
            VulkanRenderer renderer;
            VkPipelineCache cache = VK_NULL_HANDLE;
            std::string label;

            ~Inner(){
                vkDestroyPipelineCache(renderer.inner()->device, cache, nullptr);
            }
        };
        std::shared_ptr<Inner> self;
    };
}