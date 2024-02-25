//
// Created by jandr on 14/11/2023.
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

#include <fstream>
#include <format>
#include <utility>

#include "renderer.hpp"
#include "render_pass.hpp"
#include "descriptor.hpp"
#include "pipeline_cache.hpp"

#include <spdlog/spdlog.h>

namespace g_app {
    class ShaderModuleInit;

    class ShaderModule {
    public:
        ShaderModule() = default;

        VkPipelineShaderStageCreateInfo stage_info() const {
            return self->stage_info;
        }
        VkShaderModule vk_shader_module() const {
            return self->module;
        }

        void change_entry_point(const std::string& name){
            self->entry = name;
            self->stage_info.pName = self->entry.c_str();
        }
    private:
        struct Config {
            std::vector<char> src = {};
            VkShaderStageFlagBits stage = VK_SHADER_STAGE_VERTEX_BIT;
            std::string entry = "main";
            std::string label = "unnamed shader";
        };

        struct Inner {
            VulkanRenderer renderer = {};
            VkShaderModule module = VK_NULL_HANDLE;
            VkPipelineShaderStageCreateInfo stage_info = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
            std::string entry;
            std::string label;

            ~Inner(){
                if(!renderer.is_valid()) return;

                vkDestroyShaderModule(renderer.inner()->device, module, nullptr);
            }
        };

        std::shared_ptr<Inner> self;

        ShaderModule(VulkanRenderer renderer, const Config& config);

        friend class ShaderModuleInit;
    };

    class ShaderModuleInit {
    public:
        ShaderModuleInit() = default;

        ShaderModuleInit& set_label(const std::string& label){
            m_config.label = label;
            return *this;
        }

        /*
         * @param src : Compiled spirv shader code.
         */
        ShaderModuleInit& set_src(const std::vector<char>& src){
            m_config.src = src;
            return *this;
        }
        /*
         * @param path : File path to compiled shader code.
         */
        ShaderModuleInit& set_src_from_file(const std::string& path){
            std::ifstream fp(path, std::ios::binary | std::ios::ate);

            if(!fp.is_open()){
                spdlog::error("Failed to open {}", path);
                std::exit(EXIT_FAILURE);
            }

            std::streamsize len = fp.tellg();
            fp.seekg(0);
            m_config.src.resize(len);

            fp.read(m_config.src.data(), len);

            fp.close();

            return *this;
        }

        ShaderModuleInit& set_stage(VkShaderStageFlagBits stage){
            m_config.stage = stage;
            return *this;
        }

        ShaderModuleInit& set_entry_point(const std::string& name){
            m_config.entry = name;
            return *this;
        }

        ShaderModule init(const VulkanRenderer& renderer){
            try {
                return {renderer, m_config};
            } catch(const std::runtime_error& e) {
                spdlog::error(e.what());
                std::exit(EXIT_FAILURE);
            }
        }
    private:
        ShaderModule::Config m_config = {};
    };

    class GraphicsPipelineInit;
    class ComputePipelineInit;

    struct RasterizationInfo {
        VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
        float line_width = 1.0f;
        VkCullModeFlags cull_mode = VK_CULL_MODE_NONE;
        VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE;
    };
    class RasterizationInfoBuilder {
    public:
        RasterizationInfoBuilder() = default;

        RasterizationInfoBuilder& set_polygon_mode(VkPolygonMode mode){
            m_info.polygon_mode = mode;
            return *this;
        }

        RasterizationInfoBuilder& set_line_width(float width){
            m_info.line_width = width;
            return *this;
        }

        RasterizationInfoBuilder& set_cull_mode(VkCullModeFlags cull_mode){
            m_info.cull_mode = cull_mode;
            return *this;
        }

        RasterizationInfoBuilder& set_front_face(VkFrontFace face){
            m_info.front_face = face;
            return *this;
        }

        RasterizationInfo build(){
            return m_info;
        }
    private:
        RasterizationInfo m_info = {};
    };

    struct BlendInfo {
        bool blend_enabled = true;
        VkBlendFactor src_color_factor = VK_BLEND_FACTOR_SRC_ALPHA;
        VkBlendFactor dst_color_factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        VkBlendOp color_op = VK_BLEND_OP_ADD;
        VkBlendFactor src_alpha_factor = VK_BLEND_FACTOR_ONE;
        VkBlendFactor dst_alpha_factor = VK_BLEND_FACTOR_ZERO;
        VkBlendOp alpha_op = VK_BLEND_OP_ADD;
        bool logic_op_enabled = false;
        VkLogicOp logic_op = VK_LOGIC_OP_COPY;
    };
    class BlendInfoBuilder {
    public:
        BlendInfoBuilder() = default;

        BlendInfoBuilder& enable_blending() {
            m_info.blend_enabled = true;
            return *this;
        }

        BlendInfoBuilder& disable_blending() {
            m_info.blend_enabled = false;
            return *this;
        }

        BlendInfoBuilder& enable_logic_op(VkLogicOp op) {
            m_info.logic_op_enabled = true;
            m_info.logic_op = op;
            return *this;
        }

        BlendInfoBuilder& disable_logic_op() {
            m_info.logic_op_enabled = false;
            return *this;
        }

        BlendInfoBuilder& set_color_factor(VkBlendFactor src, VkBlendFactor dst, VkBlendOp op){
            m_info.src_color_factor = src;
            m_info.dst_color_factor = dst;
            m_info.color_op = op;
            return *this;
        }

        BlendInfoBuilder& set_alpha_factor(VkBlendFactor src, VkBlendFactor dst, VkBlendOp op){
            m_info.src_alpha_factor = src;
            m_info.dst_alpha_factor = dst;
            m_info.alpha_op = op;
            return *this;
        }

        BlendInfo build() { return m_info; }
    private:
        BlendInfo m_info = {};
    };

    struct DepthStencilInfo {
        bool depth_enabled = true;
        bool write_enabled = true;
        VkCompareOp compare_op = VK_COMPARE_OP_LESS;
        bool bounds_test_enabled = false;
        float min_depth_bounds = 0.0f;
        float max_depth_bounds = 1.0f;
        bool stencil_enabled = false;
        VkStencilOpState front = {};
        VkStencilOpState back = {};
    };
    class DepthStencilInfoBuilder {
    public:
        DepthStencilInfoBuilder() = default;

        DepthStencilInfoBuilder& enable_depth_test(bool write_enabled){
            m_info.depth_enabled = true;
            m_info.write_enabled = write_enabled;
            return *this;
        }

        DepthStencilInfoBuilder& disable_depth_test(){
            m_info.depth_enabled = false;
            return *this;
        }

        DepthStencilInfoBuilder& set_compare_op(VkCompareOp op){
            m_info.compare_op = op;
            return *this;
        }

        DepthStencilInfoBuilder& set_depth_bounds(float min, float max){
            m_info.min_depth_bounds = min;
            m_info.max_depth_bounds = max;
            return *this;
        }

        DepthStencilInfoBuilder& enable_stencil(VkStencilOpState front, VkStencilOpState back){
            m_info.stencil_enabled = true;
            m_info.front = front;
            m_info.back = back;
            return *this;
        }

        DepthStencilInfoBuilder& disable_stencil(){
            m_info.stencil_enabled = false;
            return *this;
        }

        DepthStencilInfo build() { return m_info; }
    private:
        DepthStencilInfo m_info = {};
    };

    struct VertexAttribute {
        VkFormat format;
        uint32_t offset;
    };
    struct VertexBinding {
        uint32_t stride = 0;
        VkVertexInputRate input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
        std::vector<VertexAttribute> attributes = {};
    };

    class Pipeline {
    public:
        Pipeline() = default;

        VkPipeline vk_pipeline() const { return self->pipeline; }
        VkPipelineLayout vk_pipeline_layout() const { return self->layout; }
    private:
        struct GraphicsConfig {
            std::string label = "unnamed pipeline";
            std::vector<ShaderModule> modules = {};
            std::vector<VkPushConstantRange> push_constants = {};
            std::vector<VkDescriptorSetLayout> set_layouts = {};
            std::vector<VertexBinding> bindings = {};
            VkRenderPass render_pass = VK_NULL_HANDLE;
            VkPipelineCache pipeline_cache = VK_NULL_HANDLE;
            uint32_t subpass = 0;

            VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            RasterizationInfo rasterization_info;
            VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;
            BlendInfo blend_info;
            DepthStencilInfo depth_stencil_info;
        };

        struct ComputeConfig {
            std::string label = "unnamed pipeline";
            ShaderModule module;
            std::vector<VkPushConstantRange> push_constants = {};
            std::vector<VkDescriptorSetLayout> set_layouts = {};
            VkPipelineCache pipeline_cache = VK_NULL_HANDLE;
        };

        struct Inner {
            VulkanRenderer renderer;
            VkPipeline pipeline = VK_NULL_HANDLE;
            VkPipelineLayout layout = VK_NULL_HANDLE;
            std::string label;

            ~Inner(){
                if(!renderer.is_valid()) return;

                vkDestroyPipeline(renderer.inner()->device, pipeline, nullptr);
                vkDestroyPipelineLayout(renderer.inner()->device, layout, nullptr);
            }
        };

        std::shared_ptr<Inner> self;

        // Inits graphics pipeline
        Pipeline(VulkanRenderer renderer, const GraphicsConfig& config);
        Pipeline(VulkanRenderer renderer, const ComputeConfig& config);

        friend class GraphicsPipelineInit;
        friend class ComputePipelineInit;
    };

    class VertexBindingBuilder {
    public:
        explicit VertexBindingBuilder(uint32_t stride, VkVertexInputRate input_rate = VK_VERTEX_INPUT_RATE_VERTEX):
            binding{stride, input_rate}
        {}
        VertexBindingBuilder& add_vertex_attribute(VkFormat format, uint32_t offset){
            binding.attributes.push_back({format, offset});
            return *this;
        }
        VertexBinding build(){
            return binding;
        }
    private:
        VertexBinding binding = {};
    };

    class GraphicsPipelineInit {
    public:
        GraphicsPipelineInit() = default;

        GraphicsPipelineInit& set_label(const std::string& label){
            m_config.label = label;
            return *this;
        }
        GraphicsPipelineInit& attach_shader_module(const ShaderModule& module){
            m_config.modules.push_back(module);
            return *this;
        }
        GraphicsPipelineInit& set_topology(VkPrimitiveTopology topology){
            m_config.topology = topology;
            return *this;
        }
        GraphicsPipelineInit& set_rasterization_info(const RasterizationInfo& info){
            m_config.rasterization_info = info;
            return *this;
        }
        GraphicsPipelineInit& set_sample_count(VkSampleCountFlagBits samples){
            m_config.sample_count = samples;
            return *this;
        }
        GraphicsPipelineInit& set_blend_info(const BlendInfo& info){
            m_config.blend_info = info;
            return *this;
        }
        GraphicsPipelineInit& set_depth_stencil_info(const DepthStencilInfo& info){
            m_config.depth_stencil_info = info;
            return *this;
        }
        GraphicsPipelineInit& add_vertex_binding(const VertexBinding& vertex_binding){
            m_config.bindings.push_back(vertex_binding);
            return *this;
        }
        GraphicsPipelineInit& add_push_constant_range(const VkPushConstantRange& range){
            m_config.push_constants.push_back(range);
            return *this;
        }
        GraphicsPipelineInit& add_descriptor_set_layout(const DescriptorSetLayout& layout){
            m_config.set_layouts.push_back(layout.vk_descriptor_set_layout());
            return *this;
        }
        GraphicsPipelineInit& set_render_pass(const RenderPass& render_pass){
            m_config.render_pass = render_pass.vk_render_pass();
            return *this;
        }
        GraphicsPipelineInit& set_render_pass(VkRenderPass render_pass){
            m_config.render_pass = render_pass;
            return *this;
        }
        GraphicsPipelineInit& set_subpass(const uint32_t subpass){
            m_config.subpass = subpass;
            return *this;
        }
        GraphicsPipelineInit& set_pipeline_cache(const PipelineCache& cache){
            m_config.pipeline_cache = cache.vk_pipeline_cache();
            return *this;
        }
        GraphicsPipelineInit& set_pipeline_cache(VkPipelineCache cache){
            m_config.pipeline_cache = cache;
            return *this;
        }

        Pipeline init(const VulkanRenderer& renderer){
            try {
                return {renderer, m_config};
            } catch(const std::runtime_error& e) {
                spdlog::error(e.what());
                std::exit(EXIT_FAILURE);
            }
        }
    private:
        Pipeline::GraphicsConfig m_config;
    };

    class ComputePipelineInit {
    public:
        ComputePipelineInit() = default;

        ComputePipelineInit& set_label(const std::string& label){
            m_config.label = label;
            return *this;
        }
        ComputePipelineInit& set_shader_module(const ShaderModule& module){
            m_config.module = module;
            return *this;
        }
        ComputePipelineInit& add_push_constant_range(const VkPushConstantRange& range){
            m_config.push_constants.push_back(range);
            return *this;
        }
        ComputePipelineInit& add_descriptor_set_layout(const DescriptorSetLayout& layout){
            m_config.set_layouts.push_back(layout.vk_descriptor_set_layout());
            return *this;
        }
        ComputePipelineInit& set_pipeline_cache(const PipelineCache& cache){
            m_config.pipeline_cache = cache.vk_pipeline_cache();
            return *this;
        }
        ComputePipelineInit& set_pipeline_cache(VkPipelineCache cache){
            m_config.pipeline_cache = cache;
            return *this;
        }

        Pipeline init(const VulkanRenderer& renderer){
            try {
                return {renderer, m_config};
            } catch(const std::runtime_error& e) {
                spdlog::error(e.what());
                std::exit(EXIT_FAILURE);
            }
        }
    private:
        Pipeline::ComputeConfig m_config;
    };
} // g_app
