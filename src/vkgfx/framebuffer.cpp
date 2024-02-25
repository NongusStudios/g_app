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

#include <vkgfx/framebuffer.hpp>

namespace g_app {
    Framebuffer::Framebuffer(VulkanRenderer renderer, const Framebuffer::Config &config):
        self{std::make_shared<Inner>(renderer)}
    {
        self->label = config.label;

        VkFramebufferCreateInfo create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        create_info.attachmentCount = config.image_views.size();
        create_info.pAttachments = config.image_views.data();
        create_info.renderPass = config.render_pass;
        create_info.width = config.extent.width;
        create_info.height = config.extent.height;
        create_info.layers = config.layers;

        VkResult result;
        if((result = vkCreateFramebuffer(renderer.inner()->device, &create_info, nullptr, &self->framebuffer)) != VK_SUCCESS){
            throw std::runtime_error(
                    std::format("Failed to create a framebuffer! label = {}, result = {}", self->label, static_cast<uint32_t>(result) )
            );
        }
    }
}