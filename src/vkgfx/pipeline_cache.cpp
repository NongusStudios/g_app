//
// Created by jeol on 21/02/24.
//
#include <vkgfx/pipeline_cache.hpp>

#include <fstream>
#include <iostream>

namespace g_app {

    bool PipelineCache::serialize(const std::string &path) {
        size_t cache_size = 0;
        vkGetPipelineCacheData(self->renderer.inner()->device, self->cache, &cache_size, nullptr);

        auto* cache_data = new char[cache_size];
        vkGetPipelineCacheData(self->renderer.inner()->device, self->cache, &cache_size, (void*)cache_data);

        std::ofstream fp(path, std::ios::binary);
        if(!fp.is_open()) return false;

        fp.write(cache_data, static_cast<std::streamsize>(cache_size));
        fp.close();

        delete[] cache_data;
        return true;
    }

    PipelineCache PipelineCache::load(VulkanRenderer renderer, const std::string &path) {
        std::ifstream fp(path, std::ios::ate | std::ios::binary);
        if(!fp.is_open()){
            return PipelineCache(renderer, path); // No file - load empty cache
        }

        std::streamsize len = fp.tellg();
        fp.seekg(0);

        char* cache_data = new char[len];
        fp.read(cache_data, len);

        auto cache = PipelineCache(renderer, (void*)cache_data, len, path);
        delete[] cache_data;
        return cache;
    }

    PipelineCache::PipelineCache(VulkanRenderer renderer, void *prev_data, size_t size, const std::string& label): self{std::make_shared<Inner>(renderer)} {
        self->label = label;

        VkPipelineCacheCreateInfo create_info = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
        create_info.initialDataSize = size;
        create_info.pInitialData = prev_data;

        VkResult result;
        if((result = vkCreatePipelineCache(renderer.inner()->device, &create_info, nullptr, &self->cache))
            != VK_SUCCESS)
        {
            throw std::runtime_error(
                    std::format("Failed to create a pipeline cache! label = {}, result = {}", self->label, static_cast<uint32_t>(result) )
            );
        }
    }

    PipelineCache::PipelineCache(VulkanRenderer renderer, const std::string& label): PipelineCache(renderer, nullptr, 0, label) {}
} // g_app