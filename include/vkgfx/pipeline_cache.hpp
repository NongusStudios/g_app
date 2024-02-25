//
// Created by jeol on 21/02/24.
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
                if(!renderer.is_valid()) return;

                vkDestroyPipelineCache(renderer.inner()->device, cache, nullptr);
            }
        };
        std::shared_ptr<Inner> self;
    };
}