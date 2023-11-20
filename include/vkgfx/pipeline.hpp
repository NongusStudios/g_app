//
// Created by jandr on 14/11/2023.
//

#pragma once

#include <fstream>
#include <format>

#include "renderer.hpp"
#include "render_pass.hpp"
#include "descriptor.hpp"

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

    class Pipeline {
    public:
        struct RasterizationInfo {
            VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
            float line_width = 1.0f;
            VkCullModeFlags cull_mode = VK_CULL_MODE_NONE;
            VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE;
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

        struct VertexAttribute {
            VkFormat format;
            uint32_t offset;
        };
        struct VertexBinding {
            uint32_t stride = 0;
            VkVertexInputRate input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
            std::vector<VertexAttribute> attributes = {};
        };

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

            VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            RasterizationInfo rasterization_info;
            VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;
            BlendInfo blend_info;
            DepthStencilInfo depth_stencil_info;
        };

        struct Inner {
            VulkanRenderer renderer;
            VkPipeline pipeline = VK_NULL_HANDLE;
            VkPipelineLayout layout = VK_NULL_HANDLE;
            std::string label;

            ~Inner(){
                vkDestroyPipeline(renderer.inner()->device, pipeline, nullptr);
                vkDestroyPipelineLayout(renderer.inner()->device, layout, nullptr);
            }
        };

        std::shared_ptr<Inner> self;

        // Inits graphics pipeline
        Pipeline(VulkanRenderer renderer, const GraphicsConfig& config);

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
        Pipeline::VertexBinding build(){
            return binding;
        }
    private:
        Pipeline::VertexBinding binding = {};
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
        GraphicsPipelineInit& set_rasterization_info(const Pipeline::RasterizationInfo& info){
            m_config.rasterization_info = info;
            return *this;
        }
        GraphicsPipelineInit& set_sample_count(VkSampleCountFlagBits samples){
            m_config.sample_count = samples;
            return *this;
        }
        GraphicsPipelineInit& set_blend_info(const Pipeline::BlendInfo& info){
            m_config.blend_info = info;
            return *this;
        }
        GraphicsPipelineInit& set_depth_stencil_info(const Pipeline::DepthStencilInfo& info){
            m_config.depth_stencil_info = info;
            return *this;
        }
        GraphicsPipelineInit& add_vertex_binding(const Pipeline::VertexBinding& vertex_binding){
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

    private:
    };
} // g_app
