//
// Created by jandr on 31/10/2023.
//
#include <g_app.hpp>

#include <iostream>

struct Vertex {
    float x, y;
    float r, g, b;
};

void draw_ui(float x, float y){
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Position");
    ImGui::Text("%f, %f", x, y);

    ImGui::End();
}

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

    app.renderer().init_imgui();

    Vertex vertices[] = {
            Vertex{ 0.0f, -0.5f, 1.0f, 0.0f, 0.0f}, // Top
            Vertex{ 0.5f,  0.5f, 0.0f, 1.0f, 0.0f}, // Right
            Vertex{-0.5f,  0.5f, 0.0f, 0.0f, 1.0f}, // Left
    };

    auto vertex_buffer = g_app::BufferInit<Vertex>()
            .set_label("Vertex Buffer")
            .set_usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
            .set_memory_usage(VMA_MEMORY_USAGE_GPU_ONLY)
            .set_size(3)
            .set_data(nullptr)
            .init(app.renderer());

    g_app::CommandBuffer(app.renderer())
        .begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)
        .copy_buffer(
            g_app::BufferInit<Vertex>()
                .set_label("Vertex Staging Buffer")
                .set_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
                .set_memory_usage(VMA_MEMORY_USAGE_CPU_ONLY)
                .set_size(3)
                .set_data(vertices)
                .init(app.renderer()),
            vertex_buffer
        ).submit(g_app::Queue::TRANSFER);

    auto pipeline = g_app::GraphicsPipelineInit()
            .add_push_constant_range({VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float)*2})
            .add_vertex_binding(g_app::VertexBindingBuilder(sizeof(Vertex))
                .add_vertex_attribute(VK_FORMAT_R32G32_SFLOAT,    0)
                .add_vertex_attribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, r))
                .build()
            ).attach_shader_module(g_app::ShaderModuleInit()
                .set_label("Vertex Shader")
                .set_src_from_file("../examples/triangle/shader.vert.spv")
                .set_stage(VK_SHADER_STAGE_VERTEX_BIT)
                .init(app.renderer())
            ).attach_shader_module(g_app::ShaderModuleInit()
                .set_label("Fragment Shader")
                .set_src_from_file("../examples/triangle/shader.frag.spv")
                .set_stage(VK_SHADER_STAGE_FRAGMENT_BIT)
                .init(app.renderer())
            ).set_render_pass(app.renderer().default_render_pass())
            .init(app.renderer());

    g_app::CommandBuffer command_buffers[g_app::VulkanRenderer::MAX_FRAMES_IN_FLIGHT] = {};
    for(auto & command_buffer : command_buffers){
        command_buffer = g_app::CommandBuffer(app.renderer());
    }

    float offset[] = {0.0f, 0.0f};
    float dir[] = {1.0f, -1.0f};

    app.main_loop([&](const g_app::Time& time){
        offset[0] += dir[0] * time.deltaf;
        offset[1] += dir[1] * time.deltaf * 0.8f;
        if(offset[0] >= 0.5f || offset[0] <= -0.5f) dir[0] *= -1.0f;
        if(offset[1] >= 0.5f || offset[1] <= -0.5f) dir[1] *= -1.0f;

        draw_ui(offset[0], offset[1]);

        if(!app.renderer().acquire_next_swapchain_image()) return;

        command_buffers[app.renderer().current_frame()]
            .begin()

            .begin_default_render_pass(0.2f, 0.2f, 0.2f, 1.0f)
                .bind_vertex_buffer(vertex_buffer)
                .bind_graphics_pipeline(pipeline)
                .push_constants(pipeline, VK_SHADER_STAGE_VERTEX_BIT, offset)
                .draw(3, 1, 0, 0)
                .draw_imgui()
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

    return 0;
}