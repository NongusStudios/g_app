//
// Created by jandr on 18/11/2023.
//
#include "../include/vkgfx/render_pass.hpp"

namespace g_app {

    RenderPass::RenderPass(VulkanRenderer renderer, const Config& config): self{std::make_shared<Inner>(renderer)} {
        self->label = config.label;

        VkRenderPassCreateInfo create_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        create_info.attachmentCount = config.attachments.size();
        create_info.pAttachments = config.attachments.data();
        create_info.subpassCount = config.subpasses.size();
        create_info.pSubpasses = config.subpasses.data();
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