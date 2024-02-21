//
// Created by jeol on 21/02/24.
//
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

#include "renderer.hpp"
#include "image.hpp"
#include "render_pass.hpp"
#include "../types.hpp"

namespace g_app {
    class FramebufferInit;
    class Framebuffer {
    public:
        Framebuffer() = default;

        struct Config {
            std::string label = "unnamed framebuffer";
            VkRenderPass render_pass;
            std::vector<VkImageView> image_views;
            Extent2D<uint32_t> extent;
            uint32_t layers;
        };
    private:
        Framebuffer(VulkanRenderer renderer, const Config& config);


        struct Inner {
            VulkanRenderer renderer;
            VkFramebuffer framebuffer;
            std::string label;

            ~Inner(){
                vkDestroyFramebuffer(renderer.inner()->device, framebuffer, nullptr);
            }
        };
        std::shared_ptr<Inner> self;

        friend class FramebufferInit;
    };

    class FramebufferInit {
    public:
        FramebufferInit() = default;

        FramebufferInit& set_label(const std::string& label){
            m_config.label = label;
            return *this;
        }
        FramebufferInit& set_render_pass(const RenderPass& render_pass){
            m_config.render_pass = render_pass.vk_render_pass();
            return *this;
        }
        FramebufferInit& set_render_pass(VkRenderPass render_pass){
            m_config.render_pass = render_pass;
            return *this;
        }
        FramebufferInit& attach_image_view(const ImageView& view){
            m_config.image_views.push_back(view.vk_image_view());
            return *this;
        }
        FramebufferInit& attach_image_views(const std::vector<ImageView>& views){
            std::vector<VkImageView> tmp_views = {};
            tmp_views.reserve(views.size());
            for(auto& view : views){
                tmp_views.push_back(view.vk_image_view());
            }

            m_config.image_views.insert(m_config.image_views.end(), tmp_views.begin(), tmp_views.end());
            return *this;
        }
        FramebufferInit& set_extent(const Extent2D<uint32_t>& extent){
            m_config.extent = extent;
            return *this;
        }
        FramebufferInit& set_layers(uint32_t layers){
            m_config.layers = layers;
            return *this;
        }

        Framebuffer init(const VulkanRenderer& renderer){
            try {
                return {renderer, m_config};
            } catch(const std::runtime_error& e) {
                spdlog::error(e.what());
                std::exit(EXIT_FAILURE);
            }
        }
    private:
        Framebuffer::Config m_config;
    };
}