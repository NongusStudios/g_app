//
// Created by jandr on 6/11/2023.
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

#include "../include/vkgfx/renderer.hpp"
#include <spdlog/spdlog.h>

#include <format>
#include <cstring>
#include <limits>
#include <algorithm>

namespace g_app {
    VulkanRenderer::VulkanRenderer(GLFWwindow* window, const Config& config): self{std::make_shared<Inner>()} {
        self->window = window;
        init_instance(config);
        init_surface();
        pick_physical_device(config);
        init_device(config);
        init_allocator(config);
        init_command_pool();
        init_swapchain(VK_NULL_HANDLE);
        init_default_render_pass();
        init_framebuffers();
        init_sync_objects();
    }

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
            throw std::runtime_error(std::format("Failed to create the Instance! result = {}", static_cast<uint32_t>(result)));
        }
    }

    void VulkanRenderer::init_surface() {
        VkResult result = VK_SUCCESS;
        if((result = glfwCreateWindowSurface(self->instance, self->window, nullptr, &self->surface)) != VK_SUCCESS){
            throw std::runtime_error(std::format("Failed to create a window surface! result = {}", static_cast<uint32_t>(result)));
        }
    }

    bool is_physical_device_features_supported(VkPhysicalDevice device, const VkPhysicalDeviceFeatures& enabled_features){
        VkPhysicalDeviceFeatures features = {};
        vkGetPhysicalDeviceFeatures(device, &features);

        auto pfeatures = reinterpret_cast<const VkBool32*>(&features);
        auto penabled_features = reinterpret_cast<const VkBool32*>(&enabled_features);

        for(size_t i = 0; i < sizeof(VkPhysicalDeviceFeatures)/sizeof(VkBool32); i++){
            if(penabled_features && !pfeatures) return false;
            pfeatures++;
            penabled_features++;
        }

        return true;
    }

    std::optional<QueueFamilyInfo> find_queue_family(VkPhysicalDevice device, VkSurfaceKHR surface){
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        uint32_t chosen_family = 0;
        uint32_t queue_count = 0;
        uint32_t family_index = 0;

        uint32_t highest_queue_count = 0;

        bool family_found = false;
        for(const auto& queue_family : queue_families){
            if( queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
                queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT &&
                queue_family.queueCount > highest_queue_count
            ){
                VkBool32 present_support = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, family_index, surface, &present_support);
                if(!present_support) continue;

                chosen_family = family_index;
                queue_count = queue_family.queueCount;
                highest_queue_count = queue_family.queueCount;
                family_found = true;
            }
            family_index++;
        }

        if(!family_found){
            return {};
        }
        return std::make_optional<QueueFamilyInfo>(chosen_family, queue_count);
    }

    bool is_device_extensions_supported(VkPhysicalDevice device, const std::vector<const char*>& extensions){
        uint32_t extension_count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
        std::vector<VkExtensionProperties> extension_properties(extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extension_properties.data());

        for(auto& ext : extensions){
            bool supported = false;
            for(auto& prop : extension_properties){
                if(strcmp(ext, prop.extensionName) == 0) supported = true;
            }
            if(!supported) return false;
        }

        return true;
    }

    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    SwapchainSupportDetails get_swapchain_support_details(VkPhysicalDevice device, VkSurfaceKHR surface){
        SwapchainSupportDetails details = {};

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());

        uint32_t present_mode_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());

        return details;
    }

    bool is_swapchain_adequate(VkPhysicalDevice device, VkSurfaceKHR surface){
        auto details = get_swapchain_support_details(device, surface);
        return !details.formats.empty() && !details.present_modes.empty();
    }

    bool is_physical_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface, const VkPhysicalDeviceFeatures& enabled_features,
                                     const std::vector<const char*>& extensions){
        return find_queue_family(device, surface).has_value() && is_physical_device_features_supported(device, enabled_features) &&
               is_device_extensions_supported(device, extensions) && is_swapchain_adequate(device, surface);
    }

    uint32_t score_physical_device(VkPhysicalDevice device, VkSurfaceKHR surface, const VkPhysicalDeviceFeatures& enabled_features,
                                   const std::vector<const char*>& extensions){
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(device, &properties);

        if(!is_physical_device_suitable(device, surface, enabled_features, extensions)) return 0;

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

    void VulkanRenderer::pick_physical_device(const VulkanRenderer::Config &config) {
        std::vector<const char*> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        for(auto& ext : config.enabled_device_extensions){
            device_extensions.push_back(ext);
        }

        uint32_t physical_device_count = 0;
        vkEnumeratePhysicalDevices(self->instance, &physical_device_count, nullptr);
        std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
        vkEnumeratePhysicalDevices(self->instance, &physical_device_count, physical_devices.data());

        uint32_t highest_score = 0;
        VkPhysicalDevice chosen_device = VK_NULL_HANDLE;
        for(auto& physical_device: physical_devices){
            uint32_t score = score_physical_device(physical_device, self->surface, config.enabled_features, device_extensions);
            if(score > highest_score){
                highest_score = score;
                chosen_device = physical_device;
            }
        }

        if(chosen_device == VK_NULL_HANDLE){
            throw std::runtime_error("Failed to find a suitable GPU!");
        }

        self->physical_device = chosen_device;

        auto device_props = this->physical_device_properties();
        spdlog::info("physical_device = {}", device_props.deviceName);
    }

    void VulkanRenderer::init_device(const VulkanRenderer::Config &config) {
        auto queue_family = find_queue_family(self->physical_device, self->surface);
        auto queue_count = (queue_family->count > MAX_QUEUE_COUNT) ? MAX_QUEUE_COUNT : queue_family->count;

        float priorities[] = {1.0f, 0.9f, 0.8f};

        VkDeviceQueueCreateInfo queue_create_info = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queue_create_info.queueFamilyIndex = queue_family->index;
        queue_create_info.queueCount = queue_count;
        queue_create_info.pQueuePriorities = priorities;

        auto device_features = this->physical_device_features();

        std::vector<const char*> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        device_extensions.insert(device_extensions.end(), config.enabled_device_extensions.begin(), config.enabled_device_extensions.end());

        VkDeviceCreateInfo create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        create_info.pQueueCreateInfos = &queue_create_info;
        create_info.queueCreateInfoCount = 1;
        create_info.pEnabledFeatures = &device_features;
        create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
        create_info.ppEnabledExtensionNames = device_extensions.data();
        create_info.enabledLayerCount = static_cast<uint32_t>(config.enabled_layers.size());
        create_info.ppEnabledLayerNames = config.enabled_layers.data();

        VkResult result = VK_SUCCESS;
        if( (result = vkCreateDevice(self->physical_device, &create_info, nullptr, &self->device)) != VK_SUCCESS){
            throw std::runtime_error(std::format("Failed to create the logical device! result = {}", static_cast<uint32_t>(result)));
        }

        self->queues.resize(queue_count);
        for(size_t i = 0; i < queue_count; i++) {
            vkGetDeviceQueue(self->device, queue_family->index, i, &self->queues[i]);
        }

        self->queue_family_info = queue_family.value();
    }

    void VulkanRenderer::init_allocator(const VulkanRenderer::Config &config) {
        VmaAllocatorCreateInfo create_info = {};
        create_info.vulkanApiVersion = config.api_version;
        create_info.instance = self->instance;
        create_info.physicalDevice = self->physical_device;
        create_info.device = self->device;

        VkResult result = VK_SUCCESS;
        if((result = vmaCreateAllocator(&create_info, &self->allocator)) != VK_SUCCESS){
            throw std::runtime_error(std::format("Failed to create the memory allocator! result = {}", static_cast<uint32_t>(result)));
        }
    }

    void VulkanRenderer::init_command_pool() {
        VkCommandPoolCreateInfo create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        create_info.queueFamilyIndex = self->queue_family_info.index;
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkResult result = VK_SUCCESS;
        if((result = vkCreateCommandPool(self->device, &create_info, nullptr, &self->command_pool)) != VK_SUCCESS){
            throw std::runtime_error(std::format("Failed to create the command pool! result = {}", static_cast<uint32_t>(result)));
        }
    }

    void VulkanRenderer::init_default_render_pass() {
        VkAttachmentDescription color_attachment = {};
        VkAttachmentReference color_ref = {};

        color_attachment.format = self->swapchain.format;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        color_ref.attachment = 0;
        color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depth_attachment = {};
        VkAttachmentReference  depth_ref = {};

        depth_attachment.format = self->swapchain.depth_resources.format;
        depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        depth_ref.attachment = 1;
        depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_ref;
        subpass.pDepthStencilAttachment = &depth_ref;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcAccessMask = 0;
        dependency.srcStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstSubpass = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkAttachmentDescription attachments[] = {color_attachment, depth_attachment};

        VkRenderPassCreateInfo create_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        create_info.attachmentCount = 2;
        create_info.pAttachments = attachments;
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass;
        create_info.dependencyCount = 1;
        create_info.pDependencies = &dependency;

        VkResult result = VK_SUCCESS;
        if((result = vkCreateRenderPass(self->device, &create_info, nullptr, &self->default_render_pass)) != VK_SUCCESS){
            throw std::runtime_error(std::format("Failed to create the default render pass! result = {}", static_cast<uint32_t>(result)));
        }
    }

    VkSurfaceFormatKHR select_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) {
        for(const auto& format : available_formats){
            if(format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }

        return available_formats[0];
    }

    VkPresentModeKHR select_presentation_mode(const std::vector<VkPresentModeKHR>& available_present_modes){
        for(const auto& present_mode : available_present_modes){
            if(present_mode == VK_PRESENT_MODE_MAILBOX_KHR){
                return present_mode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D select_swapchain_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window){
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actual_extent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
            };

            actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actual_extent;
        }
    }

    uint32_t select_image_count(const VkSurfaceCapabilitiesKHR& capabilities){
        uint32_t image_count = capabilities.minImageCount + 1;
        if(capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount){
            image_count = capabilities.maxImageCount;
        }
        return image_count;
    }

    void create_image_views(const std::shared_ptr<VulkanRenderer::Inner>& inner){
        inner->swapchain.image_views.resize(inner->swapchain.images.size());

        uint32_t i = 0;
        for(const auto& image : inner->swapchain.images){
            VkImageViewCreateInfo create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            create_info.image = image;
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format = inner->swapchain.format;
            create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            create_info.subresourceRange.baseMipLevel = 0;
            create_info.subresourceRange.levelCount = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount = 1;

            VkResult result = VK_SUCCESS;
            if((result = vkCreateImageView(inner->device, &create_info, nullptr, &inner->swapchain.image_views[i])) != VK_SUCCESS){
                throw std::runtime_error(
                        std::format("Failed to create swapchain image view [{}]! result = {}", i, static_cast<uint32_t>(result)));
            }

            i++;
        }
    }

    VkFormat find_depth_format(VkPhysicalDevice device){
        std::vector<VkFormat> depth_formats = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
        VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

        for(const auto& format : depth_formats){
            VkFormatProperties properties = {};
            vkGetPhysicalDeviceFormatProperties(device, format, &properties);
            if((properties.optimalTilingFeatures & features) == features){
                return format;
            }
        }

        throw std::runtime_error("Failed to find a suitable swapchain depth stencil format!");
    }

    void create_depth_resources(const std::shared_ptr<VulkanRenderer::Inner>& inner, const VkExtent2D& extent){
        VkFormat depth_format = find_depth_format(inner->physical_device);

        VulkanRenderer::SwapchainDepthResources& depth_resources = inner->swapchain.depth_resources;
        uint32_t image_count = inner->swapchain.images.size();
        depth_resources.images.resize(image_count);
        depth_resources.image_views.resize(image_count);
        depth_resources.allocations.resize(image_count);
        depth_resources.format = depth_format;

        for(uint32_t i = 0; i < image_count; i++){
            VkImageCreateInfo image_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            image_info.imageType = VK_IMAGE_TYPE_2D;
            image_info.extent = {extent.width, extent.height, 1};
            image_info.mipLevels = 1;
            image_info.arrayLayers = 1;
            image_info.format = depth_format;
            image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
            image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            image_info.samples = VK_SAMPLE_COUNT_1_BIT;
            image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            image_info.flags = 0;

            VmaAllocationCreateInfo alloc_info = {};
            alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            VkResult result = VK_SUCCESS;
            if((result =
                vmaCreateImage(inner->allocator, &image_info, &alloc_info,
                               &depth_resources.images[i], &depth_resources.allocations[i], nullptr))
                != VK_SUCCESS)
            {
                throw std::runtime_error(
                        std::format("Failed to create swapchain depth image {}! result = {}", i, static_cast<uint32_t>(result)));
            }

            VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            view_info.image = depth_resources.images[i];
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format = depth_format;
            view_info.subresourceRange = {
                    VK_IMAGE_ASPECT_DEPTH_BIT,
                    0, 1, 0, 1,
            };

            if((result =
                vkCreateImageView(inner->device, &view_info, nullptr, &depth_resources.image_views[i])) != VK_SUCCESS)
            {
                throw std::runtime_error(
                        std::format("Failed to create swapchain depth image view {}! result = {}", i, static_cast<uint32_t>(result)));
            }
        }
    }

    void VulkanRenderer::init_swapchain(VkSwapchainKHR old_swapchain) {
        auto details = get_swapchain_support_details(self->physical_device, self->surface);
        self->swapchain.min_image_count = details.capabilities.minImageCount;

        VkSurfaceFormatKHR selected_format = select_surface_format(details.formats);
        VkPresentModeKHR selected_present_mode = select_presentation_mode(details.present_modes);
        VkExtent2D selected_extent = select_swapchain_extent(details.capabilities, self->window);
        uint32_t selected_image_count = select_image_count(details.capabilities);

        VkSwapchainCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        create_info.surface = self->surface;
        create_info.minImageCount = selected_image_count;
        create_info.imageFormat = selected_format.format;
        create_info.imageColorSpace = selected_format.colorSpace;
        create_info.imageExtent = selected_extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
        create_info.preTransform = details.capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = selected_present_mode;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = old_swapchain;

        VkResult result = VK_SUCCESS;
        if((result = vkCreateSwapchainKHR(self->device, &create_info, nullptr, &self->swapchain.swapchain)) != VK_SUCCESS){
            throw std::runtime_error(std::format("Failed to create the swapchain! result = {}", static_cast<uint32_t>(result)));
        }

        uint32_t image_count = 0;
        vkGetSwapchainImagesKHR(self->device, self->swapchain.swapchain, &image_count, nullptr);
        self->swapchain.images.resize(image_count);
        vkGetSwapchainImagesKHR(self->device, self->swapchain.swapchain, &image_count, self->swapchain.images.data());

        self->swapchain.format = selected_format.format;
        self->swapchain.extent = selected_extent;

        create_image_views(self);
        create_depth_resources(self, selected_extent);
    }

    void VulkanRenderer::init_framebuffers() {
        auto details = get_swapchain_support_details(self->physical_device, self->surface);
        VkExtent2D selected_extent = select_swapchain_extent(details.capabilities, self->window);

        self->swapchain.framebuffers.resize(self->swapchain.images.size());

        for(uint32_t i = 0; i < self->swapchain.framebuffers.size(); i++){
            VkImageView attachments[] = {
                    self->swapchain.image_views[i],
                    self->swapchain.depth_resources.image_views[i],
            };
            uint32_t attachment_count = sizeof(attachments)/sizeof(VkImageView);

            VkFramebufferCreateInfo create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            create_info.attachmentCount = attachment_count;
            create_info.pAttachments = attachments;
            create_info.renderPass = self->default_render_pass;
            create_info.width = selected_extent.width;
            create_info.height = selected_extent.height;
            create_info.layers = 1;

            VkResult result = VK_SUCCESS;
            if((result =
                vkCreateFramebuffer(self->device, &create_info, nullptr, &self->swapchain.framebuffers[i])) != VK_SUCCESS)
            {
                throw std::runtime_error(
                        std::format("Failed to create swapchain framebuffer {}! result = {}", i, static_cast<uint32_t>(result)));
            }
        }
    }

    void VulkanRenderer::init_sync_objects() {
        self->image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        self->render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        self->in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for(uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i ++) {
            VkResult result = VK_SUCCESS;
            if ((result = vkCreateSemaphore(self->device, &semaphore_info, nullptr,
                                            &self->image_available_semaphores[i])) != VK_SUCCESS) {
                throw std::runtime_error(
                        std::format("Failed to create sync object, image available semaphore! result = {}",
                                    static_cast<uint32_t>(result)));
            }
            if ((result = vkCreateSemaphore(self->device, &semaphore_info, nullptr,
                                            &self->render_finished_semaphores[i])) != VK_SUCCESS) {
                throw std::runtime_error(
                        std::format("Failed to create sync object, render finished semaphore! result = {}",
                                    static_cast<uint32_t>(result)));
            }
            if ((result = vkCreateFence(self->device, &fence_info, nullptr, &self->in_flight_fences[i])) != VK_SUCCESS) {
                throw std::runtime_error(
                        std::format("Failed to create sync object, in flight fence! result = {}",
                                    static_cast<uint32_t>(result)));
            }
        }
    }

    bool VulkanRenderer::acquire_next_swapchain_image() {
        vkWaitForFences(self->device, 1, &self->in_flight_fences[self->current_frame], VK_TRUE, UINT64_MAX);

        VkResult result = vkAcquireNextImageKHR(self->device, self->swapchain.swapchain, UINT64_MAX,
                              self->image_available_semaphores[self->current_frame], VK_NULL_HANDLE, &self->current_image);

        if(result == VK_ERROR_OUT_OF_DATE_KHR){
            recreate_swapchain();
            return false;
        } else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
            throw std::runtime_error("Failed to acquire the next swapchain image!");
        }

        vkResetFences(self->device, 1, &self->in_flight_fences[self->current_frame]);

        return true;
    }

    void VulkanRenderer::begin_default_render_pass(VkCommandBuffer cmd, float r, float g, float b, float a) {
       auto extent = self->swapchain.extent;

        VkRenderPassBeginInfo begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        begin_info.renderPass = self->default_render_pass;
        begin_info.framebuffer = self->swapchain.framebuffers[current_image()];
        begin_info.renderArea = {{0, 0}, extent};

        VkClearValue clear_values[2] = {};
        clear_values[0].color = {
            r, g, b, a
        };
        clear_values[1].depthStencil = {1.0f, 0};

        begin_info.clearValueCount = 2;
        begin_info.pClearValues = clear_values;

        vkCmdBeginRenderPass(cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{{0, 0}, extent};
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    void VulkanRenderer::present() {
        auto wait = current_render_finished_semaphore();
        auto current = current_image();

        VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &wait;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &self->swapchain.swapchain;
        present_info.pImageIndices = &current;

        VkResult result = vkQueuePresentKHR(get_queue(Queue::GRAPHICS), &present_info);
        if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR){
            recreate_swapchain();
        } else if(result != VK_SUCCESS){
            throw std::runtime_error("Failed to present a swapchain image!");
        }

        self->current_frame = (self->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    ImGuiIO& VulkanRenderer::init_imgui() {
        init_descriptor_pool();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void) io;

        ImGui_ImplGlfw_InitForVulkan(self->window, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = self->instance;
        init_info.PhysicalDevice = self->physical_device;
        init_info.Device = self->device;
        init_info.QueueFamily = self->queue_family_info.index;
        init_info.Queue = get_queue(Queue::GRAPHICS);
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = self->descriptor_pool;
        init_info.Subpass = 0;
        init_info.MinImageCount = self->swapchain.min_image_count;
        init_info.ImageCount = self->swapchain.images.size();
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        ImGui_ImplVulkan_Init(&init_info, self->default_render_pass);

        self->cleanup_imgui = true;
        return io;
    }

    void VulkanRenderer::init_descriptor_pool() {
        VkDescriptorPoolCreateInfo create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        create_info.maxSets = 1000;
        create_info.poolSizeCount = 0;
        create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        VkResult result = VK_SUCCESS;
        if((result
            = vkCreateDescriptorPool(self->device, &create_info, nullptr, &self->descriptor_pool)) != VK_SUCCESS){
            throw std::runtime_error(
                    std::format("Failed to create the descriptor pool! result = {}",
                                static_cast<uint32_t>(result)));
        }
    }

    void VulkanRenderer::recreate_swapchain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(self->window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(self->window, &width, &height);
            glfwWaitEvents();
        }

        device_wait_idle();

        Swapchain old_swapchain = self->swapchain;
        init_swapchain(old_swapchain.swapchain);
        init_framebuffers();
        old_swapchain.destroy(self->device, self->allocator);
    }
} // g_app