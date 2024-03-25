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

#include "renderer.hpp"

namespace g_app {
    class Semaphore {
    public:
        Semaphore(){}
        Semaphore(VkSemaphore semaphore): self{std::make_shared<Inner>()} { self->semaphore = semaphore; }
        Semaphore(VulkanRenderer renderer, const std::string& label="unnamed semaphore", VkSemaphoreCreateFlags flags = 0): self{std::make_shared<Inner>(renderer)} {
            self->label = label;
            VkSemaphoreCreateInfo create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            create_info.flags = flags;

            VkResult result = VK_SUCCESS;
            if((result = vkCreateSemaphore(self->renderer.inner()->device, &create_info, nullptr, &self->semaphore)) != VK_SUCCESS){
                throw std::runtime_error(
                    std::format("Failed to create a semaphore! label = {}, result = {}", self->label, static_cast<uint32_t>(result) )
                );
            }
        }

        VkSemaphore vk_semaphore() const { return self->semaphore; }
    private:
        struct Inner {
            ~Inner(){
                if(!renderer.is_valid()) return;
                vkDestroySemaphore(renderer.inner()->device, semaphore, nullptr);
            }

            VulkanRenderer renderer;
            VkSemaphore semaphore;
            std::string label;
        };
        std::shared_ptr<Inner> self;
    };

    class Fence {
    public:
        Fence(){}
        Fence(VkFence fence): self{std::make_shared<Inner>()} { self->fence = fence; }
        Fence(VulkanRenderer renderer, const std::string& label = "unnamed fence", VkFenceCreateFlags flags = 0):
            self{std::make_shared<Inner>(renderer)}
        {
            self->label = label;
            VkFenceCreateInfo create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            create_info.flags = flags;

            VkResult result = VK_SUCCESS;
            if((result = vkCreateFence(self->renderer.inner()->device, &create_info, nullptr, &self->fence)) != VK_SUCCESS){
                throw std::runtime_error(
                    std::format("Failed to create a fence! label = {}, result = {}", self->label, static_cast<uint32_t>(result) )
                );
            }
        }
 
        VkFence vk_fence() const { return self->fence; }
    private:
        struct Inner {
            ~Inner(){
                if(!renderer.is_valid()) return;
                vkDestroyFence(renderer.inner()->device, fence, nullptr);
            }

            VulkanRenderer renderer;
            VkFence fence;
            std::string label;
        };

        std::shared_ptr<Inner> self;
    };
}
