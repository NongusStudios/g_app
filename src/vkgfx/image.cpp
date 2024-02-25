//
// Created by jandr on 18/11/2023.
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

#include "../include/vkgfx/image.hpp"

#include <stdexcept>
#include <format>
#include <cassert>

g_app::Image::Image(g_app::VulkanRenderer renderer, const g_app::Image::Config &config): self{std::make_shared<Inner>(renderer)} {
    self->label = config.label;
    self->format = config.format;
    self->mip_levels = config.mip_levels;
    self->layer_count = config.array_layers;
    self->extent = config.extent;

    VkImageCreateInfo create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    create_info.imageType = config.image_type;
    create_info.extent = config.extent;
    create_info.mipLevels = config.mip_levels;
    create_info.arrayLayers = config.array_layers;
    create_info.format = config.format;
    create_info.tiling = config.tiling;
    create_info.initialLayout = config.initial_layout;
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

    VkResult result = vkCreateImageView(self->renderer.inner()->device, &create_info, nullptr, &self->view);
    if(result != VK_SUCCESS){
        throw std::runtime_error(
                std::format("Failed to create an image view! label = {}, result = {}", self->label, static_cast<uint32_t>(result) )
        );
    }
}

g_app::Sampler::Sampler(const g_app::VulkanRenderer &renderer, const g_app::Sampler::Config &config):
    self{std::make_shared<Inner>(renderer)}
{
    self->label = config.label;
    auto inner = self->renderer.inner();

    VkSamplerCreateInfo create_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    create_info.magFilter = config.mag_filter;
    create_info.minFilter = config.min_filter;
    create_info.addressModeU = config.address_u;
    create_info.addressModeV = config.address_v;
    create_info.addressModeW = config.address_w;
    create_info.anisotropyEnable = config.anisotropy_enable;
    create_info.maxAnisotropy = config.max_anisotropy;
    create_info.borderColor = config.border_color;
    create_info.unnormalizedCoordinates = VK_FALSE;
    create_info.compareEnable = config.compare_enable;
    create_info.compareOp = config.compare_op;
    create_info.mipmapMode = config.mipmap_mode;
    create_info.mipLodBias = config.mip_lod_bias;
    create_info.minLod = config.min_lod;
    create_info.maxLod = config.max_lod;

    VkResult result = vkCreateSampler(inner->device, &create_info, nullptr, &self->sampler);
    if(result != VK_SUCCESS){
        throw std::runtime_error(
                std::format("Failed to create a sampler! label = {}, result = {}", self->label, static_cast<uint32_t>(result) )
        );
    }
}
