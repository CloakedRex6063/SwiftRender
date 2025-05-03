#pragma once
#include "swift_structs.hpp"

namespace Swift
{
    std::expected<void, Error> CreateContext(const ContextCreateInfo& create_info);
    std::expected<QueueHandle, Error> GetQueue(const QueueType& queue_type);
    std::expected<CommandHandle, Error> CreateCommand(QueueHandle queue_handle);
    std::expected<std::vector<TextureHandle>, Error> CreateSwapchainTextures();
    std::expected<ShaderLayoutHandle, Error> CreateShaderLayoutHandle(const ShaderLayoutCreateInfo& create_info);
    std::expected<ShaderHandle, Error> CreateShader(
        const GraphicsShaderCreateInfo& shader_info,
        const ShaderLayoutHandle& layout_handle);
    std::expected<BufferHandle, Error> CreateBuffer(const BufferCreateInfo& create_info);

    std::expected<uint32_t, Error> AcquireNextImage();
    std::expected<void, Error> Present(QueueHandle queue_handle, CommandHandle command_handle);
    std::expected<void, Error> WaitIdle();

    void DestroyContext();
    void DestroySwapchainTextures(std::span<const TextureHandle> textures);
    void DestroyTexture(TextureHandle texture_handle);
    void DestroyCommand(CommandHandle command_handle);
    void DestroyShaderLayout(ShaderLayoutHandle layout_handle);
    void DestroyShader(ShaderHandle shader_handle);
    void DestroyBuffer(BufferHandle buffer_handle);

    namespace Command
    {
        std::expected<void, Error> Begin(CommandHandle command_handle);
        std::expected<void, Error> End(CommandHandle command_handle);
        void BeginRender(CommandHandle command_handle, const RenderInfo& render_info);
        void EndRender(CommandHandle command_handle);
        void SetViewport(CommandHandle command_handle, const Viewport& viewport);
        void SetScissor(CommandHandle command_handle, const Rect& scissor);
        void EnableDepthWrite(CommandHandle command_handle, bool enable);
        void EnableDepthTest(CommandHandle command_handle, bool enable);
        void EnableDepthBias(CommandHandle command_handle, bool enable);
        void EnableDepthBounds(CommandHandle command_handle, bool enable);
        void SetDepthBounds(CommandHandle command_handle, float min_depth_bounds, float max_depth_bounds);
        void SetDepthBias(CommandHandle command_handle, float constant_factor, float clamp, float slope_factor);
        void EnableRasterizerDiscard(CommandHandle command_handle, bool enable);
        void BindShader(CommandHandle command_handle, ShaderHandle shader_handle);
        void BindVertexBuffer(CommandHandle command_handle, BufferHandle buffer_handle, uint64_t offset = 0);
        void BindVertexBuffers(
            CommandHandle command_handle,
            std::span<BufferHandle> buffer_handles,
            std::span<const uint64_t> offsets);
        void BindIndexBuffer(
            CommandHandle command_handle,
            BufferHandle buffer_handle,
            uint32_t offset = 0,
            IndexType type = IndexType::eUInt32);
        void ClearImage(CommandHandle command_handle, TextureHandle texture_handle, Vec4 color);
        void SetCullMode(CommandHandle command_handle, CullMode mode);
        void SetFrontFace(CommandHandle command_handle, FrontFace front_face);
        void Draw(
            CommandHandle command_handle,
            uint32_t vertex_count,
            uint32_t instance_count,
            uint32_t first_vertex,
            uint32_t first_instance);
        void DrawIndexed(
            CommandHandle command_handle,
            uint32_t index_count,
            uint32_t instance_count,
            uint32_t first_index,
            int32_t vertex_offset,
            uint32_t first_instance);
    }  // namespace Command

    namespace Default
    {
        inline BlendAttachment BlendAttachment()
        {
            return Swift::BlendAttachment{
                .BlendEnable = false,
                .ColorBlendMask = ColorComponents::eA | ColorComponents::eB | ColorComponents::eG | ColorComponents::eR,
            };
        }

        inline BlendState BlendState(const Swift::BlendAttachment& attachment, uint32_t count)
        {
            const std::vector blend_attachments(count, attachment);
            const Swift::BlendState blend_state{
                .BlendAttachments = blend_attachments,
                .LogicOpEnable = false,
            };
            return blend_state;
        }

        template<typename T>
        VertexBinding VertexBinding()
        {
            return Swift::VertexBinding{ .Binding = 0, .Stride = sizeof(T), .InputRate = VertexInputRate::eVertex };
        }
    }  // namespace Default
}  // namespace Swift