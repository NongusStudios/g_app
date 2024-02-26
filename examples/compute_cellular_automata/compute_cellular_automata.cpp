//
// Created by jandr on 19/02/2024.
//

#include <g_app.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Vertex {
    float x, y;
    float uvx, uvy;
};

enum CellType {
    CELL_TYPE_AIR,
    CELL_TYPE_SOLID,
    CELL_TYPE_SAND,
};

struct Cell {
    int type;
    int has_moved;
};

const int GRID_SIZE_X = 256;
const int GRID_SIZE_Y = 256;

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
            .set_size(4)
            .set_data(vertices)
            .set_usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
            .set_memory_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
            .init(app.renderer());

    auto index_buffer = g_app::BufferInit<uint32_t>()
            .set_size(6)
            .set_data(indices)
            .set_usage(VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
            .set_memory_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
            .init(app.renderer());

    auto cell_buffer = g_app::BufferInit<Cell>()
            .set_size(GRID_SIZE_X*GRID_SIZE_Y)
            .set_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
            .set_memory_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
            .init(app.renderer());

    auto desc_pool = g_app::DescriptorPoolInit()
            .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2)
            .set_max_sets(2)
            .init(app.renderer());

    auto comp_desc_layout = g_app::DescriptorSetLayoutInit()
            .add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
            .init(app.renderer());

    auto desc_layout = g_app::DescriptorSetLayoutInit()
            .add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT)
            .init(app.renderer());

    auto comp_set = desc_pool.allocate_set(comp_desc_layout);
    auto set = desc_pool.allocate_set(desc_layout);

    // Write sets
    g_app::DescriptorWriter()
        .write_buffer(comp_set, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, cell_buffer)
        .write_buffer(set, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, cell_buffer)
        .commit_writes(app.renderer());

    Cell* cell_data = cell_buffer.map();

    for(size_t x = 0; x < GRID_SIZE_X; x++){
        cell_data[x + (GRID_SIZE_Y-1)*GRID_SIZE_X].type = CELL_TYPE_SOLID;
    }

    for(size_t x = 24; x < 60; x++){
        for(size_t y = 60; y < 128; y++){
            cell_data[x + y*GRID_SIZE_X].type = CELL_TYPE_SAND;
        }
    }

    auto comp_pipeline = g_app::ComputePipelineInit()
            .add_descriptor_set_layout(comp_desc_layout)
            .set_shader_module(g_app::ShaderModuleInit()
                .set_stage(VK_SHADER_STAGE_COMPUTE_BIT)
                .set_src_from_file("../examples/compute_cellular_automata/cell.comp.spv")
                .init(app.renderer())
            ).init(app.renderer());

    auto pipeline = g_app::GraphicsPipelineInit()
            .add_descriptor_set_layout(desc_layout)
            .add_vertex_binding(g_app::VertexBindingBuilder(sizeof(Vertex))
                .add_vertex_attribute(VK_FORMAT_R32G32_SFLOAT, 0)
                .add_vertex_attribute(VK_FORMAT_R32G32_SFLOAT, sizeof(float)*2)
                .build())
            .attach_shader_module(g_app::ShaderModuleInit()
                .set_stage(VK_SHADER_STAGE_VERTEX_BIT)
                .set_src_from_file("../examples/compute_cellular_automata/cell.vert.spv")
                .init(app.renderer()))
            .attach_shader_module(g_app::ShaderModuleInit()
                .set_stage(VK_SHADER_STAGE_FRAGMENT_BIT)
                .set_src_from_file("../examples/compute_cellular_automata/cell.frag.spv")
                .init(app.renderer()))
            .init(app.renderer());

    g_app::CommandBuffer command_buffers[g_app::VulkanRenderer::MAX_FRAMES_IN_FLIGHT] = {};
    for(auto & command_buffer : command_buffers){
        command_buffer = g_app::CommandBuffer(app.renderer());
    }


    app.main_loop([&](const std::vector<g_app::Event>& events, const g_app::Time& time){
        if(!app.renderer().acquire_next_swapchain_image()) return;

        command_buffers[app.renderer().current_frame()]
                .begin()

                .bind_pipeline(comp_pipeline, VK_PIPELINE_BIND_POINT_COMPUTE)
                .bind_descriptor_sets(comp_pipeline, VK_PIPELINE_BIND_POINT_COMPUTE, {comp_set})
                .dispatch(256/GRID_SIZE_X, 256/GRID_SIZE_Y, 1)
                .pipeline_barrier(g_app::PipelineBarrierInfoBuilder()
                    .set_stage_flags(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT)
                    .add_buffer_memory_barrier(cell_buffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT)
                    .build())

                .begin_default_render_pass(0.2f, 0.2f, 0.2f, 1.0f)

                .bind_pipeline(pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS)
                .bind_descriptor_sets(pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS, {set})
                .bind_vertex_buffer(vertex_buffer)
                .bind_index_buffer(index_buffer, VK_INDEX_TYPE_UINT32)
                .draw_indexed(6, GRID_SIZE_X*GRID_SIZE_Y)

                .end_render_pass()
                .end()

                .submit(g_app::Queue::GRAPHICS, {
                        {app.renderer().current_image_available_semaphore()}, {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
                        {app.renderer().current_render_finished_semaphore()},
                        app.renderer().current_in_flight_fence()});
        app.renderer().present();
    });

    app.renderer().device_wait_idle();
}