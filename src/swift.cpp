#include "swift.hpp"

#include "vector"
#include "vulkan_init.hpp"
#include "vulkan_render.hpp"

namespace Swift
{
    Vulkan::Context g_context;
    std::vector<Vulkan::Queue> g_queues;

    std::vector<Vulkan::Command> g_commands;
    std::vector<uint32_t> g_free_commands;

    std::vector<Vulkan::Texture> g_textures;
    std::vector<uint32_t> g_free_textures;

    std::vector<Vulkan::Buffer> g_buffers;
    std::vector<uint32_t> g_free_buffers;

    std::vector<Vulkan::ShaderLayout> g_shader_layouts;
    std::vector<uint32_t> g_free_shader_layouts;

    std::vector<Vulkan::Shader> g_shaders;
    std::vector<uint32_t> g_free_shaders;

    VkSemaphore g_image_acquire_semaphore = nullptr;
    VkSemaphore g_render_finished_semaphore = nullptr;
    VkFence g_render_fence = nullptr;
    uint64_t g_wait_value = 0;

    std::vector<TextureHandle> g_swapchain_textures;
    uint32_t g_image_index = 0;
}  // namespace Swift

std::expected<void, Swift::Error> Swift::CreateContext(const ContextCreateInfo& create_info)
{
    const auto exp_context = Vulkan::CreateContext(create_info);
    if (!exp_context)
    {
        return std::unexpected(exp_context.error());
    }
    g_context = exp_context.value();

    const auto exp_acquire_semaphore = Vulkan::CreateSemaphore(g_context);
    if (!exp_acquire_semaphore)
    {
        return std::unexpected(exp_acquire_semaphore.error());
    }
    g_image_acquire_semaphore = exp_acquire_semaphore.value();
    const auto exp_render_finished_semaphore = Vulkan::CreateSemaphore(g_context);
    if (!exp_render_finished_semaphore)
    {
        return std::unexpected(exp_render_finished_semaphore.error());
    }
    g_render_finished_semaphore = exp_render_finished_semaphore.value();

    const auto exp_wait_semaphore = Vulkan::CreateFence(g_context);
    if (!exp_wait_semaphore)
    {
        return std::unexpected(exp_wait_semaphore.error());
    }
    g_render_fence = exp_wait_semaphore.value();

    return {};
}
std::expected<Swift::QueueHandle, Swift::Error> Swift::GetQueue(const QueueType& queue_type)
{
    const auto exp_queue = Vulkan::CreateQueue(g_context, queue_type);
    if (!exp_queue)
    {
        return std::unexpected(Error::eQueueNotFound);
    }
    const auto queue_handle = static_cast<QueueHandle>(g_queues.size());
    g_queues.emplace_back(exp_queue.value());
    return queue_handle;
}

std::expected<Swift::CommandHandle, Swift::Error> Swift::CreateCommand(const QueueHandle queue_handle)
{
    const auto& queue = g_queues[static_cast<uint32_t>(queue_handle)];
    const auto exp_command = Vulkan::CreateCommand(g_context, queue);
    if (!exp_command)
    {
        return std::unexpected(exp_command.error());
    }

    if (g_free_commands.empty())
    {
        const auto command_handle = static_cast<CommandHandle>(g_commands.size());
        g_commands.emplace_back(exp_command.value());
        return command_handle;
    }

    const auto command_handle = static_cast<CommandHandle>(g_free_commands.back());
    g_commands[g_free_commands.back()] = exp_command.value();
    g_free_commands.pop_back();
    return command_handle;
}

std::expected<std::vector<Swift::TextureHandle>, Swift::Error> Swift::CreateSwapchainTextures()
{
    const auto exp_swapchain_tex = Vulkan::GetSwapchainTextures(g_context);
    if (!exp_swapchain_tex)
    {
        return std::unexpected(exp_swapchain_tex.error());
    }

    for (auto& texture : exp_swapchain_tex.value())
    {
        const auto handle = static_cast<TextureHandle>(g_textures.size());
        g_swapchain_textures.emplace_back(handle);
        g_textures.emplace_back(texture);
    }

    return g_swapchain_textures;
}

std::expected<Swift::ShaderLayoutHandle, Swift::Error> Swift::CreateShaderLayoutHandle(
    const ShaderLayoutCreateInfo& create_info)
{
    const auto exp_layout = Vulkan::CreateShaderLayout(g_context, create_info);
    if (!exp_layout)
    {
        return std::unexpected(exp_layout.error());
    }

    if (g_free_shader_layouts.empty())
    {
        const auto layout_handle = static_cast<ShaderLayoutHandle>(g_shader_layouts.size());
        g_shader_layouts.emplace_back(exp_layout.value());
        return layout_handle;
    }

    const auto handle = static_cast<ShaderLayoutHandle>(g_free_shader_layouts.back());
    g_shader_layouts[g_free_shader_layouts.back()] = exp_layout.value();
    g_free_shader_layouts.pop_back();
    return handle;
}

std::expected<Swift::ShaderHandle, Swift::Error> Swift::CreateShader(
    const GraphicsShaderCreateInfo& shader_info,
    const ShaderLayoutHandle& layout_handle)
{
    const auto& layout = g_shader_layouts[static_cast<uint32_t>(layout_handle)];
    const auto exp_shader = Vulkan::CreateGraphicsShader(g_context, shader_info, layout);
    if (!exp_shader)
    {
        return std::unexpected(exp_shader.error());
    }

    if (g_free_shaders.empty())
    {
        const auto shader_handle = static_cast<ShaderHandle>(g_shaders.size());
        g_shaders.emplace_back(exp_shader.value());
        return shader_handle;
    }

    const auto shader_handle = static_cast<ShaderHandle>(g_free_shaders.back());
    g_shaders[g_free_shaders.back()] = exp_shader.value();
    g_free_shaders.pop_back();
    return shader_handle;
}

std::expected<Swift::BufferHandle, Swift::Error> Swift::CreateBuffer(const BufferCreateInfo& create_info)
{
    const auto exp_buffer = Vulkan::CreateBuffer(g_context, create_info);
    if (!exp_buffer)
    {
        return std::unexpected(exp_buffer.error());
    }

    if (g_free_buffers.empty())
    {
        const auto buffer_handle = static_cast<BufferHandle>(g_buffers.size());
        g_buffers.emplace_back(exp_buffer.value());
        return buffer_handle;
    }

    const auto buffer_handle = static_cast<BufferHandle>(g_free_buffers.back());
    g_buffers[g_free_buffers.back()] = exp_buffer.value();
    g_free_buffers.pop_back();
    return buffer_handle;
}

std::expected<uint32_t, Swift::Error> Swift::AcquireNextImage()
{
    if (const auto wait_result = Vulkan::WaitFence(g_context, g_render_fence); !wait_result)
    {
        return std::unexpected(wait_result.error());
    }
    const auto result = Vulkan::AcquireNextImage(g_context, g_image_acquire_semaphore);
    if (!result)
    {
        return std::unexpected(result.error());
    }
    g_image_index = result.value();
    return g_image_index;
}

std::expected<void, Swift::Error> Swift::Present(QueueHandle queue_handle, CommandHandle command_handle)
{
    const auto& queue = g_queues[static_cast<uint32_t>(queue_handle)];
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];

    const Vulkan::SubmitInfo submit_info{
        .Commands = std::array{ command },
        .WaitSemaphores = std::array{ g_image_acquire_semaphore },
        .WaitStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .SignalSemaphores = std::array{ g_render_finished_semaphore },
    };
    if (const auto submitResult = Vulkan::Submit(queue, submit_info, g_render_fence); !submitResult)
    {
        return std::unexpected(submitResult.error());
    }
    return Vulkan::Present(g_context, queue, g_render_finished_semaphore, g_image_index);
}

std::expected<void, Swift::Error> Swift::WaitIdle() { return Vulkan::WaitIdle(g_context); }

void Swift::DestroyContext()
{
    Vulkan::DestroySemaphore(g_context, g_image_acquire_semaphore);
    Vulkan::DestroySemaphore(g_context, g_render_finished_semaphore);
    Vulkan::DestroyFence(g_context, g_render_fence);
    Vulkan::DestroyContext(g_context);
}

void Swift::DestroySwapchainTextures(std::span<const TextureHandle> textures)
{
    for (auto& texture_handle : textures)
    {
        const auto texture = g_textures[static_cast<uint32_t>(texture_handle)];
        Vulkan::DestroyTexture(g_context, texture, true);
        g_free_textures.emplace_back(static_cast<int>(texture_handle));
    }
    g_swapchain_textures.clear();
}

void Swift::DestroyTexture(TextureHandle texture_handle)
{
    const auto texture = g_textures[static_cast<uint32_t>(texture_handle)];
    Vulkan::DestroyTexture(g_context, texture);
    g_free_textures.emplace_back(static_cast<int>(texture_handle));
}

void Swift::DestroyCommand(CommandHandle command_handle)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::DestroyCommand(g_context, command);
    g_free_commands.emplace_back(static_cast<int>(command_handle));
}

void Swift::DestroyShaderLayout(ShaderLayoutHandle layout_handle)
{
    const auto& layout = g_shader_layouts[static_cast<uint32_t>(layout_handle)];
    Vulkan::DestroyShaderLayout(g_context, layout);
    g_free_shader_layouts.emplace_back(static_cast<int>(layout_handle));
}

void Swift::DestroyShader(ShaderHandle shader_handle)
{
    const auto& shader = g_shaders[static_cast<uint32_t>(shader_handle)];
    Vulkan::DestroyShader(g_context, shader);
    g_free_shaders.emplace_back(static_cast<int>(shader_handle));
}

void Swift::DestroyBuffer(BufferHandle buffer_handle)
{
    const auto& buffer = g_buffers[static_cast<uint32_t>(buffer_handle)];
    Vulkan::DestroyBuffer(g_context, buffer);
    g_free_buffers.emplace_back(static_cast<int>(buffer_handle));
}

std::expected<void, Swift::Error> Swift::Command::Begin(CommandHandle command_handle)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    if (const auto result = Vulkan::BeginCommand(g_context, command); !result)
    {
        return std::unexpected(result.error());
    }

    Vulkan::EnableDepthTest(command, false);
    Vulkan::EnableDepthWrite(command, false);
    Vulkan::EnableDepthBias(command, false);
    Vulkan::EnableDepthBoundsTest(command, false);
    Vulkan::EnableStencilTest(command, false);

    Vulkan::EnableRasterizerDiscard(command, false);
    Vulkan::SetCullMode(command, CullMode::eBack);
    Vulkan::SetFrontFace(command, FrontFace::eCounterClockwise);

    return {};
}

std::expected<void, Swift::Error> Swift::Command::End(CommandHandle command_handle)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];

    auto& texture = g_textures[static_cast<uint32_t>(g_swapchain_textures[g_image_index])];
    constexpr Vulkan::TransitionInfo transition_info{
        .DstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        .DstAccessMask = VK_ACCESS_NONE,
        .NewLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    Vulkan::TransitionImage(command, texture, transition_info);

    return Vulkan::EndCommand(command);
}

void Swift::Command::BindShader(CommandHandle command_handle, ShaderHandle shader_handle)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    const auto& shader = g_shaders[static_cast<uint32_t>(shader_handle)];
    Vulkan::BindShader(command, shader);
}

void Swift::Command::BindVertexBuffers(
    CommandHandle command_handle,
    std::span<BufferHandle> buffer_handles,
    std::span<const uint64_t> offsets)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    std::vector<VkBuffer> buffers(buffer_handles.size());
    std::ranges::transform(
        buffer_handles,
        buffers.begin(),
        [&](BufferHandle& buffer_handle)
        {
            const auto& buffer = g_buffers[static_cast<uint32_t>(buffer_handle)];
            return buffer.BaseBuffer;
        });
    Vulkan::BindVertexBuffers(command, buffers, offsets);
}

void Swift::Command::BindVertexBuffer(CommandHandle command_handle, BufferHandle buffer_handle, uint64_t offset)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    const auto& buffer = g_buffers[static_cast<uint32_t>(buffer_handle)];
    Vulkan::BindVertexBuffers(command, std::array{ buffer.BaseBuffer }, std::array{ offset });
}

void Swift::Command::BindIndexBuffer(
    CommandHandle command_handle,
    BufferHandle buffer_handle,
    uint32_t offset,
    IndexType type)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    const auto& buffer = g_buffers[static_cast<uint32_t>(buffer_handle)];
    Vulkan::BindIndexBuffer(command, buffer.BaseBuffer, offset, type);
}

void Swift::Command::BeginRender(CommandHandle command_handle, const RenderInfo& render_info)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::BeginRender(command, render_info, g_textures);
}

void Swift::Command::EndRender(CommandHandle command_handle)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::EndRender(command);
}

void Swift::Command::ClearImage(CommandHandle command_handle, TextureHandle texture_handle, Vec4 color)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    auto& texture = g_textures[static_cast<uint32_t>(texture_handle)];
    Vulkan::ClearImage(command, texture, color);
}

void Swift::Command::SetViewport(CommandHandle command_handle, const Viewport& viewport)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::SetViewport(command, viewport);
}

void Swift::Command::SetScissor(CommandHandle command_handle, const Rect& scissor)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::SetScissor(command, scissor);
}

void Swift::Command::EnableDepthWrite(CommandHandle command_handle, bool enable)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::EnableDepthWrite(command, enable);
}

void Swift::Command::EnableDepthTest(CommandHandle command_handle, bool enable)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::EnableDepthTest(command, enable);
}

void Swift::Command::EnableDepthBias(CommandHandle command_handle, bool enable)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::EnableDepthBias(command, enable);
}

void Swift::Command::EnableDepthBounds(CommandHandle command_handle, bool enable)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::EnableDepthBoundsTest(command, enable);
}

void Swift::Command::SetDepthBounds(CommandHandle command_handle, float min_depth_bounds, float max_depth_bounds)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::SetDepthBounds(command, min_depth_bounds, max_depth_bounds);
}

void Swift::Command::EnableRasterizerDiscard(CommandHandle command_handle, bool enable)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::EnableRasterizerDiscard(command, enable);
}

void Swift::Command::SetDepthBias(CommandHandle command_handle, float constant_factor, float clamp, float slope_factor)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::SetDepthBias(command, constant_factor, clamp, slope_factor);
}

void Swift::Command::SetCullMode(CommandHandle command_handle, CullMode mode)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::SetCullMode(command, mode);
}

void Swift::Command::SetFrontFace(CommandHandle command_handle, FrontFace front_face)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::SetFrontFace(command, front_face);
}

void Swift::Command::Draw(
    CommandHandle command_handle,
    uint32_t vertex_count,
    uint32_t instance_count,
    uint32_t first_vertex,
    uint32_t first_instance)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::Draw(command, vertex_count, instance_count, first_vertex, first_instance);
}

void Swift::Command::DrawIndexed(
    CommandHandle command_handle,
    uint32_t index_count,
    uint32_t instance_count,
    uint32_t first_index,
    int32_t vertex_offset,
    uint32_t first_instance)
{
    const auto& command = g_commands[static_cast<uint32_t>(command_handle)];
    Vulkan::DrawIndexed(command, index_count, instance_count, first_index, vertex_offset, first_instance);
}