//
// Created by jandr on 14/11/2023.
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

#pragma once

#include "renderer.hpp"

namespace g_app {
    template<typename T>
    class BufferInit;

    template<typename T>
    class Buffer {
    public:
        Buffer() = default;
        Buffer(const Buffer&) = default;
        Buffer& operator = (const Buffer&) = default;

        T* map(){
            void* data = nullptr;
            vmaMapMemory(self->renderer.inner()->allocator, self->allocation, &data);
            return reinterpret_cast<T*>(data);
        }
        void unmap(){
            vmaUnmapMemory(self->renderer.inner()->allocator, self->allocation);
        }

        VkBuffer vk_buffer() const { return self->buffer; }
        VmaAllocation vma_allocation() const { return self->allocation; }
        size_t size() const { return self->size; }
        size_t sizeb() const { return self->size * sizeof(T); }
    private:
        struct Config {
            VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            VmaMemoryUsage     memory_usage = VMA_MEMORY_USAGE_AUTO;
            size_t             size = 0;
            const T*           data = nullptr;
            std::string        label = "unnamed buffer";
        };

        struct Inner {
            VulkanRenderer renderer = {};
            VkBuffer buffer = VK_NULL_HANDLE;
            VmaAllocation allocation = VK_NULL_HANDLE;
            size_t   size = 0;
            std::string label = "unnamed buffer";

            ~Inner(){
                if(!renderer.is_valid()) return;

                vmaDestroyBuffer(renderer.inner()->allocator, buffer, allocation);
            }
        };

        std::shared_ptr<Inner> self;

        Buffer(VulkanRenderer renderer, const Config& config): self{std::make_shared<Inner>(renderer)}{
            self->size = config.size;
            self->label = config.label;

            VmaAllocationCreateInfo alloc_info = {};
            alloc_info.usage = config.memory_usage;

            VkBufferCreateInfo create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            create_info.size = config.size * sizeof(T);
            create_info.usage = config.usage;
            create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            auto inner = renderer.inner();
            
            VkResult result = VK_SUCCESS;
            if(
                    (result = vmaCreateBuffer(inner->allocator, &create_info, &alloc_info,
                                              &self->buffer, &self->allocation, nullptr)) != VK_SUCCESS
            ){
                throw std::runtime_error(std::format("Failed to create a Buffer! buffer = {}, result = {}", self->label, static_cast<uint32_t>(result)));
            }

            if(config.data){
                auto data = this->map();
                memcpy(data, config.data, config.size*sizeof(T));
                this->unmap();
            }
        }

        friend class BufferInit<T>;
    };

    template<typename T>
    class BufferInit {
    public:
        BufferInit() = default;

        /* Used for debugging purposes. */
        BufferInit& set_label(const std::string& label){
            m_config.label = label;
            return *this;
        }
        /* Specifies the usage of the buffer. e.g. VK_BUFFER_USAGE_VERTEX_BUFFER_BIT */
        BufferInit& set_usage(VkBufferUsageFlags usage){
            m_config.usage = usage;
            return *this;
        }

        /* Specifies how the buffers memory will be used. Set to VMA_MEMORY_USAGE_AUTO by default. */
        BufferInit& set_memory_usage(VmaMemoryUsage usage){
            m_config.memory_usage = usage;
            return *this;
        }

        /* Specifies the size of the buffer in indices not bytes. */
        BufferInit& set_size(size_t size){
            m_config.size = size;
            return *this;
        }

        /*
         * Populates the buffer with data by mapping its memory and using memcpy().
         * Will not work if the buffer is not host visible.
         * Use a staging buffer if your buffer memory is GPU only.
         */
        BufferInit& set_data(const T* data){
            m_config.data = data;
            return *this;
        }

        Buffer<T> init(VulkanRenderer renderer){
            try {
                return {renderer, m_config};
            } catch(const std::runtime_error& e) {
                spdlog::error(e.what());
                std::exit(EXIT_FAILURE);
            }
        }
    private:
        Buffer<T>::Config m_config = {};
    };

    class VertexBufferBindings {
    public:
        VertexBufferBindings() = default;

        template<typename T>
        VertexBufferBindings& add_buffer(const Buffer<T>& buffer, VkDeviceSize offset=0){
            m_buffers.push_back(buffer.vk_buffer());
            m_offsets.push_back(offset * sizeof(T));
            return *this;
        }

        std::vector<VkBuffer>& buffers() { return m_buffers; }
        std::vector<VkDeviceSize>& offsets() { return m_offsets; }
    private:
        std::vector<VkBuffer> m_buffers = {};
        std::vector<VkDeviceSize> m_offsets ={};
    };
} // g_app
