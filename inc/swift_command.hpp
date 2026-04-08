#pragma once
#include "span"
#include "swift_macros.hpp"
#include "swift_shader.hpp"
#include "swift_structs.hpp"

namespace Swift
{
    class ICommandSignature;
    class IContext;
    class ICommand
    {
    public:
        SWIFT_DESTRUCT(ICommand);
        SWIFT_NO_MOVE(ICommand);
        SWIFT_NO_COPY(ICommand);

        virtual void* GetCommandList() = 0;
        virtual void* GetCommandAllocator() = 0;
        virtual void Begin() = 0;
        virtual void End() = 0;
        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetScissor(const Scissor& scissor) = 0;
        virtual void PushConstants(const void* data, uint32_t size, uint32_t offset = 0) = 0;
        virtual void BindShader(IShader* shader) = 0;
        virtual void DispatchMesh(uint32_t group_x, uint32_t group_y, uint32_t group_z) = 0;
        virtual void DispatchMeshIndirect(ICommandSignature* signature,
                                          uint32_t max_commands,
                                          IBuffer* argument_buffer,
                                          uint32_t argument_offset,
                                          IBuffer* count_buffer,
                                          uint32_t count_offset) = 0;
        virtual void DispatchCompute(uint32_t group_x, uint32_t group_y, uint32_t group_z) = 0;
        virtual void CopyBufferToTexture(IBuffer* buffer,
                                         ITexture* texture,
                                         uint16_t mip_levels = 1,
                                         uint16_t array_size = 1) = 0;
        virtual void CopyTextureToTexture(ITexture* src, ITexture* dst, const TextureCopyRegion& copy_region) = 0;
        virtual void CopyBufferToBuffer(IBuffer* src, IBuffer* dst, const BufferCopyRegion& region) = 0;
        virtual void BindConstantBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset = 0) = 0;
        virtual void BeginRender(std::span<const RenderAttachmentInfo> color_attachments,
                                 const std::optional<const DepthAttachmentInfo>& depth_attachment) = 0;
        void BeginRender(const std::optional<RenderAttachmentInfo>& color_attachment,
                         const std::optional<const DepthAttachmentInfo>& depth_attachment)
        {
            if (color_attachment.has_value())
            {
                BeginRender(std::span(&color_attachment.value(), 1), depth_attachment);
            }
            else
            {
                RenderAttachmentInfo render_attachment_info{};
                BeginRender(std::span(&render_attachment_info, 0), depth_attachment);
            }
        };
        virtual void EndRender() = 0;
        virtual void ClearRenderTarget(ITextureView* texture_handle, const Float4& color) = 0;
        virtual void ClearDepthStencil(ITextureView* texture_handle, float depth, uint8_t stencil) = 0;
        virtual void TransitionImage(ITexture* image, ResourceState new_state) = 0;
        virtual void TransitionBuffer(IBuffer* buffer, ResourceState new_state) = 0;
        virtual void UAVBarrier(IBuffer* buffer) = 0;
        virtual void UAVBarrier(ITexture* texture) = 0;

    protected:
        SWIFT_CONSTRUCT(ICommand);
    };
}  // namespace Swift
