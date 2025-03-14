#pragma once
#include "SwiftStructs.hpp"
#include "VulkanStructs.hpp"

#include <complex.h>

namespace Swift::Vulkan::Init
{
    Context CreateContext(const InitInfo& initInfo);

    vk::SwapchainKHR CreateSwapchain(
        const Context& context,
        vk::Extent2D extent,
        u32 queueIndex);

    vk::Queue GetQueue(
        const Context& context,
        u32 queueFamilyIndex,
        u32 localIndex,
        std::string_view debugName);

    Image CreateImage(
        const Context& context,
        VkImageCreateInfo& imageCreateInfo,
        VkImageViewCreateInfo& imageViewCreateInfo,
        std::string_view debugName);

    Image CreateImage(
        const Context& context,
        vk::ImageType imageType,
        vk::Extent3D extent,
        vk::Format format,
        vk::ImageUsageFlags usage,
        u32 mipLevels,
        vk::ImageCreateFlags flags,
        std::string_view debugName);

    std::vector<Image> CreateSwapchainImages(
        const Context& context,
        const Swapchain& swapchain);

    Buffer CreateBuffer(
        const Context& context,
        u32 queueFamilyIndex,
        vk::DeviceSize size,
        vk::BufferUsageFlags bufferUsageFlags,
        bool readback,
        std::string_view debugName);

    vk::Fence CreateFence(
        const Context& context,
        vk::FenceCreateFlags flags,
        std::string_view debugName);
    vk::Semaphore CreateSemaphore(
        const Context& context,
        std::string_view debugName);

    vk::CommandBuffer CreateCommandBuffer(
        const Context& context,
        vk::CommandPool commandPool,
        std::string_view debugName);
    vk::CommandPool CreateCommandPool(
        const Context& context,
        u32 queueIndex,
        std::string_view debugName);

    vk::Sampler CreateSampler(const Context& context);
    vk::Sampler CreateCubemapSampler(const Context& context);

    vk::DescriptorSetLayout CreateDescriptorSetLayout(vk::Device device);
    vk::DescriptorPool CreateDescriptorPool(
        vk::Device device,
        std::span<vk::DescriptorPoolSize> poolSizes);
    vk::DescriptorSet CreateDescriptorSet(
        vk::Device device,
        vk::DescriptorPool descriptorPool,
        vk::DescriptorSetLayout descriptorSetLayout);

    Shader CreateGraphicsShader(
        Context context,
        BindlessDescriptor descriptor,
        bool bUsePipeline,
        u32 pushConstantSize,
        std::string_view vertexPath,
        std::string_view fragmentPath,
        std::string_view debugName);

    Shader CreateComputeShader(
        Context context,
        BindlessDescriptor descriptor,
        bool bUsePipeline,
        u32 pushConstantSize,
        std::string_view computePath,
        std::string_view debugName);

    std::tuple<
        Image,
        Buffer>
    CreateDDSImage(
        const Context& context,
        Queue transferQueue,
        Command transferCommand,
        const std::filesystem::path& filePath,
        std::string_view debugName);
    std::tuple<
        Image,
        Buffer>
    CreateUnCompressedImage(
        const Context& context,
        Queue transferQueue,
        Command transferCommand,
        const std::filesystem::path& path,
        std::string_view debugName);
} // namespace Swift::Vulkan::Init