//
// Created by jandr on 20/11/2023.
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

#include "../include/vkgfx/descriptor.hpp"

namespace g_app {
    DescriptorPool::DescriptorPool(VulkanRenderer renderer, const Config& config): self{std::make_shared<Inner>(renderer)} {
        self->label = config.label;
        auto inner = renderer.inner();

        VkDescriptorPoolCreateInfo create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        create_info.maxSets = config.max_sets;
        create_info.flags = config.flags;
        create_info.poolSizeCount = config.pool_sizes.size();
        create_info.pPoolSizes = config.pool_sizes.data();

        VkResult result = VK_SUCCESS;
        if((result = vkCreateDescriptorPool(inner->device, &create_info, nullptr, &self->pool)) != VK_SUCCESS){
            throw std::runtime_error(
                    std::format(
                        "Failed to create a descriptor pool! label = {}, result = {}", self->label, static_cast<uint32_t>(result)
                    )
            );
        }
    }

    DescriptorSetLayout::DescriptorSetLayout(VulkanRenderer renderer, const DescriptorSetLayout::Config &config):
        self{std::make_shared<Inner>(renderer)}
    {
        self->label = config.label;

        auto inner = renderer.inner();

        VkDescriptorSetLayoutCreateInfo create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        create_info.flags = config.flags;
        create_info.bindingCount = config.bindings.size();
        create_info.pBindings = config.bindings.data();

        VkResult result = VK_SUCCESS;
        if((result = vkCreateDescriptorSetLayout(inner->device, &create_info, nullptr, &self->layout)) != VK_SUCCESS){
            throw std::runtime_error(
                    std::format(
                            "Failed to create a descriptor set layout! label = {}, result = {}", self->label, static_cast<uint32_t>(result)
                    )
            );
        }
    }

    DescriptorSet::DescriptorSet(const VulkanRenderer& renderer, VkDescriptorSet set, const std::string &label):
        self{std::make_shared<Inner>(renderer)}
    {
        self->label = label;
        self->set = set;
    }
}