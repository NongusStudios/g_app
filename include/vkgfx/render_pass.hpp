//
// Created by jandr on 18/11/2023.
//

#pragma once

#include "renderer.hpp"

#include <stdexcept>

namespace g_app {
    class RenderPassInit;
    class RenderPass {
    public:
        RenderPass() = default;

        VkRenderPass vk_render_pass() const { return self->render_pass; }
    private:
        struct Config {
            std::vector<VkAttachmentDescription> attachments = {};
            std::vector<VkSubpassDescription> subpasses = {};
            std::vector<VkSubpassDependency> dependencies = {};
            std::string label = "unnamed render pass";
        };

        struct Inner {
            VulkanRenderer renderer;
            VkRenderPass render_pass = VK_NULL_HANDLE;
            std::string label;
        };

        std::shared_ptr<Inner> self;

        RenderPass(VulkanRenderer renderer, const Config& config);

        friend class RenderPassInit;
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

        RenderPassInit& add_subpass_description(const VkSubpassDescription& subpass){
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
