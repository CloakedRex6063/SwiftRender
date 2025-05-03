#pragma once
#include "algorithm"
#include "ranges"
#include "swift_structs.hpp"
#include "vulkan_helpers.hpp"
#include "vulkan_structs.hpp"

namespace Swift::Vulkan
{
    inline std::expected<void, Error> WaitIdle(const Context& context)
    {
        const auto result = vkDeviceWaitIdle(context.Device);
        return HandleResult(result, Error::eDeviceWaitFailed);
    }

    inline std::expected<void, Error>
    WaitFence(const Context& context, VkFence fence, const uint64_t timeout = 1'000'000'000)
    {
        const auto result = vkWaitForFences(context.Device, 1, &fence, true, timeout);
        if (result != VK_SUCCESS)
        {
            return std::unexpected(Error::eFenceCreateFailed);
        }
        const auto reset_result = vkResetFences(context.Device, 1, &fence);
        return HandleResult(reset_result, Error::eFenceWaitFailed);
    };

    inline std::expected<uint32_t, Error> AcquireNextImage(const Context& context, VkSemaphore semaphore)
    {
        uint32_t image_index = 0;
        const auto result =
            vkAcquireNextImageKHR(context.Device, context.Swapchain, 1'000'000'000, semaphore, nullptr, &image_index);
        return HandleResult(result, Error::eAcquireNextImageFailed, image_index);
    }

    struct SubmitInfo
    {
        std::span<const Command> Commands;
        std::span<const VkSemaphore> WaitSemaphores;
        VkPipelineStageFlags WaitStage;
        std::span<const VkSemaphore> SignalSemaphores;
    };

    inline std::expected<void, Error> Submit(const Queue& queue, const SubmitInfo& info, VkFence fence)
    {
        std::vector<VkCommandBuffer> command_buffers(info.Commands.size());
        std::ranges::transform(
            info.Commands,
            command_buffers.begin(),
            [&](auto& cmd)
            {
                return cmd.CommandBuffer;
            });

        const VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = static_cast<uint32_t>(info.WaitSemaphores.size()),
            .pWaitSemaphores = info.WaitSemaphores.data(),
            .pWaitDstStageMask = &info.WaitStage,
            .commandBufferCount = static_cast<uint32_t>(command_buffers.size()),
            .pCommandBuffers = command_buffers.data(),
            .signalSemaphoreCount = static_cast<uint32_t>(info.SignalSemaphores.size()),
            .pSignalSemaphores = info.SignalSemaphores.data(),
        };
        const auto result = vkQueueSubmit(queue.BaseQueue, 1, &submit_info, fence);
        return HandleResult(result, Error::eSubmitFailed);
    }

    inline std::expected<void, Error>
    Present(const Context& context, const Queue queue, VkSemaphore render_finished_semaphore, uint32_t image_index)
    {
        const VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_finished_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &context.Swapchain,
            .pImageIndices = &image_index,
        };
        const auto result = vkQueuePresentKHR(queue.BaseQueue, &present_info);
        return HandleResult(result, Error::ePresentFailed);
    }

    inline std::expected<void, Error> BeginCommand(const Context& context, const Command& command)
    {
        constexpr VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkResetCommandPool(context.Device, command.CommandPool, 0);
        const auto result = vkBeginCommandBuffer(command.CommandBuffer, &begin_info);
        return HandleResult(result, Error::eCommandBeginFailed);
    }

    inline std::expected<void, Error> EndCommand(const Command& command)
    {
        const auto result = vkEndCommandBuffer(command.CommandBuffer);
        return HandleResult(result, Error::eCommandEndFailed);
    }

    struct TransitionInfo
    {
        VkPipelineStageFlagBits DstStageMask;
        VkAccessFlags DstAccessMask;
        VkImageLayout NewLayout;
    };

    inline void TransitionImage(const Command& command, Texture& texture, const TransitionInfo& info)
    {
        const VkImageMemoryBarrier image_barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = texture.PreviousAccess,
            .dstAccessMask = info.DstAccessMask,
            .oldLayout = texture.PreviousLayout,
            .newLayout = info.NewLayout,
            .image = texture.Image,
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = texture.MipLevels,
                    .baseArrayLayer = 0,
                    .layerCount = texture.Depth,
                },
        };

        vkCmdPipelineBarrier(
            command.CommandBuffer, texture.PreviousStage, info.DstStageMask, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);

        texture.PreviousLayout = image_barrier.newLayout;
        texture.PreviousStage = info.DstStageMask;
        texture.PreviousAccess = info.DstAccessMask;
    }

    inline void ClearImage(const Command& command, Texture& texture, const Vec4 color)
    {
        constexpr TransitionInfo info = {
            .DstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
            .DstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .NewLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        TransitionImage(command, texture, info);

        texture.PreviousStage = info.DstStageMask;
        texture.PreviousAccess = info.DstAccessMask;

        const VkImageSubresourceRange subresource_range = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = texture.MipLevels,
            .baseArrayLayer = 0,
            .layerCount = texture.Depth,
        };
        const VkClearColorValue clear_color = { color.X, color.Y, color.Z, color.W };
        vkCmdClearColorImage(
            command.CommandBuffer, texture.Image, texture.PreviousLayout, &clear_color, 1, &subresource_range);
    }

    inline void SetScissor(const Command& command, const Rect& rect)
    {
        const VkRect2D scissor = { .offset = { static_cast<int>(rect.Offset.X), static_cast<int>(rect.Offset.Y) },
                                   .extent = { static_cast<uint32_t>(rect.Extent.X),
                                               static_cast<uint32_t>(rect.Extent.Y) } };
        vkCmdSetScissor(command.CommandBuffer, 0, 1, &scissor);
    }

    inline void SetViewport(const Command& command, const Viewport& viewport)
    {
        const VkViewport vk_viewport = {
            .x = viewport.Offset.X,
            .y = viewport.Offset.Y,
            .width = viewport.Extent.X,
            .height = viewport.Extent.Y,
            .minDepth = viewport.DepthRange.X,
            .maxDepth = viewport.DepthRange.Y,
        };
        vkCmdSetViewport(command.CommandBuffer, 0, 1, &vk_viewport);
    }

    inline void EnableDepthWrite(const Command& command, bool enable)
    {
        vkCmdSetDepthWriteEnable(command.CommandBuffer, enable);
    }

    inline void EnableDepthTest(const Command& command, bool enable)
    {
        vkCmdSetDepthTestEnable(command.CommandBuffer, enable);
    }

    inline void EnableDepthBoundsTest(const Command& command, bool enable)
    {
        vkCmdSetDepthBoundsTestEnable(command.CommandBuffer, enable);
    }

    inline void SetDepthBounds(const Command& command, float min_depth_bounds, float max_depth_bounds)
    {
        vkCmdSetDepthBounds(command.CommandBuffer, min_depth_bounds, max_depth_bounds);
    }

    inline void EnableDepthBias(const Command& command, bool enable)
    {
        vkCmdSetDepthBiasEnable(command.CommandBuffer, enable);
    }

    inline void SetDepthBias(const Command& command, float constant_factor, float clamp, float slope_factor)
    {
        vkCmdSetDepthBias(command.CommandBuffer, constant_factor, clamp, slope_factor);
    }

    inline void EnableStencilTest(const Command& command, bool enable)
    {
        vkCmdSetStencilTestEnable(command.CommandBuffer, enable);
    }

    inline void EnableRasterizerDiscard(const Command& command, bool enable)
    {
        vkCmdSetRasterizerDiscardEnable(command.CommandBuffer, enable);
    }

    inline void SetCullMode(const Command& command, CullMode mode)
    {
        vkCmdSetCullMode(command.CommandBuffer, static_cast<VkCullModeFlags>(mode));
    }

    inline void SetFrontFace(const Command& command, FrontFace front_face)
    {
        vkCmdSetFrontFace(command.CommandBuffer, static_cast<VkFrontFace>(front_face));
    }

    inline void BindShader(const Command& command, const Shader& shader)
    {
        vkCmdBindPipeline(command.CommandBuffer, shader.BindPoint, shader.Pipeline);
    }

    inline void BeginRender(const Command& command, const RenderInfo& render_info, std::span<Texture> textures)
    {
        std::vector<VkRenderingAttachmentInfo> color_attachment_infos(render_info.RenderTargets.size());
        std::ranges::transform(
            render_info.RenderTargets,
            color_attachment_infos.begin(),
            [&](const TextureHandle& render_target)
            {
                auto& texture = textures[static_cast<uint32_t>(render_target)];

                constexpr TransitionInfo info{
                    .DstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .DstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                    .NewLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                };
                TransitionImage(command, texture, info);
                return VkRenderingAttachmentInfo{
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                    .imageView = texture.View,
                    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .loadOp = static_cast<VkAttachmentLoadOp>(render_info.ColorLoadOp),
                    .storeOp = static_cast<VkAttachmentStoreOp>(render_info.ColorStoreOp),
                    .clearValue = { render_info.ClearColor.X,
                                    render_info.ClearColor.Y,
                                    render_info.ClearColor.Z,
                                    render_info.ClearColor.W },
                };
            });

        VkRenderingAttachmentInfo depth_attachment_info{};
        if (render_info.DepthStencil != TextureHandle::eNull)
        {
            auto& depthStencil = textures[static_cast<uint32_t>(render_info.DepthStencil)];
            constexpr TransitionInfo info{
                .DstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .DstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
                .NewLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            };
            TransitionImage(command, depthStencil, info);
            depth_attachment_info = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = depthStencil.View,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .clearValue = {},
            };
        }

        const VkOffset2D offset = {
            .x = static_cast<int>(render_info.RenderArea.Offset.X),
            .y = static_cast<int>(render_info.RenderArea.Offset.Y),
        };

        const VkExtent2D extent = {
            .width = static_cast<uint32_t>(render_info.RenderArea.Extent.X),
            .height = static_cast<uint32_t>(render_info.RenderArea.Extent.Y),
        };

        const VkRenderingInfo vk_render_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = { .offset = offset, .extent = extent, },
            .layerCount = 1,
            .colorAttachmentCount = static_cast<uint32_t>(color_attachment_infos.size()),
            .pColorAttachments = color_attachment_infos.data(),
            .pDepthAttachment = render_info.DepthStencil != TextureHandle::eNull ? &depth_attachment_info : nullptr,
        };
        vkCmdBeginRendering(command.CommandBuffer, &vk_render_info);
    }

    inline void EndRender(const Command& command) { vkCmdEndRendering(command.CommandBuffer); }

    inline void
    BindVertexBuffers(const Command& command, const std::span<const VkBuffer>& buffers, std::span<const uint64_t> offsets)
    {
        vkCmdBindVertexBuffers(command.CommandBuffer, 0, buffers.size(), buffers.data(), offsets.data());
    }

    inline void BindIndexBuffer(const Command& command, VkBuffer buffer, uint32_t offset, IndexType type)
    {
        vkCmdBindIndexBuffer(command.CommandBuffer, buffer, offset, static_cast<VkIndexType>(type));
    }

    inline void Draw(
        const Command& command,
        uint32_t vertex_count,
        uint32_t instance_count,
        uint32_t first_vertex,
        uint32_t first_instance)
    {
        vkCmdDraw(command.CommandBuffer, vertex_count, instance_count, first_vertex, first_instance);
    }

    inline void DrawIndexed(
        const Command& command,
        uint32_t index_count,
        uint32_t instance_count,
        uint32_t first_index,
        int32_t vertex_offset,
        uint32_t first_instance)
    {
        vkCmdDrawIndexed(command.CommandBuffer, index_count, instance_count, first_index, vertex_offset, first_instance);
    }
}  // namespace Swift::Vulkan