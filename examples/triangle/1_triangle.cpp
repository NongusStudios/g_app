//
// Created by jandr on 31/10/2023.
//
#include <g_app.hpp>

int main(){
    g_app::App app = g_app::AppInit()
        .set_window_extent({800, 600})
        .set_window_mode(g_app::WindowMode::WINDOWED)
        .set_resizable(true)
        .set_window_title("1 - Triangle")
        .use_primary_monitor()
        .configure_vulkan_renderer([&](g_app::VulkanRendererInit& init){
            init.set_app_name("1 - Triangle")
                .set_engine_name("g_app");
        })
        .init();

    app.main_loop([&](){

    });
}