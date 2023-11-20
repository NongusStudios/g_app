//
// Created by jandr on 18/11/2023.
//

#pragma once

#include "renderer.hpp"

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
            VkFormat format;
            uint32_t mip_levels = 1;
            uint32_t layer_count = 1;
            std::string label;

            ~Inner(){
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
        ImageView& operator = (const ImageView&) = default;

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
}