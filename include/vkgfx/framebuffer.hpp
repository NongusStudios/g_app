//
// Created by jeol on 21/02/24.
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
            VkRenderPass render_pass = VK_NULL_HANDLE;
            std::vector<VkImageView> image_views;
            Extent2D<uint32_t> extent = {800, 600};
            uint32_t layers = 1;
        };

        VkFramebuffer vk_framebuffer() const { return self->framebuffer; }
    private:
        Framebuffer(VulkanRenderer renderer, const Config& config);


        struct Inner {
            VulkanRenderer renderer;
            VkFramebuffer framebuffer;
            std::string label;

            ~Inner(){
                if(!renderer.is_valid()) return;

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
        FramebufferInit& set_extent(const uint32_t width, const uint32_t height){
            m_config.extent = {width, height};
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