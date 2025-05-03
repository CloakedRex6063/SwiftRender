#pragma once
#include "vulkan/vulkan_core.h"
#include "vk_mem_alloc.h"

namespace Swift::Vulkan
{
    struct DeviceFeatures
    {
        bool MultiDrawIndirect = false;
        bool Widelines = false;
        bool MultiViewport = false;
        bool SamplerAnisotropy = false;
    };

    struct PhysicalDevice
    {
        VkPhysicalDevice GPU = nullptr;
        DeviceFeatures Features{};
    };

    struct Context
    {
        VkInstance Instance = nullptr;
        PhysicalDevice GPU = {};
        VkDevice Device = nullptr;
        VmaAllocator Allocator = nullptr;
        VkSurfaceKHR Surface = nullptr;
        VkSwapchainKHR Swapchain = nullptr;
        VkFormat SwapchainFormat = VK_FORMAT_UNDEFINED;
    };

    struct Texture
    {
        VkImage Image = nullptr;
        VkImageView View = nullptr;
        VmaAllocation Allocation = nullptr;
        VkFormat Format = VK_FORMAT_UNDEFINED;
        VkImageLayout PreviousLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkAccessFlags PreviousAccess = VK_ACCESS_NONE;
        VkPipelineStageFlagBits PreviousStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint32_t Depth = 1;
        uint32_t MipLevels = 1;
    };
    
    struct Buffer
    {
        VkBuffer BaseBuffer = nullptr;
        VmaAllocation Allocation = nullptr;
        VkAccessFlags PreviousAccess = VK_ACCESS_NONE;
        VkPipelineStageFlags PreviousStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        uint64_t Size = 0;
    };

    struct ShaderLayout
    {
        VkPipelineLayout PipelineLayout = nullptr;
        VkDescriptorSetLayout DescriptorSetLayout = nullptr;
    };
    
    struct Shader
    {
        VkPipeline Pipeline = nullptr;
        VkPipelineBindPoint BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    };

    struct Queue
    {
        VkQueue BaseQueue;
        uint32_t QueueFamily;
    };

    struct Command
    {
        VkCommandBuffer CommandBuffer = nullptr;
        VkCommandPool CommandPool = nullptr;
    };
}