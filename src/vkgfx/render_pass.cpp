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

#include "../include/vkgfx/render_pass.hpp"

namespace g_app {

    RenderPass::RenderPass(VulkanRenderer renderer, const Config& config): self{std::make_shared<Inner>(renderer)} {
        self->label = config.label;

        std::vector<VkSubpassDescription> subpasses;
        subpasses.reserve(config.subpasses.size());

        for(auto& subpass : config.subpasses){
            assert(subpass.resolve_attachments.empty() ||
                   subpass.color_attachments.size() == subpass.resolve_attachments.size() &&
                   "resolve_attachments must be the same length as color attachments if not empty()");

            VkSubpassDescription desc = {};
            desc.flags = subpass.flags;
            desc.pipelineBindPoint = subpass.bind_point;
            desc.inputAttachmentCount = subpass.input_attachments.size();
            desc.pInputAttachments = subpass.input_attachments.data();
            desc.colorAttachmentCount = subpass.color_attachments.size();
            desc.pColorAttachments = subpass.color_attachments.data();
            desc.pResolveAttachments = subpass.resolve_attachments.data();
            desc.pDepthStencilAttachment = (subpass.depth_stencil_attachment.has_value()) ? &subpass.depth_stencil_attachment.value() : nullptr;
            desc.preserveAttachmentCount = subpass.preserve_attachments.size();
            desc.pPreserveAttachments = subpass.preserve_attachments.data();

            subpasses.push_back(desc);
        }

        VkRenderPassCreateInfo create_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        create_info.attachmentCount = config.attachments.size();
        create_info.pAttachments = config.attachments.data();
        create_info.subpassCount = subpasses.size();
        create_info.pSubpasses = subpasses.data();
        create_info.dependencyCount = config.dependencies.size();
        create_info.pDependencies = config.dependencies.data();

        VkResult result = VK_SUCCESS;
        if((result = vkCreateRenderPass(renderer.inner()->device, &create_info, nullptr, &self->render_pass)) != VK_SUCCESS){
            throw std::runtime_error(
                    std::format("Failed to create a render pass! label = {}, result = {}", self->label, static_cast<uint32_t>(result) )
            );
        }
    }
}