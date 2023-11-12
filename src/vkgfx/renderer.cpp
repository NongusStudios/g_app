//
// Created by jandr on 6/11/2023.
//

#include "../include/vkgfx/renderer.hpp"
#include <spdlog/spdlog.h>

#include <format>
#include <cstring>

namespace g_app {
    bool is_layers_supported(const std::vector<const char*>& layers){
        uint32_t all_layer_count = 0;
        vkEnumerateInstanceLayerProperties(&all_layer_count, nullptr);
        std::vector<VkLayerProperties> all_layer_properties(all_layer_count);
        vkEnumerateInstanceLayerProperties(&all_layer_count, all_layer_properties.data());

        for(const auto& layer : layers){
            bool supported = false;
            for(const auto& _layer : all_layer_properties){
                if(strcmp(layer, _layer.layerName) == 0) { supported = true; break; }
            }
            if(!supported) return false;
        }
        return true;
    }

    bool is_instance_extensions_supported(const std::vector<const char*>& extensions){
        uint32_t all_extension_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &all_extension_count, nullptr);
        std::vector<VkExtensionProperties> all_extension_properties(all_extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &all_extension_count, all_extension_properties.data());

        for(const auto& extension : extensions){
            bool supported = false;
            for(const auto& ext : all_extension_properties){
                if(strcmp(extension, ext.extensionName) == 0) { supported = true; break; }
            }
            if(!supported) return false;
        }

        return true;
    }

    VulkanRenderer::VulkanRenderer(GLFWwindow* window, const Config& config): self{std::make_shared<Inner>()} {
        init_instance(config);
        pick_physical_device();
        init_device(config);
    }

    void VulkanRenderer::init_instance(const Config& config){
        VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
        app_info.apiVersion = config.api_version;
        app_info.applicationVersion = config.app_version;
        app_info.engineVersion = config.engine_version;
        app_info.pApplicationName = config.app_name.c_str();
        app_info.pEngineName = config.engine_name.c_str();

        VkInstanceCreateInfo info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        info.pApplicationInfo = &app_info;

        if(is_layers_supported(config.enabled_layers)){
            info.enabledLayerCount = static_cast<uint32_t>(config.enabled_layers.size());
            info.ppEnabledLayerNames = config.enabled_layers.data();
        } else throw std::runtime_error("Enabled validation layers not supported, continuing without validation.");

        uint32_t req_instance_extension_count = 0;
        const char** req_exts = glfwGetRequiredInstanceExtensions(&req_instance_extension_count);

        std::vector<const char*> extensions(req_instance_extension_count);
        memcpy(extensions.data(), req_exts, req_instance_extension_count*sizeof(const char*));

        if(is_instance_extensions_supported(extensions)){
            info.enabledExtensionCount = req_instance_extension_count;
            info.ppEnabledExtensionNames = req_exts;
        } else throw std::runtime_error("Required instance extensions not supported on this device. Unable to initialise Vulkan.");

        VkResult result = VK_SUCCESS;
        if((result = vkCreateInstance(&info, nullptr, &self->instance)) != VK_SUCCESS){
            throw std::runtime_error(std::format("Failed to create a Vulkan Instance! e({})", static_cast<uint32_t>(result)));
        }
    }

    bool is_physical_device_features_supported(VkPhysicalDevice device, VkPhysicalDeviceFeatures enabled_features){
        VkPhysicalDeviceFeatures features = {};
        vkGetPhysicalDeviceFeatures(device, &features);

        auto pfeatures = reinterpret_cast<VkBool32*>(&features);
        auto penabled_features = reinterpret_cast<VkBool32*>(&enabled_features);

        for(size_t i = 0; i < sizeof(VkPhysicalDeviceFeatures)/sizeof(VkBool32); i++){
            if(penabled_features && !pfeatures) return false;
            pfeatures++;
            penabled_features++;
        }

        return true;
    }

    bool is_physical_device_suitable(VkPhysicalDevice device){
        
        return true;
    }

    uint32_t score_physical_device(VkPhysicalDevice device){
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(device, &properties);

        if(!is_physical_device_suitable(device)) return 0;

        uint32_t score = 1;
        switch(properties.deviceType){
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                score += 1;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                score += 10;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                score += 100;
                break;
            default:
                break;
        }

        score += properties.limits.maxImageDimension2D;

        return score;
    }

    void VulkanRenderer::pick_physical_device() {
        uint32_t physical_device_count = 0;
        vkEnumeratePhysicalDevices(self->instance, &physical_device_count, nullptr);
        std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
        vkEnumeratePhysicalDevices(self->instance, &physical_device_count, physical_devices.data());

        uint32_t highest_score = 0;
        VkPhysicalDevice chosen_device = VK_NULL_HANDLE;
        for(auto& physical_device: physical_devices){
            uint32_t score = score_physical_device(physical_device);
            if(score > highest_score){
                highest_score = score;
                chosen_device = physical_device;
            }
        }

        self->physical_device = chosen_device;
    }
} // g_app