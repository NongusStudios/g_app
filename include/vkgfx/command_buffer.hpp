//
// Created by jandr on 15/11/2023.
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
#include "buffer.hpp"
#include "pipeline.hpp"
#include "descriptor.hpp"
#include "image.hpp"
#include "framebuffer.hpp"
#include "sync.hpp"

#include <iostream>

namespace g_app {
    struct SubmitSyncObjects {
        std::vector<Semaphore> wait = {};
        std::vector<VkPipelineStageFlags> wait_stages = {};
        std::vector<Semaphore> signal = {};
        Fence fence;
    };

    struct PipelineBarrierInfo {
        VkPipelineStageFlags src_stage, dst_stage;
        VkDependencyFlags flags = 0;
        std::vector<VkMemoryBarrier> memory_barriers;
        std::vector<VkBufferMemoryBarrier> buffer_barriers;
        std::vector<VkImageMemoryBarrier> image_barriers;
    };

    class PipelineBarrierInfoBuilder {
    public:
        PipelineBarrierInfoBuilder() = default;

        PipelineBarrierInfoBuilder& set_stage_flags(VkPipelineStageFlags src, VkPipelineStageFlags dst){
            m_info.src_stage = src;
            m_info.dst_stage = dst;
            return *this;
        }

        PipelineBarrierInfoBuilder& set_dependency_flags(VkDependencyFlags flags){
            m_info.flags = flags;
            return *this;
        }

        PipelineBarrierInfoBuilder& add_memory_barrier(VkAccessFlags src_access, VkAccessFlags dst_access){
            VkMemoryBarrier b = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
            b.srcAccessMask = src_access;
            b.dstAccessMask = dst_access;
            m_info.memory_barriers.push_back(b);
            return *this;
        }

        template<typename T>
        PipelineBarrierInfoBuilder& add_buffer_memory_barrier(const Buffer<T>& buffer,
                                                              VkAccessFlags src_access, VkAccessFlags dst_access, VkDeviceSize offset=0){
            VkBufferMemoryBarrier b = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
            b.size = buffer.sizeb();
            b.offset = offset;
            b.srcAccessMask = src_access;
            b.dstAccessMask = dst_access;
            b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.buffer = buffer.vk_buffer();
            m_info.buffer_barriers.push_back(b);
            return *this;
        }

        PipelineBarrierInfoBuilder& add_image_memory_barrier(const Image& image,
                                                             VkAccessFlags src_access, VkAccessFlags dst_access,
                                                             VkImageLayout old_layout, VkImageLayout new_layout,
                                                             VkImageSubresourceRange subresource_range){
            VkImageMemoryBarrier b = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            b.srcAccessMask = src_access;
            b.dstAccessMask = dst_access;
            b.oldLayout = old_layout;
            b.newLayout = new_layout;
            b.subresourceRange = subresource_range;
            b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.image =  image.vk_image();
            m_info.image_barriers.push_back(b);
            return *this;
        }

        PipelineBarrierInfo build() { return m_info; }

    private:
        PipelineBarrierInfo m_info = {};
    };

    class CommandBuffer {
    public:
        CommandBuffer() = default;

        explicit CommandBuffer(const VulkanRenderer& renderer, VkCommandBufferLevel level=VK_COMMAND_BUFFER_LEVEL_PRIMARY):
            self{std::make_shared<Inner>(renderer)}
        {
            auto inner = self->renderer.inner();
            VkCommandBufferAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            alloc_info.commandPool = inner->command_pool;
            alloc_info.commandBufferCount = 1;
            alloc_info.level = level;

            VkResult result = VK_SUCCESS;
            if((result = vkAllocateCommandBuffers(inner->device, &alloc_info, &self->cmdbuf)) != VK_SUCCESS){
                spdlog::error("Failed to allocate single use command buffers! result = {}", static_cast<uint32_t>(result));
                std::exit(EXIT_FAILURE);
            }
        }

        CommandBuffer& begin(VkCommandBufferUsageFlags usage=0){
            assert(!self->recording && "Can't begin recording when the command buffer is already recording!");

            VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            begin_info.flags = usage;

            VkResult result = VK_SUCCESS;
            if((result = vkBeginCommandBuffer(self->cmdbuf, &begin_info)) != VK_SUCCESS){
                spdlog::error("Failed to begin recording single use commands! result = {}", static_cast<uint32_t>(result));
                std::exit(EXIT_FAILURE);
            }

            self->recording = true;
            return *this;
        }
        CommandBuffer& end(){
            assert(self->recording && "Can't end a command buffer if its not recording!");

            VkResult result = VK_SUCCESS;
            if((result = vkEndCommandBuffer(self->cmdbuf)) != VK_SUCCESS){
                spdlog::error("Failed to end recording single use commands! result = {}", static_cast<uint32_t>(result));
                std::exit(EXIT_FAILURE);
            }

            self->recording = false;

            return *this;
        }

        void submit(Queue queue, const SubmitSyncObjects& sync = {}){
            if(self->in_render_pass) end_render_pass();
            if(self->recording) end();
            
            std::vector<VkSemaphore> wait = {};
            std::vector<VkSemaphore> signal = {};
            wait.reserve(sync.wait.size());
            signal.reserve(sync.signal.size());
            

            for(const auto& sem : sync.wait) { wait.push_back(sem.vk_semaphore()); }
            for(const auto& sem : sync.signal) { signal.push_back(sem.vk_semaphore()); }

            VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &self->cmdbuf;
            submit_info.waitSemaphoreCount = wait.size();
            submit_info.pWaitSemaphores = wait.data();
            submit_info.pWaitDstStageMask = sync.wait_stages.data();
            submit_info.signalSemaphoreCount = signal.size();
            submit_info.pSignalSemaphores = signal.data();

            vkQueueSubmit(self->renderer.get_queue(queue), 1, &submit_info, sync.fence.vk_fence());
            vkQueueWaitIdle(self->renderer.get_queue(queue));

            vkResetCommandBuffer(self->cmdbuf, 0);

            self->recording = false;
        }

        template<typename T>
        CommandBuffer& copy_buffer(const Buffer<T>& src, const Buffer<T>& dst,
                                   VkDeviceSize size = 0, VkDeviceSize src_offset = 0, VkDeviceSize dst_offset = 0){
            assert(self->recording && "Commands can't be called without first calling begin()!");
            if(size == 0) {
                assert(src.size() == dst.size() && "Buffers must be the same size when performing a full copy!");
            }

            VkBufferCopy copy = {};
            copy.srcOffset = src_offset * sizeof(T);
            copy.dstOffset = dst_offset * sizeof(T);
            copy.size = (size > 0) ? size * sizeof(T) : src.size() * sizeof(T);
            vkCmdCopyBuffer(self->cmdbuf, src.vk_buffer(), dst.vk_buffer(), 1, &copy);

            return *this;
        }

        template<typename T>
        CommandBuffer& copy_buffer_to_image(const Buffer<T>& src, const Image& dst,
                                            VkImageAspectFlags aspect_mask, VkImageLayout dst_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                            uint32_t mip_level = 0, uint32_t base_layer = 0, uint32_t layer_count = 1){
            assert(self->recording && "Commands can't be called without first calling begin()!");

            VkBufferImageCopy region = {};
            region.imageSubresource.aspectMask = aspect_mask;
            region.imageSubresource.mipLevel = mip_level;
            region.imageSubresource.baseArrayLayer = base_layer;
            region.imageSubresource.layerCount = layer_count;
            region.imageOffset = {0, 0,0};
            region.imageExtent = dst.extent();

            vkCmdCopyBufferToImage(
                self->cmdbuf,
                src.vk_buffer(),
                dst.vk_image(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );

            return *this;
        }

        /*CommandBuffer& transition_image_layout(const Image& image, VkImageLayout old_layout, VkImageLayout new_layout,
                                               VkImageSubresourceRange subresource_range,
                                               VkAccessFlags src_access, VkAccessFlags dst_access,
                                               VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage){
            assert(self->recording && "Commands can't be called without first calling begin()!");

            VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            barrier.oldLayout = old_layout;
            barrier.newLayout = new_layout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image.vk_image();
            barrier.subresourceRange = subresource_range;
            barrier.srcAccessMask = src_access;
            barrier.dstAccessMask = dst_access;

            vkCmdPipelineBarrier(self->cmdbuf,
                                 src_stage, dst_stage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            return *this;
        }*/

        CommandBuffer& draw_imgui(){
            assert(self->in_render_pass && "Can't execute render pass dependant commands when no render pass has begun!");
            self->renderer.render_imgui(self->cmdbuf);
            return *this;
        }

        CommandBuffer& begin_default_render_pass(float r, float g, float b, float a){
            assert(self->recording && "Commands can't be called without first calling begin()!");
            assert(!self->in_render_pass && "Can't begin a render pass when another has already begun!");

            self->renderer.begin_default_render_pass(self->cmdbuf, r, g, b, a);
            self->in_render_pass = true;
            return *this;
        }

        CommandBuffer& begin_render_pass(const RenderPass& render_pass, const Framebuffer& framebuffer,
                                         const std::vector<VkClearValue>& clear_values,
                                         const Extent2D<uint32_t>& viewport_extent){

            VkExtent2D extent = {viewport_extent.width, viewport_extent.height};

            VkRenderPassBeginInfo begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
            begin_info.renderPass = render_pass.vk_render_pass();
            begin_info.framebuffer = framebuffer.vk_framebuffer();
            begin_info.renderArea = {{0, 0}, extent};
            begin_info.clearValueCount = clear_values.size();
            begin_info.pClearValues = clear_values.data();

            vkCmdBeginRenderPass(self->cmdbuf, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(extent.width);
            viewport.height = static_cast<float>(extent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            VkRect2D scissor{{0, 0}, extent};
            vkCmdSetViewport(self->cmdbuf, 0, 1, &viewport);
            vkCmdSetScissor(self->cmdbuf, 0, 1, &scissor);

            self->in_render_pass = true;

            return *this;
        }

        CommandBuffer& end_render_pass(){
            assert(self->in_render_pass && "Can't end a render pass when one hasn't begun!");

            vkCmdEndRenderPass(self->cmdbuf);
            self->in_render_pass = false;
            return *this;
        }

        CommandBuffer &bind_pipeline(const Pipeline &pipeline, VkPipelineBindPoint bind_point) {
            assert(self->in_render_pass && "Can't execute render pass dependant commands when no render pass has begun!");
            vkCmdBindPipeline(self->cmdbuf, bind_point, pipeline.vk_pipeline());
            return *this;
        }

        template<typename T>
        CommandBuffer& bind_vertex_buffer(const Buffer<T>& buffer, VkDeviceSize offset = 0){
            assert(self->in_render_pass && "Can't execute render pass dependant commands when no render pass has begun!");
            VkDeviceSize offset_bytes = offset * sizeof(T);
            VkBuffer vk_buffer[] = {buffer.vk_buffer()};
            vkCmdBindVertexBuffers(self->cmdbuf, 0, 1, vk_buffer, &offset_bytes);
            return *this;
        }

        template<typename T>
        CommandBuffer bind_index_buffer(const Buffer<T>& buffer, VkIndexType type, VkDeviceSize offset = 0){
            assert(self->in_render_pass && "Can't execute render pass dependant commands when no render pass has begun!");
            VkDeviceSize offset_bytes = offset * sizeof(T);
            VkBuffer vk_buffer = buffer.vk_buffer();
            vkCmdBindIndexBuffer(self->cmdbuf, vk_buffer, offset_bytes, type);
            return *this;
        }

        CommandBuffer& bind_vertex_buffers(VertexBufferBindings bindings){
            assert(self->in_render_pass && "Can't execute render pass dependant commands when no render pass has begun!");
            vkCmdBindVertexBuffers(self->cmdbuf,
                                   0, bindings.buffers().size(),
                                   bindings.buffers().data(), bindings.offsets().data());
            return *this;
        }

        template<typename T>
        CommandBuffer& push_constants(const Pipeline& pipeline, VkShaderStageFlags stage, const T& constants){
            assert(self->recording && "Commands can't be called without first calling begin()!");
            vkCmdPushConstants(self->cmdbuf, pipeline.vk_pipeline_layout(), stage, 0, sizeof(T), &constants);
            return *this;
        }

        CommandBuffer& bind_descriptor_sets(
                const Pipeline& pipeline, VkPipelineBindPoint bind_point,
                const std::vector<DescriptorSet>& sets){
            assert(self->recording && "Commands can't be called without first calling begin()!");

            std::vector<VkDescriptorSet> vk_sets = {};
            vk_sets.reserve(sets.size());

            for(const auto& set : sets){
                vk_sets.push_back(set.vk_descriptor_set());
            }

            vkCmdBindDescriptorSets(self->cmdbuf, bind_point, pipeline.vk_pipeline_layout(), 0,
                                    vk_sets.size(), vk_sets.data(),
                                    0, nullptr);
            return *this;
        }

        // If VK_KHR_push_descriptor is enabled
        CommandBuffer& ext_push_descriptor_set(const Pipeline& pipeline, VkPipelineBindPoint bind_point, uint32_t set,
                                               const std::vector<VkWriteDescriptorSet>& writes){
            auto push_descriptor_set = self->renderer.get_extpfn<PFN_vkCmdPushDescriptorSetKHR>("vkCmdPushDescriptorSetKHR");
            push_descriptor_set(self->cmdbuf, bind_point, pipeline.vk_pipeline_layout(), set, static_cast<uint32_t>(writes.size()),
                                writes.data());
            return *this;
        }

        CommandBuffer& next_subpass(VkSubpassContents contents=VK_SUBPASS_CONTENTS_INLINE){
            assert(self->in_render_pass && "Can't execute render pass dependant commands when no render pass has begun!");
            vkCmdNextSubpass(self->cmdbuf, contents);
            return *this;
        }

        CommandBuffer& draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex=0, uint32_t first_instance=0){
            assert(self->in_render_pass && "Can't execute render pass dependant commands when no render pass has begun!");
            vkCmdDraw(self->cmdbuf, vertex_count, instance_count, first_vertex, first_instance);
            return *this;
        }

        CommandBuffer& draw_indexed(
                uint32_t index_count, uint32_t instance_count, uint32_t first_index=0, int32_t vertex_offset=0, uint32_t first_instance=0
        ){
            assert(self->in_render_pass && "Can't execute render pass dependant commands when no render pass has begun!");
            vkCmdDrawIndexed(self->cmdbuf, index_count, instance_count,
                             first_index, vertex_offset, first_instance);
            return *this;
        }

        CommandBuffer& dispatch(uint32_t x, uint32_t y, uint32_t z){
            assert(self->recording && "Commands can't be  called without first calling begin()!");
            vkCmdDispatch(self->cmdbuf, x, y, z);

            return *this;
        }

        CommandBuffer& pipeline_barrier(const PipelineBarrierInfo& info){
            assert(self->recording && "Commands can't be  called without first calling begin()!");
            vkCmdPipelineBarrier(self->cmdbuf, info.src_stage, info.dst_stage, info.flags,
                                 info.memory_barriers.size(), info.memory_barriers.data(),
                                 info.buffer_barriers.size(), info.buffer_barriers.data(),
                                 info.image_barriers.size(), info.image_barriers.data());
            return *this;
        }

        CommandBuffer& cmd(const std::function<void(CommandBuffer&)>& f){
            f(*this);
            return *this;
        }

        CommandBuffer& vk_cmd(const std::function<void(VkCommandBuffer)>& f){
            f(self->cmdbuf);
            return *this;
        }
    private:
        struct Inner {
            VulkanRenderer renderer;
            VkCommandBuffer cmdbuf = VK_NULL_HANDLE;
            bool recording = false;
            bool in_render_pass = false;

            ~Inner(){
                if(!renderer.is_valid()) return;

                auto inner = renderer.inner();
                vkFreeCommandBuffers(inner->device, inner->command_pool, 1, &cmdbuf);
            }
        };

        std::shared_ptr<Inner> self;
    };
}
