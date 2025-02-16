#include "Vulkan/VulkanRender.hpp"
#include "Vulkan/VulkanStructs.hpp"
#include "Vulkan/VulkanUtil.hpp"

namespace Swift::Vulkan
{
    u32 Render::AcquireNextImage(
        const Queue& queue,
        const Context& context,
        Swapchain& swapchain,
        const vk::Semaphore semaphore,
        const vk::Extent2D extent)
    {
        auto acquireInfo = vk::AcquireNextImageInfoKHR()
                                     .setDeviceMask(1)
                                     .setSemaphore(semaphore)
                                     .setSwapchain(swapchain)
                                     .setTimeout(std::numeric_limits<u64>::max());
        auto [result, image] = context.device.acquireNextImage2KHR(acquireInfo);

        if (result == vk::Result::eSuccess)
            return image;

        Util::HandleSubOptimalSwapchain(queue.index, context, swapchain, extent);
        acquireInfo = vk::AcquireNextImageInfoKHR()
                             .setDeviceMask(1)
                             .setSemaphore(semaphore)
                             .setSwapchain(swapchain)
                             .setTimeout(std::numeric_limits<u64>::max());
        std::tie(result, image) = context.device.acquireNextImage2KHR(acquireInfo);

        if (result == vk::Result::eSuccess)
            return image;
        
        throw std::runtime_error("Failed to acquire image after 2 tries!");
    }

    void Render::Present(
        const Context& context,
        Swapchain& swapchain,
        const Queue queue,
        vk::Semaphore semaphore,
        const vk::Extent2D extent)
    {
        auto presentInfo = vk::PresentInfoKHR()
                               .setImageIndices(swapchain.imageIndex)
                               .setWaitSemaphores(semaphore)
                               .setSwapchains(swapchain.swapchain);
        const auto result =
            vkQueuePresentKHR(queue.queue, reinterpret_cast<VkPresentInfoKHR*>(&presentInfo));

        if (result != VK_SUCCESS)
        {
            Util::HandleSubOptimalSwapchain(queue.index, context, swapchain, extent);
        }
    }
    
    void Render::BeginRendering(
        const vk::CommandBuffer commandBuffer,
        Swapchain& swapchain,
        const bool enableDepth)
    {
        const auto colorAttachment = vk::RenderingAttachmentInfo()
                                         .setImageView(GetSwapchainImage(swapchain).imageView)
                                         .setClearValue(vk::ClearColorValue().setFloat32({0.f}))
                                         .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                                         .setLoadOp(vk::AttachmentLoadOp::eClear)
                                         .setStoreOp(vk::AttachmentStoreOp::eStore);
        const auto depthAttachment = vk::RenderingAttachmentInfo()
                                         .setImageView(swapchain.depthImage.imageView)
                                         .setClearValue(vk::ClearColorValue().setFloat32({1.f}))
                                         .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
                                         .setLoadOp(vk::AttachmentLoadOp::eClear)
                                         .setStoreOp(vk::AttachmentStoreOp::eStore);
        const auto renderingInfo =
            vk::RenderingInfo()
                .setColorAttachments(colorAttachment)
                .setPDepthAttachment(enableDepth ? &depthAttachment : nullptr)
                .setLayerCount(1)
                .setRenderArea(vk::Rect2D().setExtent(swapchain.extent));
        commandBuffer.beginRendering(renderingInfo);
    }
    
    void Render::BeginRendering(
        const vk::CommandBuffer commandBuffer,
        const vk::Extent2D extent,
        const std::span<Image>& colorImage,
        const Image& depthImage,
        const bool enableDepth)
    {
        std::vector<vk::RenderingAttachmentInfo> colorAttachments;
        colorAttachments.reserve(colorImage.size());
        for (const auto& image : colorImage)
        {
            const auto colorAttachment =
                vk::RenderingAttachmentInfo()
                    .setImageView(image.imageView)
                    .setClearValue(vk::ClearColorValue().setFloat32({0.f}))
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore);
            colorAttachments.emplace_back(colorAttachment);
        }

        const auto depthAttachment = vk::RenderingAttachmentInfo()
                                         .setImageView(depthImage.imageView)
                                         .setClearValue(vk::ClearColorValue().setFloat32({1.f}))
                                         .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
                                         .setLoadOp(vk::AttachmentLoadOp::eClear)
                                         .setStoreOp(vk::AttachmentStoreOp::eStore);
        const auto renderingInfo =
            vk::RenderingInfo()
                .setColorAttachments(colorAttachments)
                .setPDepthAttachment(enableDepth ? &depthAttachment : nullptr)
                .setLayerCount(1)
                .setRenderArea(vk::Rect2D().setExtent(extent));
        commandBuffer.beginRendering(renderingInfo);
    }
    
    void Render::BeginRendering(
        const vk::CommandBuffer commandBuffer,
        const vk::ImageView& renderImageView,
        const vk::ImageView& depthImageView,
        const vk::Extent2D extent,
        const u32 viewMask,
        const int layerCount)
    {
        const auto colorAttachment = vk::RenderingAttachmentInfo()
                                         .setImageView(renderImageView)
                                         .setClearValue(vk::ClearColorValue().setFloat32({0.f}))
                                         .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                                         .setLoadOp(vk::AttachmentLoadOp::eLoad)
                                         .setStoreOp(vk::AttachmentStoreOp::eStore);
        const auto depthAttachment = vk::RenderingAttachmentInfo()
                                         .setImageView(depthImageView)
                                         .setClearValue(vk::ClearColorValue().setFloat32({0.f}))
                                         .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
                                         .setLoadOp(vk::AttachmentLoadOp::eClear)
                                         .setStoreOp(vk::AttachmentStoreOp::eStore);
        const auto renderingInfo =
            vk::RenderingInfo()
                .setColorAttachments(colorAttachment)
                .setPDepthAttachment(depthImageView ? &depthAttachment : nullptr)
                .setLayerCount(layerCount)
                .setRenderArea(vk::Rect2D().setExtent(extent))
                .setViewMask(viewMask);
        commandBuffer.beginRendering(renderingInfo);
    }
} // namespace Swift::Vulkan