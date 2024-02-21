//
// Created by jeol on 21/02/24.
//
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