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

#pragma once

#include "renderer.hpp"
#include "image.hpp"
#include "buffer.hpp"

namespace g_app {
    class DescriptorPoolInit;
    class DescriptorSetLayoutInit;

    class DescriptorSetLayout {
    public:
        DescriptorSetLayout() = default;

        VkDescriptorSetLayout vk_descriptor_set_layout() const { return (self) ? self->layout : VK_NULL_HANDLE; }
    private:
        struct Config {
            std::vector<VkDescriptorSetLayoutBinding> bindings = {};
            VkDescriptorSetLayoutCreateFlags flags = 0;
            std::string label = "unnamed descriptor layout";
        };

        struct Inner {
            VulkanRenderer renderer;
            VkDescriptorSetLayout layout = VK_NULL_HANDLE;
            std::string label;

            ~Inner(){
                if(!renderer.is_valid()) return;

                vkDestroyDescriptorSetLayout(renderer.inner()->device, layout, nullptr);
            }
        };

        std::shared_ptr<Inner> self;

        DescriptorSetLayout(VulkanRenderer renderer, const Config& config);

        friend class DescriptorSetLayoutInit;
    };

    class DescriptorSetLayoutInit {
    public:
        DescriptorSetLayoutInit() = default;

        DescriptorSetLayoutInit& set_label(const std::string& label){
            m_config.label = label;
            return *this;
        }

        DescriptorSetLayoutInit& set_flags(VkDescriptorSetLayoutCreateFlags flags){
            m_config.flags = flags;
            return *this;
        }

        DescriptorSetLayoutInit& add_binding(uint32_t binding, VkDescriptorType type,
                                             uint32_t descriptor_count, VkShaderStageFlags stage){
            VkDescriptorSetLayoutBinding layout_binding = {};
            layout_binding.binding = binding;
            layout_binding.descriptorType = type;
            layout_binding.descriptorCount = descriptor_count;
            layout_binding.stageFlags = stage;
            m_config.bindings.push_back(layout_binding);

            return *this;
        }

        DescriptorSetLayout init(const VulkanRenderer& renderer){
            try {
                return {renderer, m_config};
            } catch(const std::runtime_error& e) {
                spdlog::error(e.what());
                std::exit(EXIT_FAILURE);
            }
        }
    private:
        DescriptorSetLayout::Config m_config = {};
    };

    class DescriptorPool;
    class DescriptorSet {
    public:
        DescriptorSet() = default;

        VkDescriptorSet vk_descriptor_set() const { return (self) ? self->set : VK_NULL_HANDLE; }

    private:
        struct Inner {
            VulkanRenderer renderer;
            VkDescriptorSet set = VK_NULL_HANDLE;
            std::string label;
        };

        std::shared_ptr<Inner> self;

        DescriptorSet(const VulkanRenderer& renderer, VkDescriptorSet set, const std::string& label);

        friend class DescriptorPool;
    };

    class DescriptorPool {
    public:
        DescriptorPool() = default;

        VkDescriptorPool vk_descriptor_pool() const { return (self) ? self->pool : VK_NULL_HANDLE; }

        DescriptorSet allocate_set(const DescriptorSetLayout& layout){
            try {
                return allocate_sets({layout})[0];
            } catch(const std::runtime_error& e) {
                spdlog::error(e.what());
                std::exit(EXIT_FAILURE);
            }
        }

        std::vector<DescriptorSet> allocate_sets(const std::vector<DescriptorSetLayout>& layouts){
            auto inner = self->renderer.inner();

            std::vector<VkDescriptorSetLayout> vk_layouts = {};

            vk_layouts.reserve(layouts.size());
            for(const auto& layout : layouts){
                vk_layouts.push_back(layout.vk_descriptor_set_layout());
            }

            VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            alloc_info.descriptorPool = self->pool;
            alloc_info.descriptorSetCount = vk_layouts.size();
            alloc_info.pSetLayouts = vk_layouts.data();

            std::vector<VkDescriptorSet> vk_sets(vk_layouts.size());

            VkResult result = VK_SUCCESS;
            if((result = vkAllocateDescriptorSets(inner->device, &alloc_info, vk_sets.data())) != VK_SUCCESS){
                throw std::runtime_error(
                        std::format(
                                "Failed to create a descriptor set layout! label = {}, result = {}", self->label, static_cast<uint32_t>(result)
                        )
                );
            }

            std::vector<DescriptorSet> sets;
            sets.reserve(vk_sets.size());
            for(auto set : vk_sets){
                sets.push_back({self->renderer, set, std::format("Descriptor Set: pool = {}", self->label)});
            }

            return sets;
        }
    private:
        struct Config {
            std::vector<VkDescriptorPoolSize> pool_sizes = {};
            uint32_t max_sets = 1000;
            VkDescriptorPoolCreateFlags flags = 0;
            std::string label = "unnamed descriptor pool";
        };

        struct Inner {
            VulkanRenderer renderer;
            VkDescriptorPool pool = VK_NULL_HANDLE;
            std::string label;

            ~Inner(){
                if(!renderer.is_valid()) return;

                vkDestroyDescriptorPool(renderer.inner()->device, pool, nullptr);
            }
        };

        std::shared_ptr<Inner> self;

        DescriptorPool(VulkanRenderer renderer, const Config& config);

        friend class DescriptorPoolInit;
    };

    class DescriptorPoolInit {
    public:
        DescriptorPoolInit() = default;

        DescriptorPoolInit& set_label(const std::string& label){
            m_config.label = label;
            return *this;
        }

        DescriptorPoolInit& set_max_sets(uint32_t max){
            m_config.max_sets = max;
            return *this;
        }

        DescriptorPoolInit& set_flags(VkDescriptorPoolCreateFlags flags){
            m_config.flags = flags;
            return *this;
        }

        DescriptorPoolInit& add_pool_size(VkDescriptorType type, uint32_t descriptor_count){
            m_config.pool_sizes.emplace_back(type, descriptor_count);
            return *this;
        }

        DescriptorPool init(const VulkanRenderer& renderer){
            try {
                return {renderer, m_config};
            } catch(const std::runtime_error& e) {
                spdlog::error(e.what());
                std::exit(EXIT_FAILURE);
            }
        }
    private:
        DescriptorPool::Config m_config = {};
    };
    
    class DescriptorWriter {
    public:
        DescriptorWriter() = default;

        template<typename T>
        DescriptorWriter& write_buffer(const DescriptorSet& dst, uint32_t binding, VkDescriptorType type,
                                       const Buffer<T>& buffer, VkDeviceSize offset=0){
            m_buffer_infos.push_back(std::make_shared<VkDescriptorBufferInfo>());
            auto& info = m_buffer_infos[m_buffer_infos.size()-1];
            info->buffer = buffer.vk_buffer();
            info->offset = offset;
            info->range = buffer.size() * sizeof(T);

            VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet = dst.vk_descriptor_set();
            write.dstBinding = binding;
            write.dstArrayElement = 0;
            write.descriptorType = type;
            write.descriptorCount = 1;
            write.pBufferInfo = info.get();

            m_writes.push_back(write);

            return *this;
        }

        DescriptorWriter& write_image(
                const DescriptorSet& dst, uint32_t binding, VkDescriptorType type,
                const ImageView& image_view, const Sampler& sampler, VkImageLayout layout){
            m_image_infos.push_back(std::make_shared<VkDescriptorImageInfo>());
            auto& info = m_image_infos[m_image_infos.size()-1];
            info->imageView = image_view.vk_image_view();
            info->sampler = sampler.vk_sampler();
            info->imageLayout = layout;

            VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet = dst.vk_descriptor_set();
            write.dstBinding = binding;
            write.dstArrayElement = 0;
            write.descriptorType = type;
            write.descriptorCount = 1;
            write.pImageInfo = info.get();

            m_writes.push_back(write);

            return *this;
        }

        DescriptorWriter& copy_descriptor(const DescriptorSet& dst, uint32_t dst_binding, const DescriptorSet& src, uint32_t src_binding){
            VkCopyDescriptorSet copy = {VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET};
            copy.dstSet = dst.vk_descriptor_set();
            copy.srcSet = src.vk_descriptor_set();
            copy.dstBinding = dst_binding;
            copy.dstArrayElement = 0;
            copy.srcBinding = src_binding;
            copy.srcArrayElement = 0;
            copy.descriptorCount = 1;

            m_copies.push_back(copy);

            return *this;
        }

        void commit_writes(VulkanRenderer renderer){
            vkUpdateDescriptorSets(
                    renderer.inner()->device, m_writes.size(), m_writes.data(),
                    m_copies.size(), m_copies.data());
        }
        
        std::vector<VkWriteDescriptorSet>& get_writes(){ return m_writes; }
    private:
        std::vector<std::shared_ptr<VkDescriptorBufferInfo>> m_buffer_infos = {};
        std::vector<std::shared_ptr<VkDescriptorImageInfo>> m_image_infos = {};
        std::vector<VkWriteDescriptorSet> m_writes = {};
        std::vector<VkCopyDescriptorSet> m_copies = {};
    };


}
