//
// Created by jandr on 18/11/2023.
//
#include "../include/vkgfx/image.hpp"

#include <stdexcept>
#include <format>
#include <cassert>

g_app::Image::Image(g_app::VulkanRenderer renderer, const g_app::Image::Config &config): self{std::make_shared<Inner>(renderer)} {
    self->label = config.label;
    self->format = config.format;
    self->mip_levels = config.mip_levels;
    self->layer_count = config.array_layers;

    VkImageCreateInfo create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    create_info.imageType = config.image_type;
    create_info.extent = config.extent;
    create_info.mipLevels = config.mip_levels;
    create_info.arrayLayers = config.array_layers;
    create_info.format = config.format;
    create_info.tiling = config.tiling;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.usage = config.usage;
    create_info.flags = config.flags;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.samples = config.samples;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = config.memory_usage;

    VkResult result = VK_SUCCESS;
    if((result =
            vmaCreateImage(renderer.inner()->allocator, &create_info, &alloc_info,
                           &self->image, &self->allocation, nullptr)) != VK_SUCCESS)
    {
        throw std::runtime_error(
                std::format("Failed to create an image! label = {}, result = {}", self->label, static_cast<uint32_t>(result) )
        );
    }
}

g_app::ImageView::ImageView(const g_app::VulkanRenderer& renderer, const g_app::ImageView::Config &config): self{std::make_shared<Inner>(renderer)} {
    assert(config.image.vk_image() != VK_NULL_HANDLE);

    self->label = config.label;

    VkImageViewCreateInfo create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

    create_info.image = config.image.vk_image();
    create_info.viewType = config.view_type;
    create_info.format = config.image.format();
    create_info.subresourceRange = {
        config.aspect_mask,
        0, config.image.mip_levels(),
        0, config.image.layer_count(),
    };

    VkResult result = VK_SUCCESS;
    if((result =
        vkCreateImageView(self->renderer.inner()->device, &create_info, nullptr, &self->view)) != VK_SUCCESS)
    {
        throw std::runtime_error(
                std::format("Failed to create an image view! label = {}, result = {}", self->label, static_cast<uint32_t>(result) )
        );
    }
}
