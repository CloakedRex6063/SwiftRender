#pragma once
#include "swift_structs.hpp"
#include "vector"
#include "vulkan_helpers.hpp"
#include "vulkan_structs.hpp"

namespace Swift::Vulkan
{
    std::expected<Context, Error> CreateContext(const ContextCreateInfo& create_info);
    std::expected<Queue, Error> CreateQueue(const Context& context, QueueType queue_type);
    std::expected<Command, Error> CreateCommand(const Context& context, Queue queue);
    std::expected<std::vector<Texture>, Error> GetSwapchainTextures(const Context& context);
    std::expected<VkFence, Error> CreateFence(const Context& context);
    std::expected<VkSemaphore, Error> CreateSemaphore(const Context& context, bool timeline = false);
    std::expected<ShaderLayout, Error> CreateShaderLayout(
        const Context& context,
        const ShaderLayoutCreateInfo& create_info);
    std::expected<Shader, Error> CreateGraphicsShader(
        const Context& context,
        const GraphicsShaderCreateInfo& shader_info,
        ShaderLayout shader_layout);
    std::expected<Buffer, Error> CreateBuffer(const Context& context, BufferCreateInfo create_info);

    void DestroyShader(const Context& context, const Shader& shader);
    void DestroyShaderLayout(const Context& context, ShaderLayout layout);
    void DestroyCommand(const Context& context, const Command& command);
    void DestroyTexture(const Context& context, const Texture& texture, bool only_view = false);
    void DestroyBuffer(const Context& context, const Buffer& buffer);
    void DestroySemaphore(const Context& context, VkSemaphore semaphore);
    void DestroyFence(const Context& context, VkFence fence);
    void DestroyContext(const Context& context);
} // namespace Swift::Vulkan