//
// Created by jandr on 15/11/2023.
//

#pragma once

#include "renderer.hpp"
#include "buffer.hpp"
#include "pipeline.hpp"
#include "descriptor.hpp"
#include "image.hpp"

namespace g_app {
    struct SubmitSyncObjects {
        std::vector<VkSemaphore> wait = {};
        std::vector<VkPipelineStageFlags> wait_stages = {};
        std::vector<VkSemaphore> signal = {};
        VkFence fence = VK_NULL_HANDLE;
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

            VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &self->cmdbuf;
            submit_info.waitSemaphoreCount = sync.wait.size();
            submit_info.pWaitSemaphores = sync.wait.data();
            submit_info.pWaitDstStageMask = sync.wait_stages.data();
            submit_info.signalSemaphoreCount = sync.signal.size();
            submit_info.pSignalSemaphores = sync.signal.data();

            vkQueueSubmit(self->renderer.get_queue(queue), 1, &submit_info, sync.fence);
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

        CommandBuffer& transition_image_layout(const Image& image, VkImageLayout old_layout, VkImageLayout new_layout,
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
        }

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

        CommandBuffer& end_render_pass(){
            assert(self->in_render_pass && "Can't end a render pass when one hasn't begun!");

            vkCmdEndRenderPass(self->cmdbuf);
            self->in_render_pass = false;
            return *this;
        }

        CommandBuffer& bind_graphics_pipeline(const Pipeline& pipeline){
            assert(self->in_render_pass && "Can't execute render pass dependant commands when no render pass has begun!");
            vkCmdBindPipeline(self->cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.vk_pipeline());
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

        CommandBuffer& cmd(const std::function<void(VkCommandBuffer)>& f){
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
                auto inner = renderer.inner();
                vkFreeCommandBuffers(inner->device, inner->command_pool, 1, &cmdbuf);
            }
        };

        std::shared_ptr<Inner> self;
    };
}
