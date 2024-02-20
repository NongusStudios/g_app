//
// Created by jandr on 21/11/2023.
//

#pragma once

#include "command_buffer.hpp"

namespace g_app {

    class TextureInit {
    public:
        TextureInit() = default;

        ~TextureInit() {
            stbi_image_free(m_stb_data);
        }

        TextureInit &set_label(const std::string &label) {
            m_config.label = label;
            return *this;
        }

        // size = Each pixels size in bytes
        TextureInit &set_format(VkFormat format, size_t size) {
            m_config.format = format;
            m_config.size = size;
            return *this;
        }

        TextureInit &load_from_file(const std::string &path, int desired_channels = STBI_rgb_alpha) {
            if (m_stb_data) stbi_image_free(m_stb_data);

            int w, h, c;
            m_stb_data = stbi_load(path.c_str(), &w, &h, &c, desired_channels);
            if(!m_stb_data){
                spdlog::warn("TextureInit failed loading an image. TextureInit has not been modified! path = {}", path);
                return *this;
            }

            m_config.pixels = (void *) m_stb_data;
            m_config.extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};

            return *this;
        }

        TextureInit &set_pixels(uint32_t width, uint32_t height, void *pixels) {
            m_config.pixels = pixels;
            m_config.extent = {width, height};
            return *this;
        }

        std::pair<Image, ImageView> init(const VulkanRenderer &renderer) {
            auto image = ImageInit()
                    .set_label(std::format("{} -> Image", m_config.label))
                    .set_image_type(VK_IMAGE_TYPE_2D)
                    .set_extent(m_config.extent.width, m_config.extent.height)
                    .set_format(m_config.format)
                    .set_usage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
                    .set_memory_usage(VMA_MEMORY_USAGE_GPU_ONLY)
                    .init(renderer);
            {
                auto staging_buffer = BufferInit<uint8_t>()
                        .set_label(std::format("{} -> Staging Buffer", m_config.label))
                        .set_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
                        .set_memory_usage(VMA_MEMORY_USAGE_CPU_ONLY)
                        .set_size(m_config.extent.width * m_config.extent.height * m_config.size)
                        .set_data((uint8_t *) m_config.pixels)
                        .init(renderer);

                CommandBuffer(renderer)
                        .begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)
                        .transition_image_layout(image,
                             VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
                             0, VK_ACCESS_TRANSFER_WRITE_BIT,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT)
                        .copy_buffer_to_image(staging_buffer, image, VK_IMAGE_ASPECT_COLOR_BIT)
                        .transition_image_layout(image,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                             {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
                             VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
                        .submit(Queue::TRANSFER);
            }

            auto image_view = ImageViewInit()
                    .set_label(std::format("{} -> Image View", m_config.label))
                    .set_type(VK_IMAGE_VIEW_TYPE_2D)
                    .set_aspect_mask(VK_IMAGE_ASPECT_COLOR_BIT)
                    .set_image(image)
                    .init(renderer);

            return {image, image_view};
        }

    private:
        struct Config {
            VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
            size_t size = 4;
            VkExtent2D extent = {};
            void *pixels = nullptr;
            std::string label = "unnamed texture";
        };

        Config m_config = {};
        stbi_uc *m_stb_data = nullptr;
    };
}


