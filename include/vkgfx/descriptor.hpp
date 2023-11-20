//
// Created by jandr on 20/11/2023.
//
#pragma once

#include "renderer.hpp"

#include "buffer.hpp"

namespace g_app {
    class DescriptorPoolInit;
    class DescriptorSetLayoutInit;

    class DescriptorSetLayout {
    public:
        DescriptorSetLayout() = default;

        VkDescriptorSetLayout vk_descriptor_set_layout() const { return self->layout; }
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

        std::vector<VkDescriptorSet> vk_descriptor_sets() const { return self->sets; }

        void write_buffer(const std::vector<VkDescriptorBufferInfo>& infos, uint32_t binding, VkDescriptorType type){
            assert(infos.size() == self->sets.size());

            std::vector<VkWriteDescriptorSet> writes = {};
            writes.reserve(self->sets.size());

            uint32_t i = 0;
            for(auto set : self->sets) {
                VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                write.dstSet = set;
                write.dstBinding = binding;
                write.dstArrayElement = 0;
                write.descriptorType = type;
                write.descriptorCount = 1;
                write.pBufferInfo = &infos[i];

                writes.push_back(write);

                i++;
            }

            vkUpdateDescriptorSets(
                    self->renderer.inner()->device, writes.size(), writes.data(),
                    0, nullptr);
        }
    private:
        struct Config {
            VkDescriptorPool pool = VK_NULL_HANDLE;
            DescriptorSetLayout layout;
            uint32_t set_count = 1;

            std::string label = "unnamed descriptor set";
        };

        struct Inner {
            VulkanRenderer renderer;
            VkDescriptorPool pool = VK_NULL_HANDLE;
            std::vector<VkDescriptorSet> sets = {};
            std::string label;
        };

        std::shared_ptr<Inner> self;

        DescriptorSet(VulkanRenderer renderer, const Config& config);

        friend class DescriptorPool;
    };

    class DescriptorPool {
    public:
        DescriptorPool() = default;

        VkDescriptorPool vk_descriptor_pool() const { return self->pool; }

        DescriptorSet allocate_set(const DescriptorSetLayout& layout, uint32_t set_count){
            try {
                return {self->renderer, {self->pool, layout, set_count, std::format("Descriptor Set: pool = {}", self->label)} };
            } catch(const std::runtime_error& e) {
                spdlog::error(e.what());
                std::exit(EXIT_FAILURE);
            }
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
}