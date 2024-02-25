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

#include <stdexcept>
#include <optional>

namespace g_app {
    struct SubpassDescription {
        VkSubpassDescriptionFlags flags;
        VkPipelineBindPoint bind_point;
        std::vector<VkAttachmentReference> input_attachments;
        std::vector<VkAttachmentReference> color_attachments;
        std::vector<VkAttachmentReference> resolve_attachments;
        std::optional<VkAttachmentReference> depth_stencil_attachment;
        std::vector<uint32_t> preserve_attachments;
    };

    class RenderPassInit;
    class RenderPass {
    public:
        RenderPass() = default;

        VkRenderPass vk_render_pass() const { return self->render_pass; }
    private:
        struct Config {
            std::vector<VkAttachmentDescription> attachments = {};
            std::vector<SubpassDescription> subpasses = {};
            std::vector<VkSubpassDependency> dependencies = {};
            std::string label = "unnamed render pass";
        };

        struct Inner {
            VulkanRenderer renderer;
            VkRenderPass render_pass = VK_NULL_HANDLE;
            std::string label;

            ~Inner(){
                if(!renderer.is_valid()) return;

                vkDestroyRenderPass(renderer.inner()->device, render_pass, nullptr);
            }
        };

        std::shared_ptr<Inner> self;

        RenderPass(VulkanRenderer renderer, const Config& config);

        friend class RenderPassInit;
    };

    class AttachmentDescriptionBuilder {
    public:
        AttachmentDescriptionBuilder() = default;

        AttachmentDescriptionBuilder& set_format(VkFormat format){
            m_desc.format = format;
            return *this;
        }

        AttachmentDescriptionBuilder& set_sample_count(VkSampleCountFlagBits samples){
            m_desc.samples = samples;
            return *this;
        }

        AttachmentDescriptionBuilder& set_load_and_store_op(VkAttachmentLoadOp load, VkAttachmentStoreOp store){
            m_desc.loadOp = load;
            m_desc.storeOp = store;
            return *this;
        }

        AttachmentDescriptionBuilder& set_stencil_load_and_store_op(VkAttachmentLoadOp load, VkAttachmentStoreOp store){
            m_desc.stencilLoadOp = load;
            m_desc.stencilStoreOp = store;
            return *this;
        }

        AttachmentDescriptionBuilder& set_initial_and_final_image_layout(VkImageLayout initial, VkImageLayout final){
            m_desc.initialLayout = initial;
            m_desc.finalLayout = final;
            return *this;
        }

        VkAttachmentDescription build() { return m_desc; }
    private:
        VkAttachmentDescription m_desc = {};
    };



    class SubpassDescriptionBuilder {
    public:
        SubpassDescriptionBuilder& set_flags(VkSubpassDescriptionFlags flags){
            m_desc.flags = flags;
            return *this;
        }

        SubpassDescriptionBuilder& set_pipeline_bind_point(VkPipelineBindPoint bind_point){
            m_desc.bind_point = bind_point;
            return *this;
        }

        SubpassDescriptionBuilder& add_input_attachment(const VkAttachmentReference& ref){
            m_desc.input_attachments.push_back(ref);
            return *this;
        }

        SubpassDescriptionBuilder& add_color_attachment(const VkAttachmentReference& ref){
            m_desc.color_attachments.push_back(ref);
            return *this;
        }

        SubpassDescriptionBuilder& add_resolve_attachment(const VkAttachmentReference& ref){
            m_desc.resolve_attachments.push_back(ref);
            return *this;
        }

        SubpassDescriptionBuilder& set_depth_stencil_attachment(const VkAttachmentReference& ref){
            m_desc.depth_stencil_attachment = ref;
            return *this;
        }

        SubpassDescriptionBuilder& add_preserve_attachment(const uint32_t index){
            m_desc.preserve_attachments.push_back(index);
            return *this;
        }

        SubpassDescription build() {
            return m_desc;
        }
    private:
        SubpassDescription m_desc = {};
    };

    class SubpassDependencyBuilder {
    public:
        SubpassDependencyBuilder() = default;

        SubpassDependencyBuilder& set_dependency_flags(VkDependencyFlags flags){
            m_desc.dependencyFlags = flags;
            return *this;
        }

        SubpassDependencyBuilder& set_src_subpass(const uint32_t index){
            m_desc.srcSubpass = index;
            return *this;
        }

        SubpassDependencyBuilder& set_src_stage_mask(VkPipelineStageFlags stage){
            m_desc.srcStageMask = stage;
            return *this;
        }

        SubpassDependencyBuilder& set_src_access_mask(VkAccessFlags access){
            m_desc.srcStageMask = access;
            return *this;
        }

        SubpassDependencyBuilder& set_dst_subpass(const uint32_t index){
            m_desc.dstSubpass = index;
            return *this;
        }

        SubpassDependencyBuilder& set_dst_stage_mask(VkPipelineStageFlags stage){
            m_desc.dstStageMask = stage;
            return *this;
        }

        SubpassDependencyBuilder& set_dst_access_mask(VkAccessFlags access){
            m_desc.dstStageMask = access;
            return *this;
        }

        VkSubpassDependency build() {
            return m_desc;
        }
    private:
        VkSubpassDependency m_desc = {};
    };

    class RenderPassInit {
    public:
        RenderPassInit() = default;

        RenderPassInit& set_label(const std::string& label){
            m_config.label = label;
            return *this;
        }

        RenderPassInit& add_attachment_description(const VkAttachmentDescription& description){
            m_config.attachments.push_back(description);
            return *this;
        }

        RenderPassInit& add_subpass_description(const SubpassDescription& subpass){
            m_config.subpasses.push_back(subpass);
            return *this;
        }

        RenderPassInit& add_subpass_dependency(const VkSubpassDependency& dependency){
            m_config.dependencies.push_back(dependency);
            return *this;
        }

        RenderPass init(VulkanRenderer renderer){
            try {
                return {renderer, m_config};
            } catch(const std::runtime_error& e) {
                spdlog::error(e.what());
                std::exit(EXIT_FAILURE);
            }
        }
    private:
        RenderPass::Config m_config = {};
    };
}
