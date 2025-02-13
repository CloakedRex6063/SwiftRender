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
} // namespace Swift::Vulkan