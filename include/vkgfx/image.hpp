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

#pragma once

#include "renderer.hpp"

#include <format>

#include <stb_image.h>

namespace g_app {
    class VulkanRenderer;

    class ImageInit;
    class Image {
    public:
        Image() = default;

        Image(const Image&) = default;
        Image& operator = (const Image&) = default;

        VkFormat format() const { return self->format; }
        uint32_t mip_levels() const { return self->mip_levels; }
        uint32_t layer_count() const { return self->layer_count; }
        VkExtent3D extent() const { return self->extent; }

        VkImage vk_image() const { return self->image; }
        VmaAllocation vma_allocation() const { return self->allocation; }
    private:
        struct Config {
            VkImageType image_type = VK_IMAGE_TYPE_2D;
            VkExtent3D extent = { 0, 0 };
            uint32_t mip_levels = 1;
            uint32_t array_layers = 1;
            VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
            VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
            VkImageCreateFlags flags = 0;
            VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
            VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
            VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO;
            std::string label = "unnamed image";
        };

        struct Inner {
            VulkanRenderer renderer;
            VkImage image = VK_NULL_HANDLE;
            VmaAllocation allocation = VK_NULL_HANDLE;
            VkExtent3D extent = {};
            VkFormat format;
            uint32_t mip_levels = 1;
            uint32_t layer_count = 1;
            std::string label;

            ~Inner(){
                if(!renderer.is_valid()) return;

                vmaDestroyImage(renderer.inner()->allocator, image, allocation);
            }
        };

        std::shared_ptr<Inner> self;

        Image(VulkanRenderer renderer, const Config& config);

        friend class ImageInit;
    };

    class ImageInit {
    public:
        ImageInit() = default;

        ImageInit& set_label(const std::string& label){
            m_config.label = label;
            return *this;
        }

        ImageInit& set_image_type(VkImageType type){
            m_config.image_type = type;
            return *this;
        }

        ImageInit& set_extent(uint32_t width, uint32_t height, uint32_t depth = 1){
            m_config.extent = {width, height, depth};
            return *this;
        }

        ImageInit& set_mip_levels(uint32_t levels){
            m_config.mip_levels = levels;
            return *this;
        }

        ImageInit& set_memory_usage(VmaMemoryUsage usage){
            m_config.memory_usage = usage;
            return *this;
        }

        ImageInit& set_array_layers(uint32_t layers){
            m_config.array_layers = layers;
            return *this;
        }

        ImageInit& set_format(VkFormat format){
            m_config.format = format;
            return *this;
        }

        ImageInit& set_tiling(VkImageTiling tiling){
            m_config.tiling = tiling;
            return *this;
        }

        ImageInit& set_usage(VkImageUsageFlags usage){
            m_config.usage = usage;
            return *this;
        }

        ImageInit& set_samples(VkSampleCountFlagBits samples){
            m_config.samples = samples;
            return *this;
        }

        ImageInit& set_initial_layout(VkImageLayout layout){
            m_config.initial_layout = layout;
            return *this;
        }

        Image init(const VulkanRenderer& renderer){
            try {
                return {renderer, m_config};
            } catch(const std::runtime_error& e) {
                spdlog::error(e.what());
                std::exit(EXIT_FAILURE);
            }
        }

    private:
        Image::Config m_config = {};
    };

    class ImageViewInit;
    class ImageView {
    public:
        ImageView(): self{std::make_shared<Inner>()} {}

        ImageView(const ImageView&) = default;

        VkImageView vk_image_view() const { return self->view; }
    private:
        struct Config {
            Image image;
            VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
            VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;

            std::string label = "unnamed image view";
        };

        struct Inner {
            VulkanRenderer renderer;
            VkImageView view = VK_NULL_HANDLE;
            std::string label;

            ~Inner(){
                if(!renderer.is_valid()) return;

                vkDestroyImageView(renderer.inner()->device, view, nullptr);
            }
        };

        std::shared_ptr<Inner> self;

        ImageView(const VulkanRenderer& renderer, const Config& config);

        friend class ImageViewInit;
    };

    class ImageViewInit {
    public:
        ImageViewInit() = default;

        ImageViewInit& set_label(const std::string& label){
            m_config.label = label;
            return *this;
        }

        ImageViewInit& set_image(const Image& image){
            m_config.image = image;
            return *this;
        }

        ImageViewInit& set_type(VkImageViewType type){
            m_config.view_type = type;
            return *this;
        }

        ImageViewInit& set_aspect_mask(VkImageAspectFlags flags){
            m_config.aspect_mask = flags;
            return *this;
        }

        ImageView init(const VulkanRenderer& renderer){
            try {
                return {renderer, m_config};
            } catch(const std::runtime_error& e) {
                spdlog::error(e.what());
                std::exit(EXIT_FAILURE);
            }
        }
    private:
        ImageView::Config m_config = {};
    };

    class SamplerInit;
    class Sampler {
    public:
        Sampler() = default;

        VkSampler vk_sampler() const { return self->sampler; }
    private:
        struct Config {
            VkFilter mag_filter = VK_FILTER_LINEAR;
            VkFilter min_filter = VK_FILTER_LINEAR;
            VkSamplerMipmapMode mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            VkSamplerAddressMode address_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            VkSamplerAddressMode address_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            VkSamplerAddressMode address_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            VkBorderColor border_color = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            bool anisotropy_enable = false;
            float max_anisotropy = 1.0f;
            bool compare_enable = false;
            VkCompareOp compare_op = VK_COMPARE_OP_ALWAYS;
            float mip_lod_bias = 0.0f;
            float min_lod = 0.0f;
            float max_lod = 0.0f;
            std::string label = "unnamed sampler";
        };

        struct Inner {
            VulkanRenderer renderer;
            VkSampler sampler = VK_NULL_HANDLE;
            std::string label;

            ~Inner(){
                if(!renderer.is_valid()) return;

                vkDestroySampler(renderer.inner()->device, sampler, nullptr);
            }
        };

        std::shared_ptr<Inner> self;

        Sampler(const VulkanRenderer& renderer, const Config& config);

        friend class SamplerInit;
    };

    class SamplerInit {
    public:
        SamplerInit() = default;

        SamplerInit& set_label(const std::string& label){
            m_config.label = label;
            return *this;
        }
        SamplerInit& set_filter(VkFilter mag, VkFilter min){
            m_config.mag_filter = mag;
            m_config.min_filter = min;
            return *this;
        }
        SamplerInit& set_mipmap_mode(VkSamplerMipmapMode mipmap_mode){
            m_config.mipmap_mode = mipmap_mode;
            return *this;
        }
        SamplerInit& set_address_modes(VkSamplerAddressMode U, VkSamplerAddressMode V, VkSamplerAddressMode W){
            m_config.address_u = U;
            m_config.address_v = V;
            m_config.address_w = W;
            return *this;
        }
        SamplerInit& set_border_color(VkBorderColor border_color){
            m_config.border_color = border_color;
            return *this;
        }
        SamplerInit& enable_anisotropy(float max_anisotropy){
            m_config.anisotropy_enable = true;
            m_config.max_anisotropy = max_anisotropy;
            return *this;
        }
        SamplerInit& disable_anisotropy(){
            m_config.anisotropy_enable = false;
            return *this;
        }
        SamplerInit& enable_compare_op(VkCompareOp op){
            m_config.compare_enable = true;
            m_config.compare_op = op;
            return *this;
        }
        SamplerInit& set_mip_options(float mip_lod_bias, float min_lod, float max_lod){
            m_config.mip_lod_bias = mip_lod_bias;
            m_config.min_lod = min_lod;
            m_config.max_lod = max_lod;
            return *this;
        }
        Sampler init(VulkanRenderer renderer){
            try {
                return {renderer, m_config};
            } catch(const std::runtime_error& e) {
                spdlog::error(e.what());
                std::exit(EXIT_FAILURE);
            }
        }
    private:
        Sampler::Config m_config = {};
    };
}