//
// Created by jandr on 20/11/2023.
//
#include <g_app.hpp>
#include "../glm/glm/glm.hpp"
#include "../glm/glm/gtc/matrix_transform.hpp"

using namespace g_app;

struct Vertex {
    glm::vec3 a_pos;
    glm::vec3 a_color;
};

struct TransformData {
    glm::mat4 model;
    glm::mat4 projection;
};

int main(){
    auto app = AppInit()
            .set_window_extent({800, 600})
            .set_window_mode(g_app::WindowMode::WINDOWED)
            .set_resizable(true)
            .set_window_title("Cube")
            .use_primary_monitor()
            .configure_vulkan_renderer([&](g_app::VulkanRendererInit& init){
                init.set_app_name("Cube")
                        .set_engine_name("g_app")
                        .set_enabled_layers({"VK_LAYER_KHRONOS_validation"});
            })
            .init();
    app.renderer().init_imgui();

    auto descriptor_pool = DescriptorPoolInit()
            .set_label("Descriptor Pool")
            .set_max_sets(VulkanRenderer::MAX_FRAMES_IN_FLIGHT)
            .add_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VulkanRenderer::MAX_FRAMES_IN_FLIGHT)
            .init(app.renderer());

    auto descriptor_set_layout = DescriptorSetLayoutInit()
            .set_label("Set Layout")
            .add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                         VK_SHADER_STAGE_VERTEX_BIT)
            .init(app.renderer());

    std::vector<Buffer<TransformData>> uniform_buffers = {};
    uniform_buffers.reserve(VulkanRenderer::MAX_FRAMES_IN_FLIGHT);

    for(uint32_t i = 0; i < VulkanRenderer::MAX_FRAMES_IN_FLIGHT; i++){
        uniform_buffers.push_back(BufferInit<TransformData>()
                                          .set_label(std::format("Uniform Buffer {}", i))
                                          .set_memory_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
                                          .set_size(1)
                                          .set_usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
                                          .init(app.renderer())
        );
    }

    auto descriptor_sets = descriptor_pool.allocate_sets(
            {VulkanRenderer::MAX_FRAMES_IN_FLIGHT, descriptor_set_layout}
    );

    auto set_writer = DescriptorWriter();
    uint32_t i = 0;
    for(const auto& set : descriptor_sets){
        set_writer.write_buffer(set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniform_buffers[i]);
        i++;
    }
    set_writer.commit_writes(app.renderer());

    const uint32_t vertex_count = 8;
    glm::vec3 cube_color_a = {0.4f, 1.0f, 0.2f};
    glm::vec3 cube_color_b = {1.0f, 0.4f, 0.2f};
    Vertex vertices[] = {
            { {-1.0f, -1.0f,  1.0f}, cube_color_a },
            { { 1.0f, -1.0f,  1.0f}, cube_color_a },
            { { 1.0f,  1.0f,  1.0f}, cube_color_a },
            { {-1.0f,  1.0f,  1.0f}, cube_color_a },
            { {-1.0f, -1.0f, -1.0f}, cube_color_b },
            { { 1.0f, -1.0f, -1.0f}, cube_color_b },
            { { 1.0f,  1.0f, -1.0f}, cube_color_b },
            { {-1.0f,  1.0f, -1.0f}, cube_color_b }
    };

    const uint32_t index_count = 36;
    uint32_t indices[] = {
            0, 1, 3, 3, 1, 2,
            1, 5, 2, 2, 5, 6,
            5, 4, 6, 6, 4, 7,
            4, 0, 7, 7, 0, 3,
            3, 2, 7, 7, 2, 6,
            4, 5, 0, 0, 5, 1
    };

    auto vertex_buffer = BufferInit<Vertex>()
            .set_label("Vertex Buffer")
            .set_usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
            .set_memory_usage(VMA_MEMORY_USAGE_GPU_ONLY)
            .set_size(vertex_count)
            .init(app.renderer());

    auto index_buffer = BufferInit<uint32_t>()
            .set_label("Index Buffer")
            .set_usage(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
            .set_memory_usage(VMA_MEMORY_USAGE_GPU_ONLY)
            .set_size(index_count)
            .init(app.renderer());

    CommandBuffer(app.renderer())
            .begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)
            .copy_buffer(
                    BufferInit<Vertex>()
                            .set_label("Vertex Staging Buffer")
                            .set_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
                            .set_memory_usage(VMA_MEMORY_USAGE_CPU_ONLY)
                            .set_size(vertex_count)
                            .set_data(vertices)
                            .init(app.renderer()),
                    vertex_buffer)
            .copy_buffer(
                    BufferInit<uint32_t>()
                            .set_label("Index Staging Buffer")
                            .set_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
                            .set_memory_usage(VMA_MEMORY_USAGE_CPU_ONLY)
                            .set_size(index_count)
                            .set_data(indices)
                            .init(app.renderer()),
                    index_buffer)
            .submit(Queue::TRANSFER);

    RasterizationInfo rasterization_info = {};
    rasterization_info.cull_mode = VK_CULL_MODE_BACK_BIT;

    auto pipeline_cache = PipelineCache::load(app.renderer(), "cube_pipeline.cache");
    auto pipeline = GraphicsPipelineInit()
            .set_label("Cube Pipeline")
            .set_rasterization_info(rasterization_info)
            .add_descriptor_set_layout(descriptor_set_layout)
            .add_vertex_binding(VertexBindingBuilder(sizeof(Vertex))
                                        .add_vertex_attribute(VK_FORMAT_R32G32B32_SFLOAT, 0)
                                        .add_vertex_attribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, a_color))
                                        .build())
            .attach_shader_module(ShaderModuleInit()
                                          .set_label("Cube Pipeline Vertex Shader")
                                          .set_src_from_file("../examples/cube/shader.vert.spv")
                                          .set_stage(VK_SHADER_STAGE_VERTEX_BIT)
                                          .init(app.renderer()))
            .attach_shader_module(ShaderModuleInit()
                                          .set_label("Cube Pipeline Vertex Shader")
                                          .set_src_from_file("../examples/cube/shader.frag.spv")
                                          .set_stage(VK_SHADER_STAGE_FRAGMENT_BIT)
                                          .init(app.renderer()))
            .set_render_pass(app.renderer().default_render_pass())
            .set_pipeline_cache(pipeline_cache)
            .init(app.renderer());

    CommandBuffer cmd[VulkanRenderer::MAX_FRAMES_IN_FLIGHT];
    for(auto & c : cmd){
        c = CommandBuffer(app.renderer());
    }


    glm::vec3 position = {0.0f, 0.0f, -10.0f};
    glm::vec3 rotation = {0.0f, 0.2f, 0.0f};
    float scale = 1.0f;
    TransformData transform = {};

    app.main_loop([&](const std::vector<Event>& events, const Time& time){
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Transform");
        ImGui::SliderFloat("x", &position.x, -20.0f, 20.0f);
        ImGui::SliderFloat("y", &position.y, -20.0f, 20.0f);
        ImGui::SliderFloat("z", &position.z, -20.0f, 20.0f);

        ImGui::SliderFloat("rx", &rotation.x, -20.0f, 20.0f);
        ImGui::SliderFloat("ry", &rotation.y, -20.0f, 20.0f);
        ImGui::SliderFloat("rz", &rotation.z, -20.0f, 20.0f);

        ImGui::SliderFloat("Scale", &scale, 0.1f, 10.0f);
        ImGui::End();
        ImGui::Render();

        auto window_extent = app.window().extent();
        transform.projection = glm::perspective(45.0f, float(window_extent.width)/float(window_extent.height), 0.1f, 100.0f);

        auto model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, rotation.y, {0.0f, 1.0f, 0.0f});
        model = glm::rotate(model, rotation.x, {1.0f, 0.0f, 0.0f});
        model = glm::rotate(model, rotation.z, {0.0f, 0.0f, 1.0f});
        model = glm::scale(model, {scale, scale, scale});
        transform.model = model;

        // Upload uniform data
        auto data = uniform_buffers[app.renderer().current_frame()].map();
        *data = transform;
        uniform_buffers[app.renderer().current_frame()].unmap();

        if(!app.renderer().acquire_next_swapchain_image()) return;

        cmd[app.renderer().current_frame()]
                .begin()
                .begin_default_render_pass(0.2f, 0.2f, 0.2f, 1.0f)
                .bind_pipeline(pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS)
                .bind_vertex_buffer(vertex_buffer)
                .bind_index_buffer(index_buffer, VK_INDEX_TYPE_UINT32)
                .bind_descriptor_sets(pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                      {descriptor_sets[app.renderer().current_frame()]}
                ).draw_indexed(index_count, 1)
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

    pipeline_cache.serialize("cube_pipeline.cache");
    app.renderer().device_wait_idle();
    return 0;
}
