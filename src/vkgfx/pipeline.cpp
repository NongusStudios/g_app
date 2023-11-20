//
// Created by jandr on 14/11/2023.
//

#include "../include/vkgfx/pipeline.hpp"

namespace g_app {
    ShaderModule::ShaderModule(VulkanRenderer renderer, const ShaderModule::Config &config): self{std::make_shared<Inner>(renderer)} {
        self->label = config.label;
        self->entry = config.entry;

        auto inner = renderer.inner();

        VkShaderModuleCreateInfo create_info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        create_info.codeSize = config.src.size();
        create_info.pCode = reinterpret_cast<const uint32_t*>(config.src.data());

        VkResult result = VK_SUCCESS;
        if((result = vkCreateShaderModule(inner->device, &create_info, nullptr, &self->module)) != VK_SUCCESS){
            throw std::runtime_error(
                    std::format("Failed to create a shader! label = {}, result = {}", self->label, static_cast<uint32_t>(result) )
            );
        }

        self->stage_info.module = self->module;
        self->stage_info.stage = config.stage;
        self->stage_info.pName = self->entry.c_str();
        self->stage_info.pSpecializationInfo = nullptr;
    }

    Pipeline::Pipeline(VulkanRenderer renderer, const Pipeline::GraphicsConfig &config): self{std::make_shared<Inner>(renderer)} {
        self->label = config.label;

        auto inner = renderer.inner();

        VkPipelineLayoutCreateInfo layout_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layout_info.setLayoutCount = 0;
        layout_info.pushConstantRangeCount = config.push_constants.size();
        layout_info.pPushConstantRanges = config.push_constants.data();
        layout_info.setLayoutCount = config.set_layouts.size();
        layout_info.pSetLayouts = config.set_layouts.data();

        VkResult result = VK_SUCCESS;
        if((result = vkCreatePipelineLayout(inner->device, &layout_info, nullptr, &self->layout)) != VK_SUCCESS){
            throw std::runtime_error(
                    std::format("Failed to create a pipeline layout! label = {}, result = {}", self->label, static_cast<uint32_t>(result) )
            );
        }

        std::vector<VkPipelineShaderStageCreateInfo> stage_infos(config.modules.size());
        for(size_t i = 0; i < stage_infos.size(); i++){
            stage_infos[i] = config.modules[i].stage_info();
        }

        std::vector<VkVertexInputBindingDescription> bindings = {};
        std::vector<VkVertexInputAttributeDescription> attributes = {};

        uint32_t binding_index = 0;
        uint32_t attrib_location = 0;
        for(const auto& binding : config.bindings){
            VkVertexInputBindingDescription binding_desc = {};
            binding_desc.binding = binding_index;
            binding_desc.stride = binding.stride;
            binding_desc.inputRate = binding.input_rate;
            bindings.push_back(binding_desc);

            for(const auto& attribute : binding.attributes){
                VkVertexInputAttributeDescription attrib = {};
                attrib.location = attrib_location;
                attrib.binding = binding_index;
                attrib.offset = attribute.offset;
                attrib.format = attribute.format;

                attributes.push_back(attrib);

                attrib_location++;
            }

            binding_index++;
        }

        std::vector<VkPipelineShaderStageCreateInfo> stages = {};
        stages.reserve(config.modules.size());

        for(const auto& module : config.modules){
            stages.push_back(module.stage_info());
        }

        VkPipelineViewportStateCreateInfo viewport_info = {};
        VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
        VkPipelineRasterizationStateCreateInfo rasterization_info = {};
        VkPipelineMultisampleStateCreateInfo multisample_info = {};
        VkPipelineColorBlendAttachmentState color_blend_attachment = {};
        VkPipelineColorBlendStateCreateInfo color_blend_info = {};
        VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};
        std::vector<VkDynamicState> dynamic_state_enables = {};
        VkPipelineDynamicStateCreateInfo dynamic_state_info = {};

        input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_info.topology = config.topology;
        input_assembly_info.primitiveRestartEnable = VK_FALSE;

        viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_info.viewportCount = 1;
        viewport_info.pViewports = nullptr;
        viewport_info.scissorCount = 1;
        viewport_info.pScissors = nullptr;

        rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_info.depthClampEnable = VK_FALSE;
        rasterization_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_info.polygonMode = config.rasterization_info.polygon_mode;
        rasterization_info.lineWidth = config.rasterization_info.line_width;
        rasterization_info.cullMode = config.rasterization_info.cull_mode;
        rasterization_info.frontFace = config.rasterization_info.front_face;
        rasterization_info.depthBiasEnable = VK_FALSE;
        rasterization_info.depthBiasConstantFactor = 0.0f;  // Optional
        rasterization_info.depthBiasClamp = 0.0f;           // Optional
        rasterization_info.depthBiasSlopeFactor = 0.0f;     // Optional

        multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_info.sampleShadingEnable = VK_FALSE;
        multisample_info.rasterizationSamples = config.sample_count;
        multisample_info.minSampleShading = 1.0f;           // Optional
        multisample_info.pSampleMask = nullptr;             // Optional
        multisample_info.alphaToCoverageEnable = VK_FALSE;  // Optional
        multisample_info.alphaToOneEnable = VK_FALSE;       // Optional

        color_blend_attachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable = config.blend_info.blend_enabled;
        color_blend_attachment.srcColorBlendFactor = config.blend_info.src_color_factor;
        color_blend_attachment.dstColorBlendFactor = config.blend_info.dst_color_factor;
        color_blend_attachment.colorBlendOp = config.blend_info.color_op;
        color_blend_attachment.srcAlphaBlendFactor = config.blend_info.src_alpha_factor;
        color_blend_attachment.dstAlphaBlendFactor = config.blend_info.dst_alpha_factor;
        color_blend_attachment.alphaBlendOp = config.blend_info.alpha_op;

        color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_info.logicOpEnable = VK_FALSE;
        color_blend_info.logicOp = VK_LOGIC_OP_COPY;  // Optional
        color_blend_info.attachmentCount = 1;
        color_blend_info.pAttachments = &color_blend_attachment;
        color_blend_info.blendConstants[0] = 0.0f;  // Optional
        color_blend_info.blendConstants[1] = 0.0f;  // Optional
        color_blend_info.blendConstants[2] = 0.0f;  // Optional
        color_blend_info.blendConstants[3] = 0.0f;  // Optional

        depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_info.depthTestEnable = config.depth_stencil_info.depth_enabled;
        depth_stencil_info.depthWriteEnable = config.depth_stencil_info.write_enabled;
        depth_stencil_info.depthCompareOp = config.depth_stencil_info.compare_op;
        depth_stencil_info.depthBoundsTestEnable = config.depth_stencil_info.bounds_test_enabled;
        depth_stencil_info.minDepthBounds = config.depth_stencil_info.min_depth_bounds;
        depth_stencil_info.maxDepthBounds = config.depth_stencil_info.max_depth_bounds;
        depth_stencil_info.stencilTestEnable = VK_FALSE;
        depth_stencil_info.front = config.depth_stencil_info.front;
        depth_stencil_info.back = config.depth_stencil_info.back;

        dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_info.pDynamicStates = dynamic_state_enables.data();
        dynamic_state_info.dynamicStateCount =
                static_cast<uint32_t>(dynamic_state_enables.size());
        dynamic_state_info.flags = 0;

        VkPipelineVertexInputStateCreateInfo vertex_input = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertex_input.vertexBindingDescriptionCount = bindings.size();
        vertex_input.pVertexBindingDescriptions = bindings.data();
        vertex_input.vertexAttributeDescriptionCount = attributes.size();
        vertex_input.pVertexAttributeDescriptions = attributes.data();

        VkGraphicsPipelineCreateInfo create_info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        create_info.stageCount = static_cast<uint32_t>(stage_infos.size());
        create_info.pStages = stage_infos.data();
        create_info.pVertexInputState = &vertex_input;
        create_info.stageCount = stages.size();
        create_info.pStages = stages.data();
        create_info.renderPass = config.render_pass;
        create_info.layout = self->layout;
        create_info.pInputAssemblyState = &input_assembly_info;
        create_info.pViewportState = &viewport_info;
        create_info.pRasterizationState = &rasterization_info;
        create_info.pMultisampleState = &multisample_info;
        create_info.pColorBlendState = &color_blend_info;
        create_info.pDepthStencilState = &depth_stencil_info;
        create_info.pDynamicState = &dynamic_state_info;
        create_info.basePipelineIndex = -1;
        create_info.basePipelineHandle = VK_NULL_HANDLE;

        if((result =
            vkCreateGraphicsPipelines(inner->device, VK_NULL_HANDLE, 1, &create_info, nullptr, &self->pipeline))
            != VK_SUCCESS){
            throw std::runtime_error(
                    std::format("Failed to create a graphics pipeline! label = {}, result = {}", self->label, static_cast<uint32_t>(result) )
            );
        }
    }
} // g_app