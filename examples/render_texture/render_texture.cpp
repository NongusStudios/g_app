//
// Created by jandr on 22/02/2024.
//
#include <g_app.hpp>

using namespace g_app;

struct Vertex {
    float x,   y;
    float uvx, uvy;
};

class RenderTexture {
public:
    static constexpr uint32_t WIDTH = 800;
    static constexpr uint32_t HEIGHT = 600;
    const std::string CACHE_PATH = "../examples/render_texture/render_texture.cache";

    RenderTexture(const VulkanRenderer& renderer){
        const Vertex vertices[] = {
                Vertex{-1.0f, -1.0f, 0.0f, 0.0f }, // Top Left
                Vertex{ 1.0f, -1.0f, 1.0f, 0.0f }, // Top Right
                Vertex{ 1.0f,  1.0f, 1.0f, 1.0f }, // Bottom Right
                Vertex{-1.0f,  1.0f, 0.0f, 1.0f }, // Bottom Left
        };
        const uint32_t indices[] = {
                0, 1, 3, // #1
                1, 2, 3, // #2
        };

        m_vertex_buffer = BufferInit<Vertex>()
                .set_memory_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
                .set_usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
                .set_size(4)
                .set_data(vertices)
                .set_label("RenderTexture::m_vertex_buffer")
                .init(renderer);

        m_index_buffer = BufferInit<uint32_t>()
                .set_memory_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
                .set_usage(VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
                .set_size(6)
                .set_data(indices)
                .set_label("RenderTexture::m_index_buffer")
                .init(renderer);

        m_desc_pool = DescriptorPoolInit()
                .add_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VulkanRenderer::MAX_FRAMES_IN_FLIGHT)
                .set_max_sets(VulkanRenderer::MAX_FRAMES_IN_FLIGHT)
                .set_label("RenderTexture::m_desc_pool")
                .init(renderer);

        m_desc_layout = DescriptorSetLayoutInit()
                .add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                             VulkanRenderer::MAX_FRAMES_IN_FLIGHT, VK_SHADER_STAGE_FRAGMENT_BIT)
                .init(renderer);

        m_pipeline_cache = PipelineCache::load(renderer, CACHE_PATH);
        m_pipeline = GraphicsPipelineInit()
                .attach_shader_module(
                    ShaderModuleInit()
                        .set_src_from_file("../examples/render_texture/draw.vert.spv")
                        .set_stage(VK_SHADER_STAGE_VERTEX_BIT)
                        .set_label("draw.vert")
                        .init(renderer))
                .attach_shader_module(
                    ShaderModuleInit()
                            .set_src_from_file("../examples/render_texture/draw.frag.spv")
                            .set_stage(VK_SHADER_STAGE_FRAGMENT_BIT)
                            .set_label("draw.frag")
                            .init(renderer))
                .add_vertex_binding(VertexBindingBuilder(sizeof(Vertex))
                    .add_vertex_attribute(VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, x))
                    .add_vertex_attribute(VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uvx))
                    .build()
                )
                .add_descriptor_set_layout(m_desc_layout)
                .set_pipeline_cache(m_pipeline_cache)
                .set_render_pass(renderer.default_render_pass())
                .set_label("RenderTexture::m_pipeline")
                .init(renderer);

        m_sets = m_desc_pool.allocate_sets({VulkanRenderer::MAX_FRAMES_IN_FLIGHT, m_desc_layout});
        m_color_attachments.resize(VulkanRenderer::MAX_FRAMES_IN_FLIGHT);
        m_color_attachment_views.resize(VulkanRenderer::MAX_FRAMES_IN_FLIGHT);
        m_framebuffers.resize(VulkanRenderer::MAX_FRAMES_IN_FLIGHT);

        m_render_pass = RenderPassInit()
                .add_attachment_description(AttachmentDescriptionBuilder()
                    .set_format(VK_FORMAT_B8G8R8A8_UNORM)
                    .set_sample_count(VK_SAMPLE_COUNT_1_BIT)
                    .set_load_and_store_op(
                        VK_ATTACHMENT_LOAD_OP_CLEAR,
                        VK_ATTACHMENT_STORE_OP_STORE)
                    .set_stencil_load_and_store_op(VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE)
                    .set_initial_and_final_image_layout(
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                    .build())
                .add_subpass_description(SubpassDescriptionBuilder()
                    .set_pipeline_bind_point(VK_PIPELINE_BIND_POINT_GRAPHICS)
                    .add_color_attachment({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL})
                    .build())
                .init(renderer);

        m_sampler = SamplerInit().init(renderer);

        auto set_writer = DescriptorWriter();
        for(uint32_t i = 0; i < VulkanRenderer::MAX_FRAMES_IN_FLIGHT; i++) {
            m_color_attachments[i] = ImageInit()
                    .set_image_type(VK_IMAGE_TYPE_2D)
                    .set_usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
                    .set_format(VK_FORMAT_B8G8R8A8_UNORM)
                    .set_extent(WIDTH, HEIGHT)
                    .set_memory_usage(VMA_MEMORY_USAGE_GPU_ONLY)
                    .set_label("RenderTexture::m_color_attachment")
                    .init(renderer);

            m_color_attachment_views[i] = ImageViewInit()
                    .set_image(m_color_attachments[i])
                    .set_type(VK_IMAGE_VIEW_TYPE_2D)
                    .set_aspect_mask(VK_IMAGE_ASPECT_COLOR_BIT)
                    .set_label("RenderTexture::m_color_attachment_view")
                    .init(renderer);

            m_framebuffers[i] = FramebufferInit()
                    .set_extent(WIDTH, HEIGHT)
                    .attach_image_view(m_color_attachment_views[i])
                    .set_render_pass(m_render_pass)
                    .set_label("RenderTexture::m_framebuffer")
                    .init(renderer);

            set_writer.write_image(m_sets[i], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                   m_color_attachment_views[i], m_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        set_writer.commit_writes(renderer);
    }

    ~RenderTexture(){
        m_pipeline_cache.serialize(CACHE_PATH);
    }

    RenderPass render_pass() const { return m_render_pass; }

    void begin_render_pass(CommandBuffer& cmd, uint32_t current_frame){
        VkClearValue clear;
        clear.color = {0.2, 0.2, 0.2, 1.0};

        cmd.begin_render_pass(
            m_render_pass, m_framebuffers[current_frame],
            {clear}, {WIDTH, HEIGHT}
        );
    }

    void draw_texture(CommandBuffer& cmd, uint32_t current_frame){
        cmd
        .bind_pipeline(m_pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS)
        .bind_vertex_buffer(m_vertex_buffer)
        .bind_index_buffer(m_index_buffer, VK_INDEX_TYPE_UINT32)
        .bind_descriptor_sets(m_pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS, {m_sets[current_frame]})
        .draw_indexed(6, 1);
    }
private:
    Buffer<Vertex>   m_vertex_buffer;
    Buffer<uint32_t> m_index_buffer;

    Pipeline m_pipeline;
    PipelineCache m_pipeline_cache;

    DescriptorPool m_desc_pool;
    DescriptorSetLayout m_desc_layout;

    std::vector<DescriptorSet> m_sets;
    std::vector<Image> m_color_attachments;
    std::vector<ImageView> m_color_attachment_views;
    std::vector<Framebuffer> m_framebuffers;
    Sampler m_sampler;

    RenderPass m_render_pass;
};

int main(){
    auto app = AppInit()
            .set_window_extent({RenderTexture::WIDTH, RenderTexture::HEIGHT})
            .set_window_mode(WindowMode::WINDOWED)
            .set_resizable(false)
            .use_primary_monitor()
            .configure_vulkan_renderer([=](VulkanRendererInit& init){
                init.set_enabled_layers({"VK_LAYER_KHRONOS_validation"});
            }).init();


    auto render_texture = RenderTexture(app.renderer());

    Vertex vertices[] = {
            Vertex{ 0.0f, -0.5f}, // Top
            Vertex{ 0.5f,  0.5f}, // Right
            Vertex{-0.5f,  0.5f}, // Left
    };

    auto vertex_buffer = BufferInit<Vertex>()
            .set_label("vertex_buffer")
            .set_memory_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
            .set_usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
            .set_size(3)
            .set_data(vertices)
            .init(app.renderer());

    auto pipeline_cache = PipelineCache::load(app.renderer(), "../examples/render_texture/pipeline.cache");
    auto pipeline = GraphicsPipelineInit()
        .add_vertex_binding(VertexBindingBuilder(sizeof(Vertex))
            .add_vertex_attribute(VK_FORMAT_R32G32_SFLOAT,0)
            .build()
        ).attach_shader_module(ShaderModuleInit()
           .set_label("Vertex Shader")
           .set_src_from_file("../examples/render_texture/shader.vert.spv")
           .set_stage(VK_SHADER_STAGE_VERTEX_BIT)
           .init(app.renderer())
        ).attach_shader_module(ShaderModuleInit()
           .set_label("Fragment Shader")
           .set_src_from_file("../examples/render_texture/shader.frag.spv")
           .set_stage(VK_SHADER_STAGE_FRAGMENT_BIT)
           .init(app.renderer())
        ).set_render_pass(render_texture.render_pass())
        .set_pipeline_cache(pipeline_cache)
        .init(app.renderer());

    g_app::CommandBuffer command_buffers[g_app::VulkanRenderer::MAX_FRAMES_IN_FLIGHT] = {};
    for(auto & command_buffer : command_buffers){
        command_buffer = g_app::CommandBuffer(app.renderer());
    }

    app.main_loop([&](const std::vector<Event>& events, const Time& time){
        for(const auto& event : events){
            switch(event.type){
                case EventType::KEY:
                    if(event.key.key == GLFW_KEY_ESCAPE && event.key.action == GLFW_PRESS) app.window().quit();
                    break;
                default:
                    break;
            }
        }

        if(!app.renderer().acquire_next_swapchain_image()) return;

        command_buffers[app.renderer().current_frame()]
                .begin()
                /* begin_render_pass(...) */ .cmd([&](CommandBuffer& cmd){
                    render_texture.begin_render_pass(cmd, app.renderer().current_frame());
                })
                // Draw to render_texture
                .bind_pipeline(pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS)
                .bind_vertex_buffer(vertex_buffer)
                .draw(3, 1)

                .end_render_pass()

                .begin_default_render_pass(.0, .0, .0, 1.)
                // Draw to screen
                .cmd([&](CommandBuffer& cmd){
                    render_texture.draw_texture(cmd, app.renderer().current_frame());
                })

                .end_render_pass()

                .end()
                .submit(g_app::Queue::GRAPHICS, {
                        {app.renderer().current_image_available_semaphore()},
                        {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
                        {app.renderer().current_render_finished_semaphore()},
                        app.renderer().current_in_flight_fence()
                });

        app.renderer().present();
    });

    app.renderer().device_wait_idle();
    pipeline_cache.serialize("../examples/render_texture/pipeline.cache");

    return 0;
}