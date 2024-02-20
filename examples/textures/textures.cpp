//
// Created by jandr on 31/10/2023.
//
#include <g_app.hpp>

#include <iostream>

struct Vertex {
    float x, y;
    float uvx, uvy;
};

int main(){
    g_app::App app = g_app::AppInit()
        .set_window_extent({800, 600})
        .set_window_mode(g_app::WindowMode::WINDOWED)
        .set_resizable(true)
        .set_window_title("Triangle")
        .use_primary_monitor()
        .configure_vulkan_renderer([&](g_app::VulkanRendererInit& init){
            init.set_app_name("Triangle")
                .set_engine_name("g_app")
                .set_enabled_layers({"VK_LAYER_KHRONOS_validation"});
        })
        .init();

    Vertex vertices[] = {
            Vertex{-1.0f, -1.0f, 0.0f, 0.0f }, // Top Left
            Vertex{ 1.0f, -1.0f, 1.0f, 0.0f }, // Top Right
            Vertex{ 1.0f,  1.0f, 1.0f, 1.0f }, // Bottom Right
            Vertex{-1.0f,  1.0f, 0.0f, 1.0f }, // Bottom Left
    };
    uint32_t indices[] = {
            0, 1, 3, // #1
            1, 2, 3, // #2
    };

    auto vertex_buffer = g_app::BufferInit<Vertex>()
            .set_label("Vertex Buffer")
            .set_usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
            .set_memory_usage(VMA_MEMORY_USAGE_GPU_ONLY)
            .set_size(4)
            .set_data(nullptr)
            .init(app.renderer());

    auto index_buffer = g_app::BufferInit<uint32_t>()
            .set_label("Index Buffer")
            .set_usage(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
            .set_memory_usage(VMA_MEMORY_USAGE_GPU_ONLY)
            .set_size(6)
            .set_data(nullptr)
            .init(app.renderer());

    g_app::CommandBuffer(app.renderer())
        .begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)
        .copy_buffer(
            g_app::BufferInit<Vertex>()
                .set_label("Vertex Staging Buffer")
                .set_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
                .set_memory_usage(VMA_MEMORY_USAGE_CPU_ONLY)
                .set_size(4)
                .set_data(vertices)
                .init(app.renderer()),
            vertex_buffer)
        .copy_buffer(
            g_app::BufferInit<uint32_t>()
                .set_label("Index Staging Buffer")
                .set_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
                .set_memory_usage(VMA_MEMORY_USAGE_CPU_ONLY)
                .set_size(6)
                .set_data(indices)
                .init(app.renderer()),
            index_buffer)
        .submit(g_app::Queue::TRANSFER);

    auto [tex_image, tex_view] = g_app::TextureInit()
        .set_label("GruvWin Texture")
        .load_from_file("../examples/textures/gruvwin.png", STBI_rgb_alpha)
        .set_format(VK_FORMAT_R8G8B8A8_UNORM, 4)
        .init(app.renderer());

    auto sampler = g_app::SamplerInit()
            .init(app.renderer());

    auto desc_pool = g_app::DescriptorPoolInit()
            .set_label("Descriptor Pool")
            .set_max_sets(g_app::VulkanRenderer::MAX_FRAMES_IN_FLIGHT)
            .add_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)
            .init(app.renderer());

    auto desc_layout = g_app::DescriptorSetLayoutInit()
            .set_label("Descriptor Layout")
            .add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
            .init(app.renderer());

    auto desc_sets = desc_pool.allocate_sets({g_app::VulkanRenderer::MAX_FRAMES_IN_FLIGHT, desc_layout});

    auto pipeline = g_app::GraphicsPipelineInit()
            .add_descriptor_set_layout(desc_layout)
            .add_vertex_binding(g_app::VertexBindingBuilder(sizeof(Vertex))
                .add_vertex_attribute(VK_FORMAT_R32G32_SFLOAT,    0)
                .add_vertex_attribute(VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uvx))
                .build()
            ).attach_shader_module(g_app::ShaderModuleInit()
                .set_label("Vertex Shader")
                .set_src_from_file("../examples/textures/shader.vert.spv")
                .set_stage(VK_SHADER_STAGE_VERTEX_BIT)
                .init(app.renderer())
            ).attach_shader_module(g_app::ShaderModuleInit()
                .set_label("Fragment Shader")
                .set_src_from_file("../examples/textures/shader.frag.spv")
                .set_stage(VK_SHADER_STAGE_FRAGMENT_BIT)
                .init(app.renderer())
            ).set_render_pass(app.renderer().default_render_pass())
            .init(app.renderer());

    g_app::CommandBuffer command_buffers[g_app::VulkanRenderer::MAX_FRAMES_IN_FLIGHT] = {};
    {
        auto writer = g_app::DescriptorWriter();
        uint32_t i = 0;
        for (auto &command_buffer: command_buffers) {
            command_buffer = g_app::CommandBuffer(app.renderer());
            writer.write_image(desc_sets[i], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               tex_view, sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            i++;
        }
        writer.commit_writes(app.renderer());
    }

    app.main_loop([&](const g_app::Time& time){
        if(!app.renderer().acquire_next_swapchain_image()) return;

        uint32_t frame = app.renderer().current_frame();
        command_buffers[frame]
            .begin()

            .begin_default_render_pass(0.2f, 0.2f, 0.2f, 1.0f)
                .bind_vertex_buffer(vertex_buffer)
                .bind_index_buffer(index_buffer, VK_INDEX_TYPE_UINT32)
                .bind_graphics_pipeline(pipeline)
                .bind_descriptor_sets(pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS, {desc_sets[frame]})
                .draw_indexed(6, 1)
            .end_render_pass()

            .end()
            .submit(g_app::Queue::GRAPHICS,
                    {app.renderer().current_image_available_semaphore()}, {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
                    {app.renderer().current_render_finished_semaphore()},
                    app.renderer().current_in_flight_fence());
        app.renderer().present();
    });

    app.renderer().device_wait_idle();

    return 0;
}