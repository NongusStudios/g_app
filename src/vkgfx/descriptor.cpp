//
// Created by jandr on 20/11/2023.
//
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

    DescriptorSet::DescriptorSet(VulkanRenderer renderer, const DescriptorSet::Config &config):
        self{std::make_shared<Inner>(renderer)}
    {
        self->label = config.label;
        self->pool = config.pool;

        auto inner = renderer.inner();

        std::vector<VkDescriptorSetLayout> layouts(config.set_count, config.layout.vk_descriptor_set_layout());
        VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        alloc_info.descriptorPool = config.pool;
        alloc_info.descriptorSetCount = config.set_count;
        alloc_info.pSetLayouts = layouts.data();

        self->sets.resize(config.set_count);

        VkResult result = VK_SUCCESS;
        if((result = vkAllocateDescriptorSets(inner->device, &alloc_info, self->sets.data())) != VK_SUCCESS){
            throw std::runtime_error(
                    std::format(
                            "Failed to create a descriptor set layout! label = {}, result = {}", self->label, static_cast<uint32_t>(result)
                    )
            );
        }
    }
}